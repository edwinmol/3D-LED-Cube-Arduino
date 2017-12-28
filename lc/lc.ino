#include <TimerOne.h>
#include <SPI.h>

//define the pin numbers
#define latch_pin 8 //can be any pin we choose
#define blank_pin 7 //can be any pin we choose

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

byte zero[9] = {0,0,0,0,0,0,0,0,0};

byte ascii[26][9] = {{0b01110000,0b10001000,0b10001000,0b11111000,0b10001000,0b10001000,0b10001000,0b00000000,5},
{0b11110000,0b10001000,0b10001000,0b11110000,0b10001000,0b10001000,0b11110000,0b00000000,5},
{0b01110000,0b10001000,0b10000000,0b10000000,0b10000000,0b10001000,0b01110000,0b00000000,5},
{0b11110000,0b10001000,0b10001000,0b10001000,0b10001000,0b10001000,0b11110000,0b00000000,5},
{0b11110000,0b10000000,0b10000000,0b11100000,0b10000000,0b10000000,0b11110000,0b00000000,4},
{0b11110000,0b10000000,0b10000000,0b11100000,0b10000000,0b10000000,0b10000000,0b00000000,4},
{0b01110000,0b10001000,0b10000000,0b10110000,0b10001000,0b10001000,0b01110000,0b00000000,5},
{0b10001000,0b10001000,0b10001000,0b11111000,0b10001000,0b10001000,0b10001000,0b00000000,5},
{0b11100000,0b01000000,0b01000000,0b01000000,0b01000000,0b01000000,0b11100000,0b00000000,3},
{0b11100000,0b00100000,0b00100000,0b00100000,0b00100000,0b00100000,0b11000000,0b00000000,3},
{0b10010000,0b10010000,0b10100000,0b11000000,0b10100000,0b10010000,0b10010000,0b00000000,4},
{0b10000000,0b10000000,0b10000000,0b10000000,0b10000000,0b10000000,0b11100000,0b00000000,3},
{0b10001000,0b11011000,0b10101000,0b10001000,0b10001000,0b10001000,0b10001000,0b00000000,5},
{0b10000100,0b11000100,0b10100100,0b10010100,0b10001100,0b10000100,0b10000100,0b00000000,6},
{0b01110000,0b10001000,0b10001000,0b10001000,0b10001000,0b10001000,0b01110000,0b00000000,5},
{0b11110000,0b10001000,0b10001000,0b11110000,0b10000000,0b10000000,0b10000000,0b00000000,5},
{0b01110000,0b10001000,0b10001000,0b10101000,0b10011000,0b10001000,0b01110100,0b00000000,6},
{0b11110000,0b10001000,0b10001000,0b11110000,0b10100000,0b10010000,0b10001000,0b00000000,5},
{0b01110000,0b10001000,0b10000000,0b01110000,0b00001000,0b10001000,0b01110000,0b00000000,5},
{0b11111000,0b00100000,0b00100000,0b00100000,0b00100000,0b00100000,0b00100000,0b00000000,5},
{0b10001000,0b10001000,0b10001000,0b10001000,0b10001000,0b10001000,0b01110000,0b00000000,5},
{0b10001000,0b10001000,0b10001000,0b01010000,0b01010000,0b00100000,0b00100000,0b00000000,5},
{0b10000010,0b10000010,0b10000010,0b01010100,0b01010100,0b00101000,0b00101000,0b00000000,7},
{0b10001000,0b10001000,0b01010000,0b00100000,0b01010000,0b10001000,0b10001000,0b00000000,5},
{0b10001000,0b10001000,0b01010000,0b00100000,0b00100000,0b00100000,0b00100000,0b00000000,5},
{0b11111000,0b00001000,0b00010000,0b00100000,0b01000000,0b10000000,0b11111000,0b00000000,5}};

int text_buffer[8] = {0,0,0,0,0,0,0,0};

