/*
  aMazeing Alarm Clock - An alarm clock with a maze minigame
  Copyright (c) 2016 NoProblem01 Group suphanath.k@ku.th ALL RIGHTS RESERVED

  based on  RealTimeClockDS1307 Library 
    Version 0.95
    Copyright (c) 2011 David H. Brown. All rights reserved

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

#include <Wire.h>
#include <RealTimeClockDS1307.h>
#include <pt.h>

#define ROW 8
#define COL 8
#define DISPLAY_INTERVAL 1.4*
#define TRIGGER_VALUE 350

#define DS1307_I2C_ADDRESS 0x68 // the I2C address of Tiny RTC
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
return ( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
return ( (val/16*10) + (val%16) );
}

#define PT_DELAY(pt,ms,ts) \
  ts = millis(); \
  PT_WAIT_WHILE(pt,millis()-ts<(ms));

struct pt pt_taskControl;
struct pt pt_taskDisplay;
struct pt pt_taskAlarm;

int map1[ROW][COL];
int maxTravel = 0;
int stopRow = 0;
int stopCol = 0;
int startRow = 0;
int startCol = 0;
int presentRow = 0;
int presentCol = 0;
int controlX = 0;
int controlY = 0;
int buttonValue = 1000;
int lastMove = -1;
int soundAlarm = 0;
int select = 0;
int hold = 0;
int countup = 1;

int hours = 12; //Alarm Hour
int minutes = 0; //Alarm Minute

bool paused = false;
bool clockMode = false;
 
bool changed = false;
int count = 0;

PT_THREAD(taskControl(struct pt* pt))
{
  static uint32_t ts;

  PT_BEGIN(pt);

  while (1)
  {
    controlX = analogRead(PIN_PC0) - 512;
    controlY = analogRead(PIN_PC1) - 512;
    buttonValue = analogRead(PIN_PC2);
//    if(buttonValue < 125 && !changed) {
//      changed = true;
//      clockMode = !clockMode;
//    }
//    else if(buttonValue > 500 && changed) {
//      changed = false;
//    }
    if(!clockMode) {
      if(controlX > TRIGGER_VALUE && lastMove == -1) {
        updatePlayer(0);
        lastMove = 0;
        PT_DELAY(pt, (int)(DISPLAY_INTERVAL * 1000), ts);
      }
      else if(controlX < -TRIGGER_VALUE && lastMove == -1) {
        updatePlayer(1);
        lastMove = 1;
        PT_DELAY(pt, (int)(DISPLAY_INTERVAL * 1000), ts);
      }
      else if(controlY > TRIGGER_VALUE && lastMove == -1) {
        updatePlayer(2);
        lastMove = 2;
        PT_DELAY(pt, (int)(DISPLAY_INTERVAL * 1000), ts);
      }
      else if(controlY < -TRIGGER_VALUE && lastMove == -1) {
        updatePlayer(3);
        lastMove = 3;
        PT_DELAY(pt, (int)(DISPLAY_INTERVAL * 1000), ts);
      }
      else {
        //updatePlayer(-1);
        lastMove = -1;
        PT_DELAY(pt,100, ts);
      }
    }
    else {
      if(controlY > TRIGGER_VALUE) {
        paused = true;
        if(minutes < 60) {
          hold = hold + 1;
          minutes = minutes + countup;
          if(minutes > 59) {
            minutes = 0;
            hours = hours + 1;
            if(hours > 23){
              hours = 0;
            }
          }
        }
        PT_DELAY(pt, 1000, ts);
      }
      else if(controlY < -TRIGGER_VALUE) {
        paused = true;
        if(minutes > -1) {
          hold = hold + 1;
          minutes = minutes - countup;
          if(minutes < 0) {
            minutes = 59;
            hours = hours - 1;
            if(hours < 0){
              hours = 23;
            }
          }
        }
        PT_DELAY(pt, (int)(DISPLAY_INTERVAL * 1000), ts);
      }
      else {
        hold = 0;
        paused = false;
        PT_DELAY(pt, 200, ts);
      }
      if (hold >= 50) {
        //countup = 20;
      }
      else {
        countup = 5;
      }
    }
  }

  PT_END(pt);
}

PT_THREAD(taskDisplay(struct pt* pt))
{
  static uint32_t ts;

  PT_BEGIN(pt);

  while (1)
  {
    if(clockMode){
      getClock();
      if(!paused) {
        PT_DELAY(pt, (int)(DISPLAY_INTERVAL * 4000), ts);
      }
      else {
        PT_DELAY(pt, (int)(DISPLAY_INTERVAL * 3000), ts);
      }
    }
    else {
      soundAlarm = 2;
      Serial.print('M');
      for(int i = 0; i < ROW; i++){      
        for(int j=0; j < COL; j++){
            if(map1[i][j] == 'O'){
              Serial.print('X');
            }
            else if(map1[i][j] == 'A'){
              Serial.print('A');
            }
            else if(map1[i][j] == 'B'){
              Serial.print('F');
            }
            else {
              Serial.print('.');
            }
        }
      }
      Serial.println();
      PT_DELAY(pt, (int)(DISPLAY_INTERVAL * 1000), ts);
    }
  }
  PT_END(pt);
}


PT_THREAD(taskAlarm(struct pt* pt))
{
  static uint32_t ts;

  PT_BEGIN(pt);

  while (1)
  {
    if(soundAlarm == 0) {
      digitalWrite(PIN_PD5, LOW);
    }
    else if(soundAlarm == 1) {
      digitalWrite(PIN_PD5, HIGH);
    }
    else if(soundAlarm == 2) {
      clockMode = false;
      digitalWrite(PIN_PD5, HIGH);
      PT_DELAY(pt, 200, ts);
      digitalWrite(PIN_PD5, LOW);
      PT_DELAY(pt, 500, ts);
      digitalWrite(PIN_PD5, HIGH);
      PT_DELAY(pt, 200, ts);
      digitalWrite(PIN_PD5, LOW);
      PT_DELAY(pt, 500, ts);
      digitalWrite(PIN_PD5, HIGH);
      PT_DELAY(pt, 200, ts);
      digitalWrite(PIN_PD5, LOW);
      PT_DELAY(pt, 2000, ts);
      digitalWrite(PIN_PD5, HIGH);
      PT_DELAY(pt, 200, ts);
      digitalWrite(PIN_PD5, LOW);
      PT_DELAY(pt, 500, ts);
      digitalWrite(PIN_PD5, HIGH);
      PT_DELAY(pt, 200, ts);
      digitalWrite(PIN_PD5, LOW);
      PT_DELAY(pt, 500, ts);
      digitalWrite(PIN_PD5, HIGH);
      PT_DELAY(pt, 200, ts);
      digitalWrite(PIN_PD5, LOW);
      PT_DELAY(pt, 4000, ts);
      digitalWrite(PIN_PD5, HIGH);
      PT_DELAY(pt, 3000, ts);
      digitalWrite(PIN_PD5, LOW);
    }
    else if(soundAlarm == 3) {
      clockMode = true;
      digitalWrite(PIN_PD5, HIGH);
      PT_DELAY(pt, 500, ts);
      digitalWrite(PIN_PD5, LOW);
      soundAlarm = 0;
    }
    PT_DELAY(pt, DISPLAY_INTERVAL * 1000, ts);
  }

  PT_END(pt);
}

void createMaze()
{
    for(int i = 0;i < ROW;i++){
        for(int j=0;j < COL;j++){
            map1[i][j] = 'O';
        }
    };
    genMaze();
    if(startRow == stopRow && startCol == stopCol){
      createMaze();
    } else {
      return;
    }
}

void genMaze()
{
    startRow = randomInRange(0,ROW - 1);
    startCol = randomInRange(0,COL - 1);
    map1[startRow][startCol] = 'A';
    gen(startRow,startCol,0);
    map1[stopRow][stopCol] = 'B';
    presentRow = startRow;
    presentCol = startCol;
}

void gen(int pRow, int pCol, int distance) {
    if(map1[pRow][pCol] != 'A') {
        map1[pRow][pCol] = '.';
    }
    int count = 4;
    int check[4] = {0,0,0,0};
    while(count > 0) {
        int val = randomInRange(1,4);
        if(checkCanGo(pRow + 2, pCol) && val == 4 && !check[0]) {
            check[0] = 1;
            map1[pRow + 1][pCol] = '.';
            gen(pRow + 2, pCol, distance + 2);
        }
        if(checkCanGo(pRow, pCol + 2) && val == 1 && !check[1]) {
            check[1] = 1;
            map1[pRow][pCol + 1] = '.';
            gen(pRow,pCol + 2, distance + 2);
        }
        if(checkCanGo(pRow - 2,pCol) && val == 2 && !check[2]) {
            check[2] = 1;
            map1[pRow - 1][pCol] = '.';
            gen(pRow - 2,pCol, distance + 2);
        }
        if(checkCanGo(pRow, pCol - 2) && val == 3 && !check[3]) {
            check[3] = 1;
            map1[pRow][pCol - 1] = '.';
            gen(pRow,pCol - 2, distance + 2);
        }
        count--;
    }
    if(distance > maxTravel) {
            maxTravel = distance;
            stopRow = pRow;
            stopCol = pCol;
            
    }
}

int checkCanGo(int r,int c) {
    if(r < 0 || r >= ROW || c < 0 || c >= COL) {
        return 0;
    }
    if(map1[r][c] == '.') {
        return 0;
    }
    return 1;
}

void updatePlayer(int dir) {
  if(dir == 0 && presentRow > 0 && map1[presentRow - 1][presentCol] != 'O' ){
    map1[presentRow - 1][presentCol] = 'A';
    map1[presentRow][presentCol] = '.';
    presentRow --;
  }
  if(dir == 1 && presentRow < ROW - 1 && map1[presentRow + 1][presentCol] != 'O'){
    map1[presentRow + 1][presentCol] = 'A';
    map1[presentRow][presentCol] = '.';
    presentRow ++;
  }
  if(dir == 2 && presentCol <  COL -1 && map1[presentRow][presentCol  + 1] != 'O'){
    map1[presentRow][presentCol + 1] = 'A';
    map1[presentRow][presentCol] = '.';
    presentCol ++;
  }
  if(dir == 3 && presentCol > 0 && map1[presentRow ][presentCol -  1] != 'O'){
    map1[presentRow][presentCol - 1] = 'A';
    map1[presentRow][presentCol] = '.';
    presentCol --;
  }
  if(presentRow == stopRow && presentCol == stopCol){
    soundAlarm = 3;
    clockMode = true;
  } else {
    soundAlarm = 0;
  }
}


int randomInRange(int min, int max) {
    return random(min,max+1);
}


/* TIME */
/////////////////////////////////////////////////////////////////////////////////

