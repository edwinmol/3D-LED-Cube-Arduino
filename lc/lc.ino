#include <TimerOne.h>

//define the pin numbers
#define latch_pin 1 //can be any pin we choose
#define blank_pin 2 //can be any pin we choose

#define layer_select_pin_0 3 // select layer bit 1
#define layer_select_pin_1 4 // select layer bit 2
#define layer_select_pin_2 5 // select layer bit 3

#define red_data_pin 6 // data for the red LEDs
#define green_data_pin 7 // data for the green LEDs
#define blue_data_pin 8 // data for the blue LEDs

#define clock_pin 9 //clock pin, we do not use SPI, so can be any pin

//setup the data structures
byte layer = 0; //keep track of the current layer
byte bamcounter = 0; //4bit resolution = 16 passes per cycle x layers = 16 x 8 = 128 
byte BAMBit = 0; //0 to 3 -> first bit = Most Significant Bit

// Now the data for the LEDs
// We have to store this in performance optimized format,
// such that when these values are written to the LEDs, we use as less cpu cycles as possible
//
// Given that fact that we use BAM, we will store the bits in byte arrays that can be pushed 
// out very directly.
// We use multidimensional arrays, every array has an row-column-layer stucture and each 
// value represents a brightness value of a specific LED in the 3D-Cube
byte red[8][8][8]; 
byte green[8][8][8];
byte blue[8][8][8];

void setup() {
  pinMode(blank_pin, OUTPUT);//Output Enable  

  // Disable output
  // important to do this first, so LEDs do not flash on boot up
  digitalWrite(blank_pin, HIGH);
  
  //while setting up, we don't want to get thrown out of the setup function
  noInterrupts(); 

  //finally set up the Outputs
  //LATCH
  pinMode(latch_pin, OUTPUT);

  // DATA
  pinMode(red_data_pin, OUTPUT);
  pinMode(green_data_pin, OUTPUT);
  pinMode(blue_data_pin, OUTPUT);

  // LAYER SELECT
  pinMode(layer_select_pin_0, OUTPUT);
  pinMode(layer_select_pin_1, OUTPUT);
  pinMode(layer_select_pin_2, OUTPUT);

  // CLOCK
  pinMode(clock_pin, OUTPUT);
  
  //125 uSeconds = 8kHz sampling fequency. 
  Timer1.initialize(125);   //timer in microseconds
  Timer1.attachInterrupt(driveLayer);  // attaches driveLayer() as a timer overflow interrupt

  clear();
  interrupts(); //re-enable interrupts
}

void driveLayer()
{
 
  // first turn the output off
  digitalWrite(blank_pin, HIGH);
  
  // now select the layer
  selectLayer(layer);
  
  // then we write the RGB values
  writeRGBValues(layer, BAMBit); 
  
  // latch the data
  digitalWrite(latch_pin,HIGH);
  digitalWrite(latch_pin,LOW);

  // turn the output on
  digitalWrite(blank_pin, LOW);
  
  // go to the next layer 
  layer++;
  if (layer == 8)  {
    //go back to 0
    layer = 0;
  }
  // increase the bam counter
  bamcounter++;
  if (bamcounter == 128) {
    //go back to 0
    bamcounter = 0;
  }
  // Finally set the BAMBit 
  if (bamcounter < 64) {
    BAMBit = 0;
  } else if (bamcounter < 96) {
    BAMBit = 1;
  } else if (bamcounter < 112) {
    BAMBit = 2;
  } else {
    BAMBit = 3;
  }  
  
}

void selectLayer(byte layer) {
    //check layer and convert to 3 bits output
    writeOutput(layer,0b00000001,layer_select_pin_0);
    writeOutput(layer,0b00000010,layer_select_pin_1);
    writeOutput(layer,0b00000100,layer_select_pin_2);
}

void writeOutput(byte value,byte mask, byte pin) {
    if (layer & mask) {
        digitalWrite(pin,HIGH);
    } else {
        digitalWrite(pin,LOW);
    } 
}

//function to control a LED
void LED (byte row, byte column, byte layer, byte r, byte g, byte b) {
  //write to the data structure, the driveLayer interrupt will push it to the HW
  int index = +column/8;
  for (int i=0; i<4; i++) {      
      red[row][column][layer] = r;        
      green[row][column][layer] = g;        
      green[row][column][layer] = b;           
  }
}

void clear() {
  for (int r=0;r<8;r++) {
    for (int c=0;c<8;c++) {
        for (int l=0;l<8;l++) {
            LED (r,c,l,0,0,0);
        }
    }
  }
}

void writeRGBValues(byte layer, byte BAMbit) {
  for (int row=0; row<8; row++) {
      for (int column=0; column<8; column++) {
        writeOutput(red[row][column][layer], BAMbit, red_data_pin);
        writeOutput(green[row][column][layer], BAMbit, green_data_pin);
        writeOutput(blue[row][column][layer], BAMbit, blue_data_pin);
        toggleClockPulse();
    }    
  }
} 

void toggleClockPulse() {    
    digitalWrite(clock_pin,HIGH);
    digitalWrite(clock_pin,LOW);
}

void loop() {

  doRandom();
  RGBGlow();
  
}


// ANIMATION CODE

void doRandom() {
  for (int i=0; i<128; i++) {
    LED(random(16), random(16), random(8), random(16), random(16), random(16));
    delay(random(100));    
    if (random(100)%100==0) {
        clear();
    }
  }
  clear();
}

#define RED 0
#define GREEN 1
#define BLUE 2
#define YELLOW 3
#define CYAN 4
#define MAGENTA 5

void RGBGlow() {
   
  glow(RED);
  glow(YELLOW);  
  glow(GREEN);
  glow(CYAN);
  glow(BLUE);
  glow(MAGENTA);
  
}


void glow(int color) {
    byte value;
    for (int r=0; r<8; r++) {
      for (int c=0; c<8; c++) {
        for (int l=0; l<8; l++) {
          value = r*2;
          if (value > 15) {
            value = 31 - value;
          }
          switch (color) {
            case RED:
              LED(r, c, l, value, 0, 0);
              break;
            case GREEN:
              LED(r, c, l, 0, value, 0);
              break;
            case BLUE:
              LED(r, c, l, 0, 0, value);
              break;
            case YELLOW:
              LED(r, c, l, value, value, 0);
              break;            
            case CYAN:
              LED(r, c, l, 0, value, value);
              break;            
            case MAGENTA:
              LED(r, c, l, value, 0, value);
              break;            
              
          }
          delay(1);
       }
    }
  }
}