/*

byte A[8] = {0b00111100,0b01000010,0b01000010,0b01000010,0b01111110,0b01000010,0b01000010,0b01000010};
byte B[8] = {0b01111100,0b01000010,0b01000010,0b01111100,0b01000010,0b01000010,0b01000010,0b01111100};
byte C[8] = {0b00111100,0b01000010,0b01000000,0b01000000,0b01000000,0b01000000,0b01000010,0b00111100};
byte D[8] = {0b01111100,0b01000010,0b01000010,0b01000010,0b01000010,0b01000010,0b01000010,0b01111100};
byte E[8] = {0b01111110,0b01000000,0b01000000,0b01111100,0b01000000,0b01000000,0b01000000,0b01111110};
byte F[8] = {0b01111110,0b01000000,0b01000000,0b01111100,0b01000000,0b01000000,0b01000000,0b01000000};
byte G[8] = {0b00111100,0b01000010,0b01000000,0b01000000,0b01001100,0b01000010,0b01000010,0b00111100};
byte H[8] = {0b01000010,0b01000010,0b01000010,0b01000010,0b01111110,0b01000010,0b01000010,0b01000010};
byte I[8] = {0b00111000,0b00010000,0b00010000,0b00010000,0b00010000,0b00010000,0b00010000,0b00111000};
byte J[8] = {0b01111000,0b00001000,0b00001000,0b00001000,0b00001000,0b00001000,0b00001000,0b01110000};
byte K[8] = {0b01000100,0b01001000,0b01010000,0b01100000,0b01100000,0b01010000,0b01001000,0b01000100};
byte L[8] = {0b01000000,0b01000000,0b01000000,0b01000000,0b01000000,0b01000000,0b01000000,0b01111100};
byte M[8] = {0b01000001,0b01100011,0b01010101,0b01001001,0b01000001,0b01000001,0b01000001,0b01000001};
byte N[8] = {0b01000001,0b01100001,0b01010001,0b01001001,0b01000101,0b01000011,0b01000001,0b01000001};
byte O[8] = {0b00111100,0b01000010,0b01000010,0b01000010,0b01000010,0b01000010,0b01000010,0b00111100};
byte P[8] = {0b01111100,0b01000010,0b01000010,0b01000010,0b01111100,0b01000000,0b01000000,0b01000000};
byte Q[8] = {0b00111100,0b01000010,0b01000010,0b01000010,0b01001010,0b01000110,0b01000010,0b00111101};
byte R[8] = {0b01111100,0b01000010,0b01000010,0b01000010,0b01111100,0b01001000,0b01000100,0b01000010};
byte S[8] = {0b00111100,0b01000010,0b01000000,0b00111100,0b00000010,0b00000010,0b01000010,0b00111100};
byte T[8] = {0b01111100,0b00010000,0b00010000,0b00010000,0b00010000,0b00010000,0b00010000,0b00010000};
byte U[8] = {0b01000010,0b01000010,0b01000010,0b01000010,0b01000010,0b01000010,0b01000010,0b00111100};
byte V[8] = {0b01000001,0b01000001,0b00100010,0b00100010,0b00010100,0b00010100,0b00001000,0b00001000};
byte W[8] = {0b01000001,0b01000001,0b01000001,0b00100010,0b00101010,0b00101010,0b00010100,0b00010100};
byte X[8] = {0b01000010,0b01000010,0b00100100,0b00011000,0b00011000,0b00100100,0b01000010,0b01000010};
byte Y[8] = {0b01000001,0b01000001,0b00100010,0b00010100,0b00001000,0b00001000,0b00001000,0b00001000};
byte Z[8] = {0b01111110,0b00000010,0b00000100,0b00001000,0b00010000,0b00100000,0b01000000,0b01111110};

*/



//These variables can be used for other things
unsigned long start;//for a millis timer to cycle through the animations

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

  // turn the output off
  digitalWrite(blank_pin, HIGH); 

  // now select layer
  selectLayer(layer);
  
  // write the RGB values
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

void printString(const char string[], byte r, byte g, byte b) {
  clear(); 
  int _cursor = 0;
  int _column = 0;
  int _length = strlen(string);
  while (_cursor<_length) {
    int a = string[_cursor]-65;
    byte  _charsize = ascii[a][8];
    shiftAndFill(ascii[a],_column,r,g,b);
    _column++;
    // check if we have more columns of the char to show
    if (_column > _charsize) {
      _cursor++; //next char      
    }
  }
  //at the end we need to shift in zeros until the last char has been shifted out (15x)
  for (int i=0;i<15;i++) {    
    shiftAndFill(zero,0,r,g,b);
  }
}

