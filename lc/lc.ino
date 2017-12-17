#include <TimerOne.h>
#include <SPI.h>

//define the pin numbers
#define latch_pin 8 //can be any pin we choose
#define blank_pin 9 //can be any pin we choose

#define layer_select_pin_0 4 // select layer bit 1
#define layer_select_pin_1 3 // select layer bit 2
#define layer_select_pin_2 2 // select layer bit 3

#define data_pin 11 // data pint (must be 11 = MOSI)
#define clock_pin 13 //clock pin (must be 13 = SPI CLK)

//setup the data structures
byte layer = 0; //keep track of the current layer
byte bamcounter = 0; //4bit resolution = 15 passes per cycle x layers = 15 x 8 = 120 
byte BAMBit = 0; //0 to 3 -> first bit = Most Significant Bit

// Now the data for the LEDs
// We have to store this in performance optimized format,
// such that when these values are written to the LEDs, we use as less cpu cycles as possible
//
// Given that fact that we use BAM, we will store the bits in byte arrays that can be pushed 
// out very directly.
// We use a multidimensional array:
// - the first index is the BAM bit
// - the second index is the layer
// - the last index is the bit array containing the bits to shift into the shift registers
byte rgb[4][8][24];

void setup() {
  pinMode(blank_pin, OUTPUT);//Output Enable  

  // Disable output
  // important to do this first, so LEDs do not flash on boot up
  digitalWrite(blank_pin, HIGH);

  //set all outputs to 0
  clear();

    //Setup SPI
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE0); //mode0 rising edge of data
  SPI.setClockDivider(SPI_CLOCK_DIV2); //run data at 16MHz/2 = 8MHz.
  
  //while setting up, we don't want to get thrown out of the setup function
  noInterrupts(); 

  //finally set up the Outputs
  //LATCH
  pinMode(latch_pin, OUTPUT);

  // DATA
  pinMode(data_pin, OUTPUT); //MOSI Data
  pinMode(clock_pin, OUTPUT); //SPI Clock

  // LAYER SELECT
  pinMode(layer_select_pin_0, OUTPUT);
  pinMode(layer_select_pin_1, OUTPUT);
  pinMode(layer_select_pin_2, OUTPUT);

  //start up the SPI library
  SPI.begin();
  
  //125 uSeconds = 8kHz sampling fequency. 
  Timer1.initialize(125);   //timer in microseconds
  Timer1.attachInterrupt(driveLayer);  // attaches driveLayer() as a timer overflow interrupt

  interrupts(); //re-enable interrupts
}

void driveLayer()
{
  
  // write the RGB values
  writeRGBValues(layer, BAMBit);  

  // turn the output off
  digitalWrite(blank_pin, HIGH); 

  // latch the data
  digitalWrite(latch_pin,HIGH);
  digitalWrite(latch_pin,LOW);
  
  // now select layer
  selectLayer(layer);
  
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
  if (bamcounter == 120) {
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
    if (value & mask) {
        digitalWrite(pin,HIGH);
    } else {
        digitalWrite(pin,LOW);
    } 
}

//function to control a LED
void LED (byte row, byte column, byte layer, byte r, byte g, byte b) {
  //write to the data structure, the driveLayer interrupt will push it to the HW
  for(int i=0; i<4; i++) {
    bitWrite(rgb[i][layer][column],row,bitRead(b,3-i));
    bitWrite(rgb[i][layer][8+column],row,bitRead(g,3-i));
    bitWrite(rgb[i][layer][16+column],row,bitRead(r,3-i));
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

void writeRGBValues(byte l, byte BAM) {
  for (int i=0; i<24; i++) {
    SPI.transfer(rgb[BAM][l][i]);
  }
} 

void loop() {
  //doRandom();
  RGBGlow();
  delay(1000);
}


// ANIMATION CODE

void doRandom() {
  for (int i=0; i<256; i++) {
    LED(random(8), random(8), random(8), random(16), random(16), random(16));
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

void filllayer(byte level, byte r, byte g, byte b) {
  for (byte row=0;row<8;row++) {
      for (byte column=0;column<8;column++) {
          LED (row,column,level,r,g,b);
      }
  }
}

void fillrow(byte row, byte r, byte g, byte b) {
  for (byte level=0;level<8;level++) {
      for (byte column=0;column<8;column++) {
          LED (row,column,level,r,g,b);
      }
  }
}

void fillcolumn(byte column, byte r, byte g, byte b) {
    for (byte level=0;level<8;level++) {
      for (byte row=0;row<8;row++) {
          LED (row,column,level,r,g,b);
      }
  }
}


void all(byte r, byte g, byte b) {
  for (byte level=0;level<8;level++) {
    filllayer(level,r,g,b);
  }
}

void glow(int color) {
    byte value;
    for (int i=0;i<32;i++) {
      value = i;
      if (i>15) {
        value = 31 - i;
      }
      for (int r=0; r<8; r++) {
        for (int c=0; c<8; c++) {
          for (int l=0; l<8; l++) {          
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
         }
      }
    }
    delay(50);        
  }
}
