/*
  Основано на прокте AlexGyver https://kit.alexgyver.ru/tutorials/magic-lamp/
*/

#include <Arduino.h>
#include <Adafruit_DotStar.h>
#include <Wire.h>
#include <string>

#include "home_ConnectionSettings.h"
#include "home_ConnectionHelper.h"
#include <Secret.h>

using std::string;

const char* ssid = WI_FI_SSID;
const char* wifiPass = WI_FI_PASSWORD;
const char* mqttServer = MQTT_SERVER;
const int mqttPort = MQTT_PORT;
const char* mqttUser = MQTT_USER;
const char* mqttPass = MQTT_PASSWORD;

#define HOME_DEBUG 0
#define HC_ECHO 4       // пин Echo
#define HC_TRIG 5       // пин Trig

#define LED_DATA_PIN 13      // пин ленты
#define LED_CLOCK_PIN 14
#define LED_NUM 30      // к-во светодиодов

#define VB_DEB 0        // отключаем антидребезг (он есть у фильтра)
#define VB_CLICK 1500    // таймаут клика
#include <VirtualButton.h>
VButton gest;

Adafruit_DotStar strip(LED_NUM, LED_DATA_PIN, LED_CLOCK_PIN, DOTSTAR_BRG);

// структура настроек
struct Data {
  bool state = 1;     // 0 выкл, 1 вкл
  byte mode = 0;      // 0 цвет, 1 теплота, 2 огонь
  byte bright[4] = {100, 100, 100, 150};  // яркость
  byte hue[4] = {0, 50, 50, 0};      // цвет    
  byte sat[4] = {255, 175, 175, 255};      // насыщенность
};

Data data;

// менеджер памяти
#include <EEManager.h>
EEManager mem(data);

int prev_br;

ConnectionSettings settings(
	ssid,
	wifiPass,
	mqttServer,
	mqttPort,
	mqttUser,
	mqttPass,
	"ardbeg/magiclamp"
);

ConnectionHelper helper(&settings);

void print(string message)
{
  if (HOME_DEBUG){
	  Serial.println(message.c_str());
  }
}

// получение расстояния с дальномера
#define HC_MAX_LEN 500L  // макс. расстояние измерения, мм
int getDist(byte trig, byte echo) {
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // измеряем время ответного импульса
  uint32_t us = pulseIn(echo, HIGH, (HC_MAX_LEN * 2000 / 343));

  // считаем расстояние и возвращаем
  return (us * 343L / 2000);
}