// shift everything to the left and add a new column to the right
void shiftAndFill(byte letter[], int _column, byte r, byte g, byte b) {
  
    // shift all bits
    for (int x=0;x<8;x++) {
      text_buffer[x] == text_buffer[x] << 1;
    }
    // add new column
    for (int x=0;x<8;x++) {
      //if (bitRead(letter[x],_column)==1) {
        text_buffer[x] == text_buffer[x] | 0x01;
      //}
    }
    // transfer the text_buffer to the LED_CUBE
    for (int i=0;i<7;i++) {
      setColumn(i,0,text_buffer,i,r,g,b);
    }
    for (int i=0;i<7;i++) {
      setColumn(7,i,text_buffer,i+7,r,g,b);
    }
    delay(100);
}

void setColumn(int x,int y, int letter[], int index, byte r, byte g, byte b) {
  for(int i=0; i<8; i++) {
      if (bitRead(letter[i],index)==1) {
        LED(x,y,7-i,r,g,b);
      } else {
        LED(x,y,7-i,0,0,0);
      }
  }
}

void print(byte letter[], byte r, byte g, byte b) {
  for(int x=0; x<8; x++) {
    for(int i=0; i<8; i++) {
      for (int p=0; p<8; p++) {
        if (bitRead(letter[i],p)==1) {
          LED(x,7-p,7-i,r,g,b);
        } else {
          LED(x,7-p,7-i,0,0,0);
        }
      }
    }
    delay(60);
    fillrow(x,0,0,0);
  }
}

