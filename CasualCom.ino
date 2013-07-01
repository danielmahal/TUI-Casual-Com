//#define DisplaySerial Serial

#include <SoftwareSerial.h>
SoftwareSerial DisplaySerial(10,11);

#include <Picaso_Const4D.h>
#include <Picaso_Serial_4DLib.h>


// Pins
const int sdPin = 2;
const int microswitchPin = 3;
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
char peopleNames[][30] = { "WONG", "DANIEL", "TAKESHI", "RITIKA", "LUKE" };

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
  
  clearScreen();
  
  while(!Display.file_Mount()) {
    Display.putstr("Not mounted");
    delay(300);
    Display.gfx_Cls();
    delay(100);
  }
  
  Display.putstr("Mounted");
  
  delay(500);
  
  font1 = Display.file_LoadImageControl("NoName1.da1", "NoName1.gc1", 1);
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

void drawList() {
  char* name = peopleNames[scrollIndex];
  
  Display.txt_FontID(font1);
  Display.txt_Width(2);
  Display.txt_Height(2);
  
  int textWidth = Display.charwidth(name[0]) * strlen(name);
  int textHeight = Display.charheight(name[0]);
  
  Display.gfx_MoveTo((width / 2) - (textWidth / 2), (height / 2) - (textHeight / 2));
  Display.txt_FGcolour(WHITE);
  Display.putstr(name);
}

void drawIdle() {
  int num = (sizeof(people) / sizeof(int));
  char string[15];
  sprintf(string, "%d people", num);
  
  Display.txt_FontID(font1);
  Display.txt_Width(2);
  Display.txt_Height(2);
  
  int textWidth = Display.charwidth('A') * strlen(string);
  int textHeight = Display.charheight('A');
  
  Display.gfx_MoveTo((width / 2) - (textWidth / 2), (height / 2) - (textHeight / 2));
  Display.txt_FGcolour(WHITE);
  
  Display.putstr(string);
}

void loop() {
  boolean prevPower = power;
  boolean forceDraw = false;
  
  boolean prevSdcard = sdcard;
  sdcard = digitalRead(sdPin);
  
  if(sdcard != prevSdcard && sdcard) {
    Serial.println("SD card change");
    power = !power;
  }
  
  if(power != prevPower || initial) {
    if(!power) {
      clearScreen();
    } else {
      forceDraw = true;
      drawTimezones();
    }
  }
  
  if(!power) {
    return;
  }

  int prevScrollIndex = scrollIndex;
  
  boolean microswitchRead = digitalRead(microswitchPin);
  
  encoder0Read = digitalRead(encoder0PinA);
  encoder1Read = digitalRead(encoder1PinA);
  
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
    scrollTimeout = millis() + 2000;
  }
  
  if(scrollTimeout < millis()) {
    scrolling = false;
  }
  
  // Check voice
  boolean voiceChange = false;
  voice = false;
  
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