void initTimer() {
  RTC.start();
  RTC.readClock();
  long seed = (int)(RTC.getMinutes() * analogRead(0) * analogRead(PIN_PC3) * analogRead(PIN_PC4) * PI) % 1024;
  randomSeed(seed);
  setDate();
}

void setDate() {
  second = 50;
  minute = 33;
  hour = 16;
  dayOfWeek = 5;
  dayOfMonth = 23;
  month = 12;
  year = 16;
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(decToBcd(0));
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

void getClock() {
  if(paused) {
    Serial.print('S');
    if(hours < 10) {
      Serial.print('0');
    }
    Serial.print(hours);
    if(minutes < 10) {
      Serial.print('0');
    }
    Serial.println(minutes);
    return;
  }
  RTC.readClock();
  Serial.print('C');
  int firstNumber = RTC.getHours();
  int secondNumber = RTC.getMinutes();
  //int firstNumber = analogRead(PIN_PC2);
  //int secondNumber = 0;
  if(firstNumber < 10) {
    Serial.print('0');
  }
  Serial.print(firstNumber);
  if(secondNumber < 10) {
    Serial.print('0');
  }
  Serial.print(secondNumber);
  Serial.println();
  if(firstNumber == hours && secondNumber >= minutes && secondNumber < minutes + 30) {
    soundAlarm = 2;
  }
  else if(minutes + 30 >= 60 && firstNumber == hours + 1 && secondNumber <= (minutes + 30) % 60) {
    soundAlarm = 2;
  }
  else if(soundAlarm == 2){
    soundAlarm = 0;
  }
}

//////////////////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(9600);
  initTimer();
  createMaze();
  delay(2000);
  PT_INIT(&pt_taskControl);
  PT_INIT(&pt_taskDisplay);
  PT_INIT(&pt_taskAlarm);
}

void loop()
{
  taskControl(&pt_taskControl);
  taskDisplay(&pt_taskDisplay);
  taskAlarm(&pt_taskAlarm);
}