//function to control a LED
void LED (int row, int column, int l, byte r, byte g, byte b) {
  //write to the data structure, the driveLayer interrupt will push it to the HW
  for(int i=0; i<4; i++) {
    bitWrite(rgb[i][l][column],row,bitRead(b,3-i));
    bitWrite(rgb[i][l][8+column],row,bitRead(g,3-i));
    bitWrite(rgb[i][l][16+column],row,bitRead(r,3-i));
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


  //const char *hello = "hello";
  //printString(hello,15,15,15);
 
  sinwaveTwo();
  for (int i=0;i<26;i++) {
    print(ascii[i],random(8),random(8),random(8));
  }

 /*
  print(A,random(8),random(8),random(8));
  print(B,random(8),random(8),random(8));
  print(C,random(8),random(8),random(8));
  print(D,random(8),random(8),random(8));
  print(E,random(8),random(8),random(8));
  print(F,random(8),random(8),random(8));
  print(G,random(8),random(8),random(8));
  print(H,random(8),random(8),random(8));
  print(I,random(8),random(8),random(8));
  print(J,random(8),random(8),random(8));
  print(K,random(8),random(8),random(8));
  print(L,random(8),random(8),random(8));
  print(M,random(8),random(8),random(8));
  print(N,random(8),random(8),random(8));
  print(O,random(8),random(8),random(8));
  print(P,random(8),random(8),random(8));
  print(Q,random(8),random(8),random(8));
  print(R,random(8),random(8),random(8));
  print(S,random(8),random(8),random(8));
  print(T,random(8),random(8),random(8));
  print(U,random(8),random(8),random(8));
  print(V,random(8),random(8),random(8));
  print(W,random(8),random(8),random(8));
  print(X,random(8),random(8),random(8));
  print(Y,random(8),random(8),random(8));
  print(Z,random(8),random(8),random(8));
  */
  clear();
  /*
  RGBGlow();
  clear();
  rain();
  clear();
  sinwaveTwo();  
  clear();
  bouncyvTwo();
  clear();
  color_wheelTWO();
  clear();
  cycleLayers();
  clear();
  doRandom();
  clear();
  */
  
}

void cube() {
  //grow a cube from inside out and vice versa
  
}

void rain() {
  //create some random drops in a plane
  int data[8][8][8];
  //select some random leds
  for (int z=0;z<8;z++) {    
    randomLEDs(data[z]);            
  }
  for (int i=0;i<200;i++) { //change i to vary the duration of the rainshower
    // now we shift each layer down 
    for (int z=0;z<7;z++) {    
      for (int x=0;x<8;x++) {
        for (int y=0;y<8;y++) {
          data[z][x][y] = data[z+1][x][y];
        }
      }            
    }
    //and create a new top layer each iteration
    randomLEDs(data[7]);
    //transfer the shower to the controller
    for (int z=0;z<8;z++) {    
      for (int x=0;x<8;x++) {
        for (int y=0;y<8;y++) {
          byte color = (data[z][x][y] == 1)?(8+random(8)):0;
          LED(x,y,z, 0, color, color);
        }
      }            
    }
    delay(15);
  }
}

void randomLEDs(int data[8][8]) {
    for (int x=0;x<8;x++) {
      for (int y=0;y<8;y++) {
        data[x][y] = 0;
      }
    }
    for (int led=0;led<3;led++) {
      data[random(8)][random(8)] = 1;
    }
}

// ANIMATION CODE

void cycleLayers() {
  for (int i=0;i<8;i++) {
    filllayer(i,15,15,15);
    delay(200);
  }
  
}

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
      for (int r=0; r<8; r=r+7) {
        for (int c=0; c<8; c=c+7) {
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
    for (int r=0; r<8; r++) {
        for (int c=0; c<8; c=c+7) {
          for (int l=0; l<8; l=l+7) {          
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
    for (int r=0; r<8; r=r+7) {
        for (int c=0; c<8; c++) {
          for (int l=0; l<8; l=l+7) {          
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

void bouncyvTwo(){//****bouncyTwo****bouncyTwo****bouncyTwo****bouncyTwo****bouncyTwo****bouncyTwo****bouncyTwo
  int wipex, wipey, wipez, ranr, rang, ranb, select, oldx[50], oldy[50], oldz[50];
  int x[50], y[50], z[50], addr, ledcount=20, direct, direcTwo;
  int xx[50], yy[50], zz[50];
  int xbit=1, ybit=1, zbit=1;
   for(addr=0; addr<ledcount+1; addr++){
     oldx[addr]=0;
     oldy[addr]=0;
     oldz[addr]=0;
     x[addr]=0;
     y[addr]=0;
     z[addr]=0;
     xx[addr]=0;
     yy[addr]=0;
     zz[addr]=0;
 
   }
  
      start=millis();
      
  while(millis()-start<15000){
    direct = random(3);

for(addr=1; addr<ledcount+1; addr++){
LED(oldx[addr], oldy[addr],oldz[addr], 0,0,0);
LED(x[addr], y[addr], z[addr], xx[addr],yy[addr],zz[addr]);
}

for(addr=1; addr<ledcount+1; addr++){
oldx[addr]=x[addr];
oldy[addr]=y[addr];
oldz[addr]=z[addr];
}
delay(20);


//direcTwo=random(3);  
//if(direcTwo==1)



if(direct==0)
x[0]= x[0]+xbit;
if(direct==1)
y[0]= y[0]+ybit;
if(direct==2)
z[0]= z[0]+zbit;

if(direct==3)
x[0]= x[0]-xbit;
if(direct==4)
y[0]= y[0]-ybit;
if(direct==5)
z[0]= z[0]-zbit;





if(x[0]>7){
xbit=-1;
x[0]=7;
xx[0]=random(16);
yy[0]=random(16);
zz[0]=0;
//wipe_out();
}
if(x[0]<0){
xbit=1;
  x[0]=0;
xx[0]=random(16);
yy[0]=0;
zz[0]=random(16);
//wipe_out();
}
if(y[0]>7){
ybit=-1;
y[0]=7;
xx[0]=0;
yy[0]=random(16);
zz[0]=random(16);
//wipe_out();
}
if(y[0]<0){
ybit=1;
  y[0]=0;
  xx[0]=0;
yy[0]=random(16);
zz[0]=random(16);
//wipe_out();
}
if(z[0]>7){
zbit=-1;
z[0]=7;
xx[0]=random(16);
yy[0]=0;
zz[0]=random(16);
//wipe_out();
}
if(z[0]<0){
zbit=1;
  z[0]=0;
  xx[0]=random(16);
yy[0]=random(16);
zz[0]=0;
//wipe_out();
}

for(addr=ledcount; addr>0; addr--){
  x[addr]=x[addr-1];
  y[addr]=y[addr-1];
  z[addr]=z[addr-1];
  xx[addr]=xx[addr-1];
  yy[addr]=yy[addr-1];
  zz[addr]=zz[addr-1];
}


  }//while
  

  
  
  
}//bouncyv2

void sinwaveTwo(){//*****sinewaveTwo*****sinewaveTwo*****sinewaveTwo*****sinewaveTwo*****sinewaveTwo*****sinewaveTwo
    int sinewavearray[8], addr, sinemult[8], colselect, rr=0, gg=0, bb=15, addrt;
  int sinewavearrayOLD[8], select, subZ=-7, subT=7, multi=0;//random(-1, 2);
  sinewavearray[0]=0;
  sinemult[0]=1;
   sinewavearray[1]=1;
  sinemult[1]=1; 
    sinewavearray[2]=2;
  sinemult[2]=1;
    sinewavearray[3]=3;
  sinemult[3]=1;
    sinewavearray[4]=4;
  sinemult[4]=1;
    sinewavearray[5]=5;
  sinemult[5]=1;
    sinewavearray[6]=6;
  sinemult[6]=1;
    sinewavearray[7]=7;
  sinemult[7]=1;
  
      start=millis();
      
  while(millis()-start<15000){
  for(addr=0; addr<8; addr++){
    if(sinewavearray[addr]==7){
    sinemult[addr]=-1;
    }
    if(sinewavearray[addr]==0){
    sinemult[addr]=1;     
    }
    sinewavearray[addr] = sinewavearray[addr] + sinemult[addr];
}//addr
     if(sinewavearray[0]==7){
     select=random(3);
    if(select==0){
      rr=random(1, 16);
      gg=random(1, 16);
      bb=0;} 
     if(select==1){
      rr=random(1, 16);
      gg=0;
      bb=random(1, 16);}    
     if(select==2){
      rr=0;
      gg=random(1, 16);
      bb=random(1, 16);}
   /*
 if(multi==1)
 multi=0;
 else
 multi=1;
*/

 }    
   


      for(addr=0; addr<8; addr++){
    LED(sinewavearrayOLD[addr], addr, 0, 0, 0, 0);
    LED(sinewavearrayOLD[addr], 0, addr, 0, 0, 0);
    LED(sinewavearrayOLD[addr], subT-addr, 7, 0, 0, 0);
    LED(sinewavearrayOLD[addr], 7, subT-addr, 0, 0, 0);     
   LED(sinewavearray[addr], addr, 0, rr, gg, bb);
   LED(sinewavearray[addr], 0, addr, rr, gg, bb);
   LED(sinewavearray[addr], subT-addr,7, rr, gg, bb);
   LED(sinewavearray[addr], 7, subT-addr, rr, gg, bb);
    }//}
    
       for(addr=1; addr<7; addr++){   
    LED(sinewavearrayOLD[addr+multi*1], addr, 1, 0, 0, 0);
    LED(sinewavearrayOLD[addr+multi*1], 1, addr, 0, 0, 0);
    LED(sinewavearrayOLD[addr+multi*1], subT-addr, 6, 0, 0, 0);
    LED(sinewavearrayOLD[addr+multi*1], 6, subT-addr, 0, 0, 0);  
   LED(sinewavearray[addr+multi*1], addr, 1, rr, gg, bb);
   LED(sinewavearray[addr+multi*1], 1, addr, rr, gg, bb);
   LED(sinewavearray[addr+multi*1], subT-addr,6, rr, gg, bb);
   LED(sinewavearray[addr+multi*1], 6, subT-addr, rr, gg, bb);
       }
 
        for(addr=2; addr<6; addr++){   
    LED(sinewavearrayOLD[addr+multi*2], addr, 2, 0, 0, 0);
    LED(sinewavearrayOLD[addr+multi*2], 2, addr, 0, 0, 0);
    LED(sinewavearrayOLD[addr+multi*2], subT-addr, 5, 0, 0, 0);
    LED(sinewavearrayOLD[addr+multi*2], 5, subT-addr, 0, 0, 0);  
   LED(sinewavearray[addr+multi*2], addr, 2, rr, gg, bb);
   LED(sinewavearray[addr+multi*2], 2, addr, rr, gg, bb);
   LED(sinewavearray[addr+multi*2], subT-addr,5, rr, gg, bb);
   LED(sinewavearray[addr+multi*2], 5, subT-addr, rr, gg, bb);
       }  
             for(addr=3; addr<5; addr++){   
    LED(sinewavearrayOLD[addr+multi*3], addr, 3, 0, 0, 0);
    LED(sinewavearrayOLD[addr+multi*3], 3, addr, 0, 0, 0);
    LED(sinewavearrayOLD[addr+multi*3], subT-addr, 4, 0, 0, 0);
    LED(sinewavearrayOLD[addr+multi*3], 4, subT-addr, 0, 0, 0);  
   LED(sinewavearray[addr+multi*3], addr, 3, rr, gg, bb);
   LED(sinewavearray[addr+multi*3], 3, addr, rr, gg, bb);
   LED(sinewavearray[addr+multi*3], subT-addr,4, rr, gg, bb);
   LED(sinewavearray[addr+multi*3], 4, subT-addr, rr, gg, bb);
       }      
     
     for(addr=0; addr<8; addr++)
   sinewavearrayOLD[addr]=sinewavearray[addr];
    delay(30);

    
    
  }//while
  
  
}//SinewaveTwo

void color_wheel(){
  int xx, yy, zz, ww, rr=1, gg=1, bb=1, ranx, rany, swiper;
  
        start=millis();
      
  while(millis()-start<100000){
    swiper=random(3);
     ranx=random(16);
     rany=random(16);
     
    for(xx=0;xx<8;xx++){
    for(yy=0;yy<8;yy++){ 
    for(zz=0;zz<8;zz++){
      
     LED(xx, yy, zz,  ranx, 0, rany);
    }}
  delay(50);
}

     ranx=random(16);
     rany=random(16);
     
    for(xx=7;xx>=0;xx--){ 
    for(yy=0;yy<8;yy++){
    for(zz=0;zz<8;zz++){
    LED(xx,yy, zz, ranx, rany, 0);
    }}
  delay(50); 
  }
       ranx=random(16);
     rany=random(16);
    for(xx=0;xx<8;xx++){ 
    for(yy=0;yy<8;yy++){
    for(zz=0;zz<8;zz++){
      LED(xx,yy, zz, 0, ranx, rany);
    }}
    delay(50);
  }
    
     ranx=random(16);
     rany=random(16);
    for(xx=7;xx>=0;xx--){ 
    for(yy=0;yy<8;yy++){
    for(zz=0;zz<8;zz++){
    LED(xx,yy, zz, rany, ranx, 0);
    }}
  delay(50); 
  }
    
  }//while
    
}//color wheel

void color_wheelTWO(){//*****colorWheelTwo*****colorWheelTwo*****colorWheelTwo*****colorWheelTwo*****colorWheelTwo
  int xx, yy, zz, ww, rr=1, gg=1, bb=1, ranx, rany ,ranz, select, swiper;
  
        start=millis();
      
  while(millis()-start<10000){
    swiper=random(6);
    select=random(3);
    if(select==0){
     ranx=0;
     rany=random(16);
     ranz=random(16);}
    if(select==1){
     ranx=random(16);
     rany=0;
     ranz=random(16);}   
      if(select==2){
     ranx=random(16);
     rany=random(16);
     ranz=0;}  
    
     
    if(swiper==0){
    for(yy=0;yy<8;yy++){//left to right
    for(xx=0;xx<8;xx++){
    for(zz=0;zz<8;zz++){
    LED(xx, yy, zz,  ranx, ranz, rany);
    }}
  delay(30);
 }}
    if(swiper==1){//bot to top
    for(xx=0;xx<8;xx++){
    for(yy=0;yy<8;yy++){
    for(zz=0;zz<8;zz++){
    LED(xx, yy, zz,  ranx, ranz, rany);
    }}
  delay(30);
 }}  
    if(swiper==2){//back to front
    for(zz=0;zz<8;zz++){
    for(xx=0;xx<8;xx++){
    for(yy=0;yy<8;yy++){
    LED(xx, yy, zz,  ranx, ranz, rany);
    }}
  delay(30);
}}    
    if(swiper==3){
    for(yy=7;yy>=0;yy--){//right to left
    for(xx=0;xx<8;xx++){
    for(zz=0;zz<8;zz++){
    LED(xx, yy, zz,  ranx, ranz, rany);
    }}
  delay(30);
}}
    if(swiper==4){//top to bot
    for(xx=7;xx>=0;xx--){
    for(yy=0;yy<8;yy++){
    for(zz=0;zz<8;zz++){
    LED(xx, yy, zz,  ranx, ranz, rany);
    }}
 delay(30);
}}  
    if(swiper==5){//front to back
    for(zz=7;zz>=0;zz--){
    for(xx=0;xx<8;xx++){
    for(yy=0;yy<8;yy++){
    LED(xx, yy, zz,  ranx, ranz, rany);
    }}
  delay(30);
}}
  
  
  
  
  }//while
    
}//color wheel
