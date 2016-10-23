#include <Adafruit_NeoPixel.h>

#define PIN 2  

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, 2, NEO_GRBW + NEO_KHZ800);

/*
 5 "flames" of 3 pixels each.
 Each flame can have a brightness of 0 to 254 (play with this scale)
 Eventually, light up center pixel of three first then the sides. This version will just distribute amongst the 3 pixels.
*/

#define NUMBER_OF_FLAMES 5 // depends on number of neopixel triplets. 5 for 16 NeoPixel ring. 4 for 12 NeoPixel ring
#define FLICKER_CHANCE 3 // increase this to increase the chances an individual flame will flicker

uint32_t rez_range = 256 * 4;
#define D_ false

// console buttons:
struct flame_element {
  int brightness;
  int step;
  int max_brightness;
  long rgbw[4];
  byte state;
} flames[5];

int new_brightness = 0;
unsigned long rgbw[4]; // reusable temporary array
uint8_t scaleD_rgbw[4];
byte acc;
 
 #define SCALERVAL 255*4
 const int flamecolors[22][4] = {
{ SCALERVAL, 0,  0, 0}, // Red
{ SCALERVAL, 0,  0, 0},
{ SCALERVAL, 0,  0, 0},
{ SCALERVAL, 0,  0, 0},
{ SCALERVAL, 0,  0, 0},
{ SCALERVAL, 0,  0, 0},
{ SCALERVAL, 0,  0, 0},
{ SCALERVAL, 0,  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.4),  0, 0}, // oranges
{ SCALERVAL, static_cast<const int>(SCALERVAL*.4),  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.4),  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.4),  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.3),  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.3),  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.3),  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.3),  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.3),  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.3),  0, 0},
{ SCALERVAL, static_cast<const int>(SCALERVAL*.3),  0, 0},
{ 0, 0,  0, SCALERVAL * 0.5}, // white
{ 0, static_cast<const int>(SCALERVAL * 0.2),  SCALERVAL, 0}, // that one blue flame
{ SCALERVAL,  static_cast<const int>(SCALERVAL * 0.3),  static_cast<const int>(SCALERVAL * 0.5), 0} // violet
};

  
  
void setup() {
 // if (D_){
    Serial.begin(9600);
    Serial.println("STARTUP");
//  }
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  randomSeed(analogRead(4));
  InitFlames();
  
}

void loop() {
 //return;
 for(byte flame_count = 0; flame_count < NUMBER_OF_FLAMES; flame_count++) {
    switch(flames[flame_count].state){
      case 0: // reset
        CreateNewFlame(flame_count);
        break;
      
      case 1: // increasing
        new_brightness = flames[flame_count].brightness + flames[flame_count].step;
        if (new_brightness > flames[flame_count].max_brightness){
          UpdateFlameColor(flame_count, flames[flame_count].max_brightness);
          flames[flame_count].brightness = flames[flame_count].max_brightness;
          flames[flame_count].step = GetStepSize(); // pick a different speed for flame going out
          flames[flame_count].state = 2;
        } else {
          UpdateFlameColor(flame_count, new_brightness);
          flames[flame_count].brightness = new_brightness;
        }
        break;

      case 2: // decreasing
        new_brightness = flames[flame_count].brightness - flames[flame_count].step;
        // chance to flicker/rekindle:
        if (random(new_brightness) < FLICKER_CHANCE){
          // rekindle:
          flames[flame_count].state = 1; // increase again
          flames[flame_count].brightness = max(GetMaxBrightness(), flames[flame_count].brightness);
          flames[flame_count].step = GetStepSize(); 
        
        } else {
          if (new_brightness < 1){
            flames[flame_count].state = 0; // bottomed out - reset to next flame
            flames[flame_count].brightness = 0;
             UpdateFlameColor(flame_count, 0);
          } else {
            UpdateFlameColor(flame_count, new_brightness);
            flames[flame_count].brightness = new_brightness;
          }
        }
        break;
    }
  
  }
  strip.show();
  delay(100);
} //loop()


void InitFlames(){
  // Sets initial states in flames array
  for(byte i = 0; i < NUMBER_OF_FLAMES; i++) {
    flames[i].state = 0;
    
  }
}


void UpdateFlameColor(byte flame_num, int new_brightness){
// 
  uint32_t c = 0;
  uint32_t color_channel_value;
  byte rgbw_channel;
  
  new_brightness = min(new_brightness, flames[flame_num].max_brightness);
  

  for(byte rgbw_channel = 0; rgbw_channel < 4; rgbw_channel++) {
    color_channel_value = flames[flame_num].rgbw[rgbw_channel];
    color_channel_value = color_channel_value * (uint32_t)new_brightness; // keep it long
    color_channel_value = color_channel_value / (uint32_t)rez_range;
    rgbw[rgbw_channel] = max(0L, color_channel_value);
  } // step through R G B W



  // spread possible values of 0-768 across 3 pixels
  for(byte sub_pixel = 0; sub_pixel < 3; sub_pixel++) {
    
    for(byte i = 0; i < 4; i++) { // rgbw
      acc = rgbw[i] / 4;
      byte d = rgbw[i] % 4;
      if (sub_pixel < d){
        acc++;
      }
      scaleD_rgbw[i] = acc;
    }
    c = strip.Color(scaleD_rgbw[0], scaleD_rgbw[1], scaleD_rgbw[2], scaleD_rgbw[3]);
    strip.setPixelColor(flame_num * 3 + sub_pixel, c);
  }
  
}


void CreateNewFlame(byte flame_num){
  flames[flame_num].step = GetStepSize();
  flames[flame_num].max_brightness = GetMaxBrightness();

  flames[flame_num].brightness = 0;
  flames[flame_num].state = 1;
  byte color_index = random(22);
  for(byte i = 0; i < 4; i++) {
    flames[flame_num].rgbw[i] = flamecolors[color_index][i];
  }
 
}

int GetStepSize(){
   return random(70)+1;
}

int GetMaxBrightness(){
  int retVal;
  //retVal = random(rez_range/4) +  random(rez_range/4) + random(rez_range/4) + rez_range/4 +1; // bell curve
  retVal = random(rez_range * 4 / 4) +  rez_range/4; // flat distribution
  //retVal = random(rez_range / 2) +  rez_range / 2; // brighter flat distribution
  return retVal;
}
