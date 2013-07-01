//#define DisplaySerial Serial

#include <SoftwareSerial.h>
SoftwareSerial DisplaySerial(10,11);

#include <Picaso_Const4D.h>
#include <Picaso_Serial_4DLib.h>


// Pins
const unsigned int potPin = A0;

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

boolean scrolling;
int scrollIndex;
unsigned long scrollTimeout;

boolean voice;
int voiceIndex;

boolean idle = false;

int potValue;
int prevPotValue;



Picaso_Serial_4DLib Display(&DisplaySerial);

void setup() {
  Serial.begin(9600);
  
  pinMode(13, OUTPUT);
  pinMode(potPin, INPUT);
  
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
  
  clearScreen();
  drawTimezones();
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
  // Check scrolling
  int prevScrollIndex = scrollIndex;
  
  int potValue = analogRead(potPin);
  int potDiff = potValue - prevPotValue;
  
  if(abs(potDiff) > 40) {
    scrollIndex = wrapIndex(scrollIndex + (potDiff < 0 ? -1 : 1), sizeof(people) / sizeof(int));
    prevPotValue = potValue;
  }
  
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
  if(scrolling && scrollChange) {
    Serial.println("Draw list");
    clearCenter();
    drawList();
    setTimezone(people[prevScrollIndex], false);
    setTimezone(people[scrollIndex], true);
  }
 
  if(voice && voiceChange) {
    Serial.println("Draw voice");
    clearCenter();
  }
   
  if(idle && idleChange) {
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