// медианный фильтр
int getFilterMedian(int newVal) {
  static int buf[3];
  static byte count = 0;
  buf[count] = newVal;
  if (++count >= 3) count = 0;
  return (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
}

// пропускающий фильтр
#define FS_WINDOW 7   // количество измерений, в течение которого значение не будет меняться
#define FS_DIFF 80    // разница измерений, с которой начинается пропуск
int getFilterSkip(int val) {
  static int prev;
  static byte count;

  if (!prev && val) prev = val;   // предыдущее значение 0, а текущее нет. Обновляем предыдущее
  // позволит фильтру резко срабатывать на появление руки

  // разница больше указанной ИЛИ значение равно 0 (цель пропала)
  if (abs(prev - val) > FS_DIFF || !val) {
    count++;
    // счётчик потенциально неправильных измерений
    if (count > FS_WINDOW) {
      prev = val;
      count = 0;
    } else val = prev;
  } else count = 0;   // сброс счётчика
  prev = val;
  
  return val;
}

// экспоненциальный фильтр со сбросом снизу
#define ES_EXP 2L     // коэффициент плавности (больше - плавнее)
#define ES_MULT 16L   // мультипликатор повышения разрешения фильтра
int getFilterExp(int val) {
  static long filt;
  if (val) filt += (val * ES_MULT - filt) / ES_EXP;
  else filt = 0;  // если значение 0 - фильтр резко сбрасывается в 0
  // в нашем случае - чтобы применить заданную установку и не менять её вниз к нулю
  return filt / ES_MULT;
}

#define BR_STEP 4
void applyMode() {
  if (data.state) {

    uint32_t color = strip.ColorHSV(data.hue[data.mode] * 257, data.sat[data.mode], data.bright[data.mode]);
    color = strip.gamma32(color);
    strip.fill(color, 0, 0);
    strip.show();
    
  // if (HOME_DEBUG){
  //     print(string("applyMode: ") + std::to_string(data.mode) 
  //       + " hue: " + std::to_string(data.hue[data.mode])
  //       + " sat: " + std::to_string(data.sat[data.mode])
  //       + " bright: " + std::to_string(data.bright[data.mode])
  //       );
  // }

    // плавная смена яркости при ВКЛЮЧЕНИИ и СМЕНЕ РЕЖИМА
    if (prev_br != data.bright[data.mode]) {
      int shift = prev_br > data.bright[data.mode] ? -BR_STEP : BR_STEP;
      while (abs(prev_br - data.bright[data.mode]) > BR_STEP) {
        prev_br += shift;
        strip.setBrightness(prev_br);
        strip.show();
        print(string("setBrightness1: " + std::to_string(prev_br)));
        delay(10);
      }
      prev_br = data.bright[data.mode];
    }
  } else {
    // плавная смена яркости при ВЫКЛЮЧЕНИИ
    while (prev_br > 0) {
      prev_br -= BR_STEP;
      if (prev_br < 0) prev_br = 0;
      strip.setBrightness(prev_br);
      strip.show();
      print(string("setBrightness2: " + std::to_string(prev_br)));
      delay(10);
    }
  }
}

// огненный эффект
void fireTick() {
  static uint32_t rnd_tmr, move_tmr;
  static int rnd_val, fil_val;
  
  // таймер 100мс, генерирует случайные значения
  if (millis() - rnd_tmr > 100) {
    rnd_tmr = millis();
    rnd_val = random(0, 13);
  }
  
  // таймер 20мс, плавно движется к rnd_val
  if (millis() - move_tmr > 20) {
    move_tmr = millis();
    // эксп фильтр, на выходе получится число 0..120
    fil_val += (rnd_val * 10 - fil_val) / 5;

    // преобразуем в яркость от 100 до 255
    int br = map(fil_val, 0, 120, 100, 255);

    // преобразуем в цвет как текущий цвет + (0.. 24)
    int hue = data.hue[2] + fil_val / 5;

    uint32_t color = strip.ColorHSV(hue * 257, 255, br);
    color = strip.gamma32(color);
    strip.fill(color, 0, 0);
    strip.show();
  }
}

void rainbow(unsigned long timeout = 10){
  static long move_tmr;
  static uint16_t first_hue;

  if (millis() - move_tmr > timeout) {
    move_tmr = millis();

    strip.rainbow(first_hue, 1, data.sat[data.mode], data.bright[data.mode], true);
    strip.show();

    if (HOME_DEBUG)
    helper.sender.publish("test/magic-lamp/rainbow", 
        string("first_hue: " + std::to_string(first_hue) +
        " sat: " + std::to_string(data.sat[data.mode]) +
        " brt: " + std::to_string(data.bright[data.mode]))
        , false);

    first_hue+=256;
  }
}

// подмигнуть яркостью
void pulse() {
  uint8_t pulse_br = (prev_br + 125 > 255) ? prev_br - 125 : prev_br + 125;

  if (pulse_br > prev_br)
  {
    for (int i = prev_br; i <= pulse_br; i += 15) {
      strip.setBrightness(i);
      strip.show();
      delay(10);
    }
    for (int i = pulse_br; i >= prev_br; i -= 15) {
      strip.setBrightness(i);
      strip.show();
      delay(10);
    }
  }
  else
  {
    for (int i = prev_br; i > pulse_br; i -= 5) {
      strip.setBrightness(i);
      strip.show();
      delay(10);
    }
    for (int i = pulse_br; i < prev_br; i += 5) {
      strip.setBrightness(i);
      strip.show();
      delay(10);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(HC_TRIG, OUTPUT); // trig выход
  pinMode(HC_ECHO, INPUT);  // echo вход

  strip.begin();

  mem.begin(0, 'a');  // запуск и чтение настроек
  applyMode();        // применить режим

  helper.setup();
}

void loop() {
  helper.handle();

  mem.tick();   // менеджер памяти

  if (data.state)
    switch (data.mode){
      case 2:
        fireTick();   // анимация огня
        break;
      case 3:
        rainbow(); // радуга
        break;
    }

  // таймер 50мс, опрос датчика и вся основная логика
  static uint32_t tmr;
  if (millis() - tmr >= 50) {
    tmr = millis();

    static uint32_t tout;   // таймаут настройки (удержание)
    static int offset_d;    // оффсеты для настроек
    static byte offset_v;

    int dist1 = getDist(HC_TRIG, HC_ECHO); // получаем расстояние
    int dist2 = getFilterMedian(dist1);         // медиана
    int dist3 = getFilterSkip(dist2);           // пропускающий фильтр
    int dist_f = getFilterExp(dist3);      // усреднение

    // защита от дребезга
    static uint8_t count = 0;
    count += dist3 ? 1 :0;
    if (!dist3) count = 0;

    gest.poll(count >= 3 && dist3);                      // расстояние > 0 - это клик

    if (HOME_DEBUG)
      helper.sender.publish("test/magic-lamp/dist", 
        string("dist1: " + std::to_string(dist1) +
        " dist2: " + std::to_string(dist2) +
        " dist3: " + std::to_string(dist3) +
        " dist_f: " + std::to_string(dist_f) +
        " count: " + std::to_string(count) +
        " clicks: " + std::to_string(gest.clicks)
      ) 
      , false);

    // есть клики и прошло 2 секунды после настройки
    if (gest.hasClicks() && millis() - tout > 2000) {
      switch (gest.clicks) {
        case 1:
          data.state = !data.state;  // вкл/выкл
          applyMode();
          break;
        case 2:
          // если включена И меняем режим (0.. 1)
          if (data.state && ++data.mode > 1) data.mode = 0;
          applyMode();
          break;
        case 3:
          // свеча
          if (data.state) data.mode = 2;
          break;
        case 4:
          // радуга
          if (data.state) data.mode = 3;
          break;
      }
      mem.update();

      if (HOME_DEBUG)
      helper.sender.publish("test/magic-lamp/clicks", 
        string("clicks: " + std::to_string(gest.clicks))
        , false);
    }

    // клик
    if (gest.click() && data.state) {
      pulse();  // мигнуть яркостью
    }

    // удержание (выполнится однократно)
    if (gest.held() && data.state) {
      offset_d = dist_f;    // оффсет расстояния для дальнейшей настройки
      switch (gest.clicks) {
        case 0: offset_v = data.bright[data.mode]; break;   // оффсет яркости
        case 1:
          if (data.mode == 0) // для rgb меняем цвет
            offset_v = data.hue[data.mode];
          else // для белого режима меняем температуру
            offset_v = data.sat[data.mode];
          break;   
        case 2: offset_v = data.sat[data.mode]; break; 
      }
    }

    // удержание (выполнится пока удерживается)
    if (gest.hold() && data.state) {
      tout = millis();
      // смещение текущей настройки как оффсет + (текущее расстояние - расстояние начала)
      int shift = constrain(offset_v + (dist_f - offset_d), 0, 255);
      offset_d = dist_f;
      offset_v = shift;
      
      // меняем настройки
      switch (gest.clicks) {
        case 0: 
          // Удержание - всегда меняется яркость
          data.bright[data.mode] = shift; 
          print(string("data.bright[") + std::to_string(data.mode) + "] = " + std::to_string(shift));
          break;
        case 1: 
          if (data.mode == 0){ // для rgb меняем цвет
            data.hue[data.mode] = shift; 
            print(string("data.hue[") + std::to_string(data.mode) + "] = " + std::to_string(shift));
          }
          else{ // для белого режима меняем температуру
            data.sat[data.mode] = shift; 
            print(string("data.sat[") + std::to_string(data.mode) + "] = " + std::to_string(shift));
          }
          break; 
        case 2:
            // после 2 кликов удержание всегда меняет температуру
            data.sat[data.mode] = shift; 
            print(string("data.sat[") + std::to_string(data.mode) + "] = " + std::to_string(shift));
            break;
      }
      applyMode();
      mem.update();
    }
  }
}