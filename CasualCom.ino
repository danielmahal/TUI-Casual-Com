//#define DisplaySerial Serial

#include <SoftwareSerial.h>
SoftwareSerial DisplaySerial(10,11);

#include <Picaso_Const4D.h>
#include <Picaso_Serial_4DLib.h>


// Pins
const int sdPin = 8;
const int microswitchPin = 9;
const int encoder0PinA = 4;
const int encoder0PinB = 5;
const int encoder1PinA = 6;
const int encoder1PinB = 7;

// Drawing variables
const unsigned int width = 320;
const unsigned int height = 240;
const unsigned int bgColor = BLACK;

int font1;
int colors[] = {0xF7DF, 0x0451, 0xFB56, 0xCAEB};

int personRadius = 5;
float personDistance = height / 2 - personRadius;

// People
int people[] = { 7, 9, 13, 18, 18 };
char peopleNames[][30] = { "Wong", "Daniel", "Takeshi", "Ritika", "Luke" };
char peoplePlaces[][30] = { "Toronto", "OSLO", "Tokyo", "Copenhagen", "London" };

// State
boolean initial = true;
boolean power = false;

boolean scrolling;
int scrollIndex;
unsigned long scrollTimeout;

boolean voice;
int voiceIndex;

boolean idle = false;

boolean sdcard;
boolean microswitch;
boolean prevMicroswitch;
long msLastDebounceTime;
long msDebounceDelay = 50;

int encoder0PinALast = LOW;
int encoder0Read = LOW;

int encoder1PinALast = LOW;
int encoder1Read = LOW;



Picaso_Serial_4DLib Display(&DisplaySerial);

void setup() {
  Serial.begin(9600);
  
  pinMode(sdPin, INPUT);
  pinMode(microswitchPin, INPUT);
  
  pinMode (encoder0PinA,INPUT);
  pinMode (encoder0PinB,INPUT);
  
  pinMode (encoder1PinA,INPUT);
  pinMode (encoder1PinB,INPUT);
   
  digitalWrite(encoder0PinA, HIGH);
  digitalWrite(encoder0PinB, HIGH);
  
  digitalWrite(encoder1PinA, HIGH);
  digitalWrite(encoder1PinB, HIGH);
  
  DisplaySerial.begin(9600);
  Display.TimeLimit4D  = 5000;
  
  Display.gfx_ScreenMode(LANDSCAPE_R);
  
  while(!Display.file_Mount()) {
    Display.putstr("Not mounted");
    delay(300);
    Display.gfx_Cls();
    delay(100);
  }
  
  Display.putstr("Mounted");
  
  delay(5000);
  clearScreen();
  
  font1 = Display.file_LoadImageControl("NoName1.da1", "NoName1.gc1", 1);
}

void drawPersonIcon(int x, int y, int r, int color) {
  float bodyDistance = 2;
  y = y + r;
  x = x + r;
  Display.gfx_CircleFilled(x, y, r / 1.5, color);
  Display.gfx_CircleFilled(x, y + r * bodyDistance, r, color);
  Display.gfx_MoveTo(x - r, y + r * bodyDistance);
  Display.gfx_RectangleFilled(x - r, y + r * bodyDistance, x + r, y + r * bodyDistance + r, color);
}

void clearCenter() {
  int r = getCenterRadius();
  Display.gfx_EllipseFilled(width / 2, height / 2, r, r, bgColor);
}

int getCenterRadius() {
  return (height / 2 - personRadius * 2) - 5;
}

void clearScreen() {
  Display.gfx_BGcolour(bgColor);
  Display.gfx_Cls();
}

void setTimezone(float time, boolean active) {
  drawTimezone(time, active ? MAGENTA : WHITE);
}

void clearTimezone(float time) {
  drawTimezone(time, bgColor);
}

void drawTimezone(float time, int color) {
  float angle = getTimezoneAngle(time);
  Display.gfx_CircleFilled(width / 2 + (sin(angle) * personDistance), height / 2 + (cos(angle) * personDistance), personRadius, color);
}

void drawTimezones() {
  for(int i = 0; i < sizeof(people) / sizeof(int); i++) {
    setTimezone(people[i], false);
  }
}

void setTextSize(int textSize) {
  Display.txt_Width(textSize);
  Display.txt_Height(textSize);
}

int getTextCenterX(char* str) {
  int textWidth = Display.charwidth(str[0]) * strlen(str);
  return width / 2 - textWidth / 2;
}

int getTextCenterY(char* str) {
  int textHeight = Display.charheight(str[0]);
  return height / 2 - textHeight / 2;
}

char* getTime(int time) {
  return "18:45";
}

