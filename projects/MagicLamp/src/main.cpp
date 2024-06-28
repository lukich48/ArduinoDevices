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
#define USE_WIFI 0
#define ESP8266 1
#define EEPROM_SIZE 4096
#define LOCK_PIN 12     // пин закрытой крышки
#define HC_ECHO 4       // пин Echo
#define HC_TRIG 5       // пин Trig

#define LED_DATA_PIN 13      // пин ленты
#define LED_CLOCK_PIN 14
#define LED_NUM 30      // к-во светодиодов

#define VB_DEB 0        // отключаем антидребезг (он есть у фильтра)
#define VB_CLICK 1500    // таймаут клика
#define HC_MAX_LEN 600L  // макс. расстояние измерения, мм
#include <VirtualButton.h>
VButton gest;

Adafruit_DotStar strip(LED_NUM, LED_DATA_PIN, LED_CLOCK_PIN, DOTSTAR_BRG);

// структура настроек
struct Data {
  bool state = 1;     // 0 выкл, 1 вкл
  byte mode = 0;      // 0 цвет, 1 теплота, 2 огонь
  byte brightness = 120;  // яркость
  byte hue[4] = {0, 55, 50, 0};      // цвет    
  byte sat[4] = {255, 225, 255, 255};      // насыщенность
  byte rainbow_timeout = 125;   // скорость радуги в мс
};

Data data;

// менеджер памяти
#include <EEManager.h>
EEManager mem(data, 10000);

int prev_br;
byte is_locked = 2; // для эффекта закрывания крышки

ConnectionSettings settings(
	ssid,
	wifiPass,
	mqttServer,
	mqttPort,
	mqttUser,
	mqttPass,
	"ardbeg/magiclamp"
);

#if USE_WIFI
  ConnectionHelper helper(&settings);
#endif

void print(string message)
{
  #if HOME_DEBUG
	  Serial.println(message.c_str());
  #endif
}

// получение расстояния с дальномера
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
int getFilterMedian(int val) {
  static int buf[3];
  static byte count = 0;

  // очистка фильтра
  if (val == -1){
    for(int i = 0; i < 3; i++) buf[i] = 0;
    count = 0; 
    return 0;
  }

  buf[count] = val;
  if (++count >= 3) count = 0;
  return (max(buf[0], buf[1]) == max(buf[1], buf[2])) ? max(buf[0], buf[2]) : max(buf[1], min(buf[0], buf[2]));
}

// пропускающий фильтр
#define FS_WINDOW 7   // количество измерений, в течение которого значение не будет меняться
#define FS_DIFF 80    // разница измерений, с которой начинается пропуск
int getFilterSkip(int val) {
  static int prev;
  static byte count;

  // очистка фильтра
  if (val == -1){
    prev = 0;
    count = 0; 
    return 0;
  }

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

  // очистка фильтра
  if (val == -1){
    filt = 0;
    return 0;
  }

  if (val) filt += (val * ES_MULT - filt) / ES_EXP;
  else filt = 0;  // если значение 0 - фильтр резко сбрасывается в 0
  // в нашем случае - чтобы применить заданную установку и не менять её вниз к нулю
  return filt / ES_MULT;
}

void setBrightness(uint8_t value){
  strip.setBrightness(value);
  strip.show();
  prev_br = value;
}

#define BR_STEP 4
void applyMode() {
  if (data.state) {
    if (data.mode < 2){ // актуально только для одноцветных режимов     
      uint32_t color = strip.ColorHSV(data.hue[data.mode] * 257, data.sat[data.mode], 255);
      color = strip.gamma32(color);
      strip.fill(color, 0, 0);

      // плавная смена яркости при ВКЛЮЧЕНИИ и СМЕНЕ РЕЖИМА
      if (prev_br != data.brightness) {
        int shift = prev_br > data.brightness ? -BR_STEP : BR_STEP;
        while (abs(prev_br - data.brightness) > BR_STEP) {
          prev_br += shift;
          strip.setBrightness(prev_br);
          strip.show();
          delay(10);
        }
        prev_br = data.brightness;
      }
      else{
        strip.setBrightness(data.brightness);
        strip.show();
      }
    }
    else{
      prev_br = data.brightness;
    }
  } else {
    // плавная смена яркости при ВЫКЛЮЧЕНИИ
    while (prev_br > 0) {
      prev_br -= BR_STEP;
      if (prev_br < 0) prev_br = 0;
      strip.setBrightness(prev_br);
      strip.show();
      delay(10);
    }
  }

  #if HOME_DEBUG
    helper.sender.publish("test/magic-lamp/apply",
    string("applyMode: ") + std::to_string(data.mode) 
      + " state: " + std::to_string(data.state)
      + " hue: " + std::to_string(data.hue[data.mode])
      + " sat: " + std::to_string(data.sat[data.mode])
      + " bright: " + std::to_string(data.brightness)
      , false);
  #endif
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

    // преобразуем в яркость от 125 до 255
    int br = map(fil_val, 0, 120, 125, 255);

    // преобразуем в цвет как текущий цвет + (0.. 20)
    int hue = data.hue[data.mode] + fil_val / 6;

    #if HOME_DEBUG
      helper.sender.publish("test/magic-lamp/fire", 
          string("hue: " + std::to_string(hue) +
          " sat: " + std::to_string(data.sat[data.mode]) +
          " brt: " + std::to_string(br))
          , false);
    #endif

    uint32_t color = strip.ColorHSV(hue * 257, data.sat[data.mode], br);
    color = strip.gamma32(color);
    strip.fill(color, 0, 0);
    strip.setBrightness(data.brightness);
    strip.show();
  }
}

