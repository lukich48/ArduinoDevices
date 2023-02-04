// An example demonstrating how to control the Adafruit Dot Star RGB LED
// included on board the ItsyBitsy M4 board.

#include <Adafruit_DotStar.h>
#include <Wire.h>

#include "home_ConnectionSettings.h"
#include "home_ConnectionHelper.h"
#include <Secret.h>

const char* ssid = WI_FI_SSID;
const char* wifiPass = WI_FI_PASSWORD;
const char* mqttServer = MQTT_SERVER;
const int mqttPort = MQTT_PORT;
const char* mqttUser = MQTT_USER;
const char* mqttPass = MQTT_PASSWORD;

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

// There is only one pixel on the board
#define NUMPIXELS 30

//Use these pin definitions for the ItsyBitsy M4
#define DATAPIN    13
#define CLOCKPIN   14

Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow_old(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

void setColor(uint16_t hue)
{
    uint16_t first_saturation = 255;
    for( int i = 0; i < NUMPIXELS; i++){
        uint16_t saturation = first_saturation; // - (10*(i+1));
        uint32_t color = strip.ColorHSV(hue, saturation, 150);
        color = strip.gamma32(color);
        strip.setPixelColor(i, color);
        hue+=500;
    }
    strip.show();
}

void rainbow(unsigned long timeout = 100){
  static long move_tmr;
  static uint16_t first_hue; // = data.hue[data.mode];

  if (millis() - move_tmr > timeout) {
    move_tmr = millis();

    // strip.rainbow(first_hue, 1, 255, 150, true);
    // strip.show();
    // delay(10);

    // helper.sender.publish("test/magic-lamp/rainbow", 
    //     string("first_hue: " + std::to_string(first_hue) +
    //     " sat: " + std::to_string(data.sat[data.mode]) +
    //     " brt: " + std::to_string(data.bright[data.mode]))
    //     , false);

    //first_hue+=256;

    strip.rainbow(first_hue, 1, 255, 150, true);
    strip.show();
    first_hue+=256;
  }
}

void setup() {
  strip.begin(); // Initialize pins for output
//   strip.setBrightness(80);
//   strip.show();  // Turn all LEDs off ASAP
    //strip.rainbow(0, 1, 150, 255, true);

    // uint32_t color = strip.ColorHSV(13000, 175, 150);
    // color = strip.gamma32(color);
    
    // strip.fill(color, 0, 0);
    // strip.show();
    // setColor(13000);
    helper.setup();
}

void loop() {
  helper.handle();
  // rainbow(10);             // Flowing rainbow cycle along the whole strip
    //strip.rainbow(0, 1, 255, 150, true);
//   uint32_t color = strip.ColorHSV(52, 80, 255);
//     color = strip.gamma32(color);
    
//   strip.fill(color, 0, 0);
  //   strip.show();


  //  delay(1000);

  rainbow(10);

  // strip.rainbow(13000, 1, 255, 150, true);
  // strip.show();
  // delay(1000);
  // strip.rainbow(25000, 1, 255, 150, true);
  // strip.show();
  // delay(1000);
    // setColor(13000);
    // delay(1000);
    // setColor(25000);
    // delay(1000);
        

}