void drawList() {
  char* name = peopleNames[scrollIndex];
  char* place = peoplePlaces[scrollIndex];
  
  setTextSize(3);
  
  Display.gfx_MoveTo(getTextCenterX(name), 60);
  Display.txt_FGcolour(WHITE);
  Display.putstr(name);
  
  setTextSize(2);
  Display.gfx_MoveTo(getTextCenterX(place), 110);
  Display.putstr(place);
  
  char* time = getTime(1);
  Display.gfx_MoveTo(getTextCenterX(time), 140);
  Display.putstr(time);
}

void drawVoice() {
  char str[15] = "LOOOL";
  
  setTextSize(2);
  
  Display.gfx_MoveTo(getTextCenterX(str), getTextCenterY(str));
  Display.txt_FGcolour(WHITE);
  
  Display.putstr(str);
};

void drawIdle() {
  char* time = getTime(1);
  setTextSize(2);
  Display.gfx_MoveTo(getTextCenterX(time), 85);
  Display.putstr(time);
  
  setTextSize(1);
  
  int nPeople = (sizeof(people) / sizeof(int));
  char str[15];
  sprintf(str, "%d connected", nPeople);
  int x = getTextCenterX(str) - 6;
  int y = 120;
  
  drawPersonIcon(x, y + 2, 3, GRAY);
  
  Display.gfx_MoveTo(x + 14, y);
  Display.txt_FGcolour(WHITE);
  
  Display.putstr(str);
}

void loop() {
  // Power
  boolean prevPower = power;
  boolean forceDraw = false;
  
  boolean prevSdcard = sdcard;
  sdcard = digitalRead(sdPin);
  
  if(sdcard != prevSdcard && sdcard) {
    Serial.println("SD card change");
    power = !power;
  }
  
  /*if(power != prevPower || initial) {
    if(!power) {
      clearScreen();
    } else {
      forceDraw = true;
      drawTimezones();
    }
  }
  
  if(!power) {
    return;
  }*/
  
  // Microswitch
  boolean microswitchRead = digitalRead(microswitchPin);
  
  if(microswitchRead != prevMicroswitch) {
    msLastDebounceTime = millis();
  }
  
  if ((millis() - msLastDebounceTime) > msDebounceDelay) {
    if(microswitchRead != microswitch) {
      microswitch = microswitchRead;
      
      Serial.println(microswitch ? "Microswitch pressed" : "Microswitch released");
    }
  }
  
  prevMicroswitch = microswitchRead;
  
  
  // Encoder
  int prevScrollIndex = scrollIndex;
  
  encoder0Read = digitalRead(encoder0PinA);
  encoder1Read = digitalRead(encoder1PinA);
  
  if ((encoder0PinALast == LOW) && (encoder0Read == HIGH)) {
    int dir = digitalRead(encoder0PinB) == LOW ? -1 : 1;
    scrollIndex = wrapIndex(scrollIndex + dir, sizeof(people) / sizeof(int));
  }
  
  if ((encoder1PinALast == LOW) && (encoder1Read == HIGH)) {
    if (digitalRead(encoder1PinB) == LOW) {
      Serial.println("Encoder #2 -1");
    } else {
      Serial.println("Encoder #2 +1");
    }
  }
  
  encoder0PinALast = encoder0Read;
  encoder1PinALast = encoder1Read;
  
  boolean scrollChange = prevScrollIndex != scrollIndex;
  
  if(!initial && scrollChange) {
    scrolling = true;
    scrollTimeout = millis() + 5000;
  }
  
  if(scrollTimeout < millis()) {
    scrolling = false;
  }
  
  // Check voice
  boolean voiceChange = false;
  
  while(Serial.available()) {
    int in = int(Serial.read());
    
    voiceChange = true;
    voice = in % 2 == 1;
    voiceIndex = round(in / 2);
  }
  
  // Idle
  boolean prevIdle = idle;
  idle = !scrolling && !voice;
  boolean idleChange = prevIdle != idle;
  
  // Draw screen
  if(scrolling && (scrollChange || forceDraw)) {
    Serial.println("Draw list");
    clearCenter();
    drawList();
    setTimezone(people[prevScrollIndex], false);
    setTimezone(people[scrollIndex], true);
  }
 
  if(voice && (voiceChange || forceDraw)) {
    Serial.println("Draw voice");
    clearCenter();
    drawVoice();
  }
   
  if(idle && (idleChange || forceDraw)) {
    Serial.println("Draw idle");
    clearCenter();
    drawIdle();
    setTimezone(people[scrollIndex], false);
  }

  initial = false;
}

float getTimezoneAngle(float time) {
  return -((time / 24.0) * TWO_PI) - PI;
}

int wrapIndex(int i, int imax) {
   return ((i % imax) + imax) % imax;
}