void rainbow(){
  static long move_tmr;
  static uint16_t first_hue;

  if (millis() - move_tmr > map(data.rainbow_timeout, 0, 255, 4, 40)) {
    move_tmr = millis();

    strip.rainbow(first_hue, 1, data.sat[data.mode], 255, true);
    strip.setBrightness(data.brightness);
    strip.show();

    first_hue+=256;
  }
}

// подмигнуть яркостью
void pulse() {
  uint8_t pulse_br = (prev_br + 125 > 255) ? prev_br - 125 : prev_br + 125;

  if (pulse_br > prev_br)
  {
    for (int i = prev_br; i <= pulse_br; i += 10) {
      strip.setBrightness(i);
      strip.show();
      delay(5);
    }
    for (int i = pulse_br; i >= prev_br; i -= 10) {
      strip.setBrightness(i);
      strip.show();
      delay(5);
    }
  }
  else
  {
    for (int i = prev_br; i > pulse_br; i -= 10) {
      strip.setBrightness(i);
      strip.show();
      delay(5);
    }
    for (int i = pulse_br; i < prev_br; i += 10) {
      strip.setBrightness(i);
      strip.show();
      delay(5);
    }
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  pinMode(HC_TRIG, OUTPUT); // trig выход
  pinMode(HC_ECHO, INPUT);  // echo вход

  strip.begin();
  // mem.reset();
  mem.begin(0, 'a');  // запуск и чтение настроек

  strip.rainbow(0, 1, 255, 150, true);
  strip.show();
  delay(1000);
  strip.clear();
  strip.show();
  delay(10);

  applyMode();        // применить режим

  #if USE_WIFI
  helper.setup();
  #endif
}

void loop() {
  #if USE_WIFI
  helper.handle();
  #endif

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

  // крышка закрыта
  if (digitalRead(LOCK_PIN) == 0){
    delay(50);
    if (digitalRead(LOCK_PIN) == 0){
      if (is_locked == 0) {
        getFilterMedian(-1);
        getFilterSkip(-1);
        getFilterExp(-1);
        gest.reset();
        // эффект закрытой крышки
        for (byte i = 0; i < 3; i++){
        pulse();
        delay(50);
        }
      }
      is_locked = 1;
      return;
    }
  }
  is_locked = 0;

  // таймер 50мс, опрос датчика и вся основная логика
  static uint32_t tmr;
  if (millis() - tmr >= 50) {
    tmr = millis();

    static uint32_t tout;   // таймаут настройки (удержание)
    static int offset_d;    // оффсеты для настроек

    int dist1 = getDist(HC_TRIG, HC_ECHO); // получаем расстояние
    int dist2 = getFilterMedian(dist1);         // медиана
    int dist3 = getFilterSkip(dist2);           // пропускающий фильтр
    int dist_f = getFilterExp(dist3);      // усреднение

    // защита от дребезга
    static uint8_t count = 0;
    if (dist3)
      count++;
    else
      count = 0;

    #if HOME_DEBUG
      helper.sender.publish("test/magic-lamp/dist", 
        string("dist1: " + std::to_string(dist1) +
        " dist2: " + std::to_string(dist2) +
        " dist3: " + std::to_string(dist3) +
        " dist_f: " + std::to_string(dist_f) +
        " count: " + std::to_string(count) +
        " clicks: " + std::to_string(gest.clicks) +
        " hold(): " + std::to_string(gest.hold())
      ) 
      , false);
    #endif

    gest.poll(count >= 3 && dist3);                      // расстояние > 0 - это клик
    
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
    }

    // клик
    if (gest.click() && data.state) {
      pulse();  // мигнуть яркостью
    }

    // удержание (выполнится однократно)
    if (gest.held() && data.state) {
      offset_d = dist_f;    // оффсет расстояния для дальнейшей настройки
    }

    // удержание (выполнится пока удерживается)
    if (gest.hold() && data.state) {
      tout = millis();
      // смещение текущей настройки как оффсет + (текущее расстояние - расстояние начала)
      uint8_t shift = 0;   
      uint8_t offset_v = 0; 

      // меняем настройки
      switch (gest.clicks) {
        case 0: 
          // Удержание - всегда меняется яркость
          offset_v = data.brightness;
          shift = constrain(offset_v + (dist_f - offset_d), 20, 255);

          data.brightness = shift; 
          setBrightness(shift);
          break;
        case 1: 
          if (data.mode == 1){ // для белого меняем температуру
            offset_v = data.sat[data.mode];
            shift = constrain(offset_v + (dist_f - offset_d), 0, 255);
            data.sat[data.mode] = shift; 
            applyMode();
          }
          else if (data.mode == 2){ // для свечи ничего не делаем
          }
          else if (data.mode == 3){ // для радуги меняем скорость
            offset_v = data.rainbow_timeout;
            shift = constrain(offset_v + (dist_f - offset_d), 0, 255);
            data.rainbow_timeout = shift;
          }
          else{ // для остальных режимов меняем цвет
            offset_v = data.hue[data.mode];
            shift = constrain(offset_v + (dist_f - offset_d), 0, 255);
            data.hue[data.mode] = shift; 
            applyMode();
          }
          break; 
        case 2:
            // после 2 кликов удержание всегда меняет температуру
            offset_v = data.sat[data.mode];
            shift = constrain(offset_v + (dist_f - offset_d), 0, 255);
            data.sat[data.mode] = shift; 
            applyMode();
            break;
      }
      // подтягиваем ладонь
      offset_d = dist_f;

      mem.update();
    }
  }
}