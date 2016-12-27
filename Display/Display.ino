/*
  aMazeing Alarm Clock - An alarm clock with a maze minigame
  Copyright (c) 2016 NoProblem01 Group suphanath.k@ku.th ALL RIGHTS RESERVED

  based on  Color cycling plasma 
    Version 0.1 - 8 July 2009
    Copyright (c) 2009 Ben Combee.  All right reserved.
    Copyright (c) 2009 Ken Corey.  All right reserved.
    Copyright (c) 2008 Windell H. Oskay.  All right reserved.
    Copyright (c) 2011 Sam C. Lin All Rights Reserved
  and ColorduinoPlasma
    Copyright (c) 2011 Sam C. Lin All Rights Reserved

  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Colorduino.h>

typedef struct
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} ColorRGB;

//a color with 3 components: h, s and v
typedef struct 
{
  unsigned char h;
  unsigned char s;
  unsigned char v;
} ColorHSV;

unsigned char plasma[ColorduinoScreenWidth][ColorduinoScreenHeight];
unsigned int bitDisplay[ColorduinoScreenWidth] [ColorduinoScreenHeight];
unsigned int clockDisplay[34] [ColorduinoScreenHeight];
long paletteShift;
bool blinking;
bool player;
bool showClock;
String lastInput = "xxxx";

int loopCount;
int clockLoop;

unsigned int sevenSeg[10][7] = {
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
  {0, 1, 1, 0, 0, 1, 1}, // 4
  {1, 0, 1, 1, 0, 1, 1}, // 5
  {1, 0, 1, 1, 1, 1, 1}, // 6
  {1, 1, 1, 0, 0, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {1, 1, 1, 1, 0, 1, 1}  // 9
};

void HSVtoRGB(void *vRGB, void *vHSV) 
{
  float r, g, b, h, s, v;
  float f, p, q, t;
  int i;
  ColorRGB *colorRGB=(ColorRGB *)vRGB;
  ColorHSV *colorHSV=(ColorHSV *)vHSV;

  h = (float)(colorHSV->h / 256.0);
  s = (float)(colorHSV->s / 256.0);
  v = (float)(colorHSV->v / 256.0);
  
  if(s == 0.0) {
    b = v;
    g = b;
    r = g;
  }
  
  else
  {
    h *= 6.0;
    i = (int)(floor(h));
    f = h - i;

    p = (float)(v * (1.0 - s));
    q = (float)(v * (1.0 - (s * f)));
    t = (float)(v * (1.0 - (s * (1.0 - f))));

    switch(i)
    {
      case 0: r=v; g=t; b=p; break;
      case 1: r=q; g=v; b=p; break;
      case 2: r=p; g=v; b=t; break;
      case 3: r=p; g=q; b=v; break;
      case 4: r=t; g=p; b=v; break;
      case 5: r=v; g=p; b=q; break;
      default: r = g = b = 0; break;
    }
  }
  colorRGB->r = (int)(r * 255.0);
  colorRGB->g = (int)(g * 255.0);
  colorRGB->b = (int)(b * 255.0);
}

float
dist(float a, float b, float c, float d) 
{
  return sqrt((c-a)*(c-a)+(d-b)*(d-b));
}


void plasma_morph() {
  unsigned char x,y;
  float value;
  ColorRGB colorRGB;
  ColorHSV colorHSV;

  for(y = 0; y < ColorduinoScreenHeight; y++) {
    for(x = 0; x < ColorduinoScreenWidth; x++) {
      {
        value = sin(dist(x + paletteShift, y, 128.0, 128.0) / 8.0)
        + sin(dist(x, y, 64.0, 64.0) / 8.0)
        + sin(dist(x, y + paletteShift / 7, 192.0, 64) / 7.0)
        + sin(dist(x, y, 192.0, 100.0) / 8.0);
        colorHSV.h=(unsigned char)((value) * 128)&0xff;
        colorHSV.s=255; 
        colorHSV.v=255;
        HSVtoRGB(&colorRGB, &colorHSV);
        if(bitDisplay[x][y] == 1) {
          Colorduino.SetPixel(x, y, colorRGB.r , colorRGB.g, colorRGB.b);
        }
        //if(bitDisplay[x][y] == 1) {
        //  Colorduino.SetPixel(x, y, 0, 0, 255);
        //}
        else if(bitDisplay[x][y] == 2){
          Colorduino.SetPixel(x, y, 255, 0, 0);
        }
        else if(bitDisplay[x][y] == 3){
          Colorduino.SetPixel(x, y, 0, 255, 0);
        }
        else {
          Colorduino.SetPixel(x, y, 0, 0, 0);
        }
      }
    }
    paletteShift++;
  }
  Colorduino.FlipPage(); // swap screen buffers to show it
}

void ColorFill(unsigned char R,unsigned char G,unsigned char B)
{
  PixelRGB *p = Colorduino.GetPixel(0,0);
  for (unsigned char y=0;y<ColorduinoScreenWidth;y++) {
    for(unsigned char x=0;x<ColorduinoScreenHeight;x++) {
      p->r = R;
      p->g = G;
      p->b = B;
      p++;
    }
  }
  
  Colorduino.FlipPage();
}

void read_command() {
  if(Serial.available()) {
    delay(10);
    String recieveString = "";
    recieveString = Serial.readString();
    Serial.println(recieveString);
    if(recieveString.compareTo(lastInput) == 0) {
      return;
    }
    if((recieveString[0] >= '0' && recieveString[0] <= '7') && recieveString.length() >= 9) { // MAZE MODE
      lastInput = recieveString;
      if(showClock) {
        clockLoop = 0;
        clearBitDisplay();
      }
      showClock = false;
      int scanning = 1;
      int x = 0;
      int y = 7- (recieveString[0] - '0');
      while(recieveString[scanning] != '\0') {
        if(recieveString[scanning] == 'X') {
          bitDisplay[x][y] = 1;
        }
        else if(recieveString[scanning] == '.') {
          bitDisplay[x][y] = 0;
        }
        if(recieveString[scanning] == 'A') {
          bitDisplay[x][y] = 2;
        }
        if(recieveString[scanning] == 'F') {
          bitDisplay[x][y] = 3;
        }
        x = x + 1;
        if(x == ColorduinoScreenWidth) {
          x = 0;
        }
        scanning = scanning + 1;
      }
    }
    else if((recieveString[0] == 'M' || recieveString[0] == 'm') && recieveString.length() >= 60) {
      lastInput = recieveString;
      if(showClock) {
        clearBitDisplay();
      }
      showClock = false;
      int scanning = 1;
      int x = 0;
      int y = 7;
      while(recieveString[scanning] != '\0') {
        if(y < 0) {
          break;
        }
        if(recieveString[scanning] == 'X') {
          bitDisplay[x][y] = 1;
        }
        else if(recieveString[scanning] == '.') {
          bitDisplay[x][y] = 0;
        }
        if(recieveString[scanning] == 'A') {
          bitDisplay[x][y] = 2;
        }
        if(recieveString[scanning] == 'F') {
          bitDisplay[x][y] = 3;
        }
        x = x + 1;
        if(x == ColorduinoScreenWidth) {
          x = 0;
          y = y - 1;
        }
        scanning = scanning + 1;
      }
    }
    else if((recieveString[0] == 'S' || recieveString[0] == 's')&& recieveString.length() >= 5) { // Show Alarm Time
      lastInput = recieveString;
      clearBitDisplay();
      setClockDisplay(0, (recieveString[1] - '0'));
      setClockDisplay(1, (recieveString[2] - '0'));
      setClockDisplay(2, (recieveString[3] - '0'));
      setClockDisplay(3, (recieveString[4] - '0'));
      clockLoop = 8;
      showClockDisplay();
      showClock = true;
    }
    else if((recieveString[0] == 'C' || recieveString[0] == 'c')&& recieveString.length() >= 5) { // CLOCK MODE
      lastInput = recieveString;
      clearBitDisplay();
      setClockDisplay(0, (recieveString[1] - '0'));
      setClockDisplay(1, (recieveString[2] - '0'));
      setClockDisplay(2, (recieveString[3] - '0'));
      setClockDisplay(3, (recieveString[4] - '0'));
      showClockDisplay();
      showClock = true;
    }
    else if((recieveString[0] == 'R' || recieveString[0] == 'r') && recieveString.length() == 1) { // CLEAR SCREEN
      lastInput = recieveString;
      clearBitDisplay();
    }
  }
}

void setClockDisplay(int digit, int value) {
  if(digit == 0 || digit == 1) {
    clockDisplay[0 + (4 * digit) + 8][5] = sevenSeg[value][0] + sevenSeg[value][5];
    clockDisplay[0 + (4 * digit) + 8][4] = sevenSeg[value][5];
    clockDisplay[0 + (4 * digit) + 8][3] = sevenSeg[value][4] + sevenSeg[value][5] + sevenSeg[value][6];
    clockDisplay[0 + (4 * digit) + 8][2] = sevenSeg[value][4];
    clockDisplay[0 + (4 * digit) + 8][1] = sevenSeg[value][3] + sevenSeg[value][4];
    clockDisplay[1 + (4 * digit) + 8][1] = sevenSeg[value][3];
    clockDisplay[2 + (4 * digit) + 8][1] = sevenSeg[value][2] + sevenSeg[value][3];
    clockDisplay[2 + (4 * digit) + 8][2] = sevenSeg[value][2];
    clockDisplay[2 + (4 * digit) + 8][3] = sevenSeg[value][1] + sevenSeg[value][2] + sevenSeg[value][6];
    clockDisplay[2 + (4 * digit) + 8][4] = sevenSeg[value][1];
    clockDisplay[2 + (4 * digit) + 8][5] = sevenSeg[value][0] + sevenSeg[value][1];
    clockDisplay[1 + (4 * digit) + 8][5] = sevenSeg[value][0];
    clockDisplay[1 + (4 * digit) + 8][3] = sevenSeg[value][6];
  }
  else if (digit == 2 || digit == 3) {
    clockDisplay[10 + (4 * (digit - 2)) + 8][5] = sevenSeg[value][0] + sevenSeg[value][5];
    clockDisplay[10 + (4 * (digit - 2)) + 8][4] = sevenSeg[value][5];
    clockDisplay[10 + (4 * (digit - 2)) + 8][3] = sevenSeg[value][4] + sevenSeg[value][5] + sevenSeg[value][6];
    clockDisplay[10 + (4 * (digit - 2)) + 8][2] = sevenSeg[value][4];
    clockDisplay[10 + (4 * (digit - 2)) + 8][1] = sevenSeg[value][3] + sevenSeg[value][4];
    clockDisplay[11 + (4 * (digit - 2)) + 8][1] = sevenSeg[value][3];
    clockDisplay[12 + (4 * (digit - 2)) + 8][1] = sevenSeg[value][2] + sevenSeg[value][3];
    clockDisplay[12 + (4 * (digit - 2)) + 8][2] = sevenSeg[value][2];
    clockDisplay[12 + (4 * (digit - 2)) + 8][3] = sevenSeg[value][1] + sevenSeg[value][2] + sevenSeg[value][6];
    clockDisplay[12 + (4 * (digit - 2)) + 8][4] = sevenSeg[value][1];
    clockDisplay[12 + (4 * (digit - 2)) + 8][5] = sevenSeg[value][0] + sevenSeg[value][1];
    clockDisplay[11 + (4 * (digit - 2)) + 8][5] = sevenSeg[value][0];
    clockDisplay[11 + (4 * (digit - 2)) + 8][3] = sevenSeg[value][6];
  }
}

void clearBitDisplay() {
  for(int y = 0; y < ColorduinoScreenHeight; y = y + 1) {
    for(int x = 0; x < ColorduinoScreenWidth; x = x + 1) {
      bitDisplay[x][y] = 0;
    }
  }
  for(int y = 0; y < ColorduinoScreenHeight; y = y + 1) {
    for(int x = 0; x < 20; x = x + 1) {
      if(x == 16 && (y == 4 || y == 2)) {
        clockDisplay[x][y] = 1;
      }
      else {
        clockDisplay[x][y] = 0;
      }
    }
  }
  ColorFill(0, 0, 0);
  blinking = false;
  player = false;
  showClock = false;
  loopCount = 0;
}

void showClockDisplay() {
  for(int y = 0; y < ColorduinoScreenHeight; y = y + 1) {
    for(int x = 0; x < ColorduinoScreenWidth; x = x + 1) {
      if(clockDisplay[x + clockLoop][y] == 0) {
        bitDisplay[x][y] = 0;
      }
      else if(clockDisplay[x + clockLoop][y] > 0) {
        bitDisplay[x][y] = 1;
      }
    }
  }
}

void setup()
{
  Colorduino.Init();
  Serial.begin(9600);
  clearBitDisplay();
  
  unsigned char whiteBalVal[3] = {36,63,63};
  Colorduino.SetWhiteBal(whiteBalVal);
  
  paletteShift=128000;
  unsigned char bcolor;

  for(unsigned char y = 0; y < ColorduinoScreenHeight; y++)
    for(unsigned char x = 0; x < ColorduinoScreenWidth; x++)
    {
      //the plasma buffer is a sum of sines
      bcolor = (unsigned char)
      (
            128.0 + (128.0 * sin(x*8.0 / 16.0))
          + 128.0 + (128.0 * sin(y*8.0 / 16.0))
      ) / 2;
      plasma[x][y] = bcolor;
    }
}

void loop()
{
  read_command();
  if(showClock) {
    showClockDisplay();
    loopCount = loopCount + 1;
    if(loopCount % 1 == 0) {
      loopCount = 0;
      clockLoop = clockLoop + 2;
      if(clockLoop > 26){
        clockLoop = 0;
      }
    }
  }
  plasma_morph();
}
