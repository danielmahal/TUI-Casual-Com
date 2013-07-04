#include <Encoder.h>

#include <SoftwareSerial.h>
#include <Picaso_Const4D.h>
#include <Picaso_Serial_4DLib.h>

SoftwareSerial DisplaySerial(10,11);

const int rotaryThreshold = 3;
const float volumeControlSpeed = 0.1;
const int volumeDelay = 2000; // Timeout from showing volume

// Pins
const int sdPin = 8;
const int microswitchPin = 9;
Encoder positionKnob(3, 5);
Encoder volumeKnob(2, 4);

// Drawing variables
const unsigned int width = 292;
const unsigned int height = 240;
const unsigned int bgColor = BLACK;
const unsigned int accentColor = getColor(79, 227, 135);
const unsigned int volumeMinRadius = 20;
const unsigned int volumeMaxRadius = height / 2;

int franklin36;
int franklin26;

int personRadius = 5;
float personDistance = height / 2 - personRadius - 10;

// People
int people[] = { 7, 9, 13, 18, 18 };
char peopleNames[][30] = { "Wong", "Daniel", "Takeshi", "Ritika", "Luke" };
char peoplePlaces[][30] = { "Toronto", "Oslo", "Tokyo", "Copenhagen", "London" };

// State
boolean initial = true;
boolean power = false;

boolean scrolling;
int scrollIndex;
unsigned long scrollTimeout;

boolean voice;
int voiceIndex;

boolean volume;
float volumeValue = 0.7;
long volumeTimeout;
int potValue;
int prevPotValue;

boolean idle = false;

boolean sdcard = true;
boolean microswitch;
boolean prevMicroswitch;

long msLastDebounceTime;
long msDebounceDelay = 50;

int positionIndex;
int prevPositionIndex;
int volumeIndex;
int prevVolumeIndex;

Picaso_Serial_4DLib Display(&DisplaySerial);

void setup() {
    Serial.begin(9600);

    pinMode(sdPin, INPUT);
    pinMode(microswitchPin, INPUT);

    DisplaySerial.begin(9600);
    Display.TimeLimit4D  = 5000;

    Display.gfx_ScreenMode(LANDSCAPE_R);

    while(!Display.file_Mount()) {
        Display.putstr("Not mounted");
        delay(300);
        Display.gfx_Cls();
        delay(100);
    }

    clearScreen();
    Display.putstr("Mounted");
    delay(200);
    clearScreen();

    franklin36 = Display.file_LoadImageControl("Franklin.da1", "Franklin.gc1", 1);
    franklin26 = Display.file_LoadImageControl("Franklin.da2", "Franklin.gc2", 1);
}

void clearCenter() {
    int r = getCenterRadius();
    Display.gfx_CircleFilled(width / 2, height / 2, r, bgColor);
}

int getCenterRadius() {
    return (personDistance - personRadius) - 2;
}

void clearScreen() {
    Display.gfx_BGcolour(bgColor);
    Display.gfx_Cls();
}

void setTimezone(float time, boolean active) {
    drawTimezone(time, active ? accentColor : WHITE);
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

void setFontSize(int textSize) {
    Display.txt_Width(textSize);
    Display.txt_Height(textSize);
}

void setFont(int font) {
    Display.txt_FontID(font);
}

int getTextCenterX(char* str) {
    int stringWidth = 0;

    for(int i = 0; i < strlen(str); i++) {
        stringWidth += Display.charwidth(str[i]);
    }

    return width / 2 - stringWidth / 2;

    // For monospaced fonts:
    // int textWidth = Display.charwidth(str[0]) * strlen(str);
    // return width / 2 - textWidth / 2;
}

int getTextCenterY(char* str) {
    int textHeight = Display.charheight(str[0]);
    return height / 2 - textHeight / 2;
}

char* getTime(int time) {
    return "18:45";
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

void drawList() {
    char* name = peopleNames[scrollIndex];
    char* place = peoplePlaces[scrollIndex];

    int y = 70;

    setFont(franklin36);

    Display.gfx_MoveTo(getTextCenterX(name), y);
    Display.txt_FGcolour(WHITE);
    Display.putstr(name);

    setFont(franklin26);
    Display.gfx_MoveTo(getTextCenterX(place), y + 40);
    Display.putstr(place);

    char* time = getTime(1);
    Display.gfx_MoveTo(getTextCenterX(time), y + 65);
    Display.putstr(time);
}

void drawVoice() {
    char* str = peopleNames[voiceIndex];

    setFontSize(2);

    Display.gfx_MoveTo(getTextCenterX(str), getTextCenterY(str));
    Display.txt_FGcolour(WHITE);

    Display.putstr(str);
};

void drawIdle() {
    setFont(franklin36);

    int baseY = 70;

    char* time = getTime(1);
    Display.gfx_MoveTo(getTextCenterX(time), baseY);
    Display.putstr(time);

    setFont(franklin26);

    int nPeople = (sizeof(people) / sizeof(int));
    char str[15];
    sprintf(str, "%d connected", nPeople);
    int x = getTextCenterX(str) - 10;
    int y = baseY + 35;

    drawPersonIcon(x, y + 3, 4, accentColor);

    Display.gfx_MoveTo(x + 18, y);
    Display.txt_FGcolour(WHITE);

    Display.putstr(str);
}

void drawVolume(float value, boolean redraw) {
    if(redraw) {
        clearScreen();

        for(float i = 1; i >= value; i -= volumeControlSpeed) {
            drawVolumeCircle(i);
        }
    } else {
        drawVolumeCircle(value);
    }

    drawVolumeIcon(width / 2, height / 2, 5, WHITE);
}

void drawVolumeCircle(float value) {
    int shade = value * 80 + 20;
    int color = getColor(shade, shade, shade);
    int radius = value * (volumeMaxRadius - volumeMinRadius) + volumeMinRadius;

    Display.gfx_CircleFilled(width / 2, height / 2, radius, color);
}

void drawVolumeIcon(int x, int y, int s, int color) {
    x = x;

    int x1 = x - s;
    int y1 = y;
    int x2 = x;
    int y2 = y - s;
    int x3 = x;
    int y3 = y + s;

    Display.gfx_TriangleFilled(x1, y1, x2, y2, x3, y3, color);

    int lines = ((volumeValue + 0.1) * 3);
    // int lineWidth = s / lines;

    for(int i = 0; i < lines; i++) {
        int d = i * s + s * 1.6;

        for(int k = 0; k <= 2; k++) {
            float angle = (PI / 6) * k + (PI / 3);
            int px = x + sin(angle) * d;
            int py = y + cos(angle) * d;

            if(k == 0) {
                Display.gfx_MoveTo(px, py);
            } else {
                Display.gfx_LineTo(px, py);
            }
        }
    }
}

uint16_t getColor(int r, int g, int b) {
    return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

void loop() {
    // Power
    boolean prevPower = power;
    boolean forceDraw = false;

    boolean prevSdcard = sdcard;
    sdcard = digitalRead(sdPin);

    if(sdcard != prevSdcard) {
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
    int positionIndex = positionKnob.read();
    int positionDiff = positionIndex - prevPositionIndex;

    boolean prevVolume = volume;
    // boolean volumeChange = false;
    // float prevVolumeValue = volumeValue;

    // int volumeIndex = volumeKnob.read();
    // int volumeDiff = volumeIndex - prevVolumeIndex;

    // if(abs(positionDiff) > rotaryThreshold) {
    //     int dir = positionDiff < 0 ? -1 : 1;
    //     scrollIndex = wrapIndex(scrollIndex + dir, sizeof(people) / sizeof(int));
    //     prevPositionIndex = positionIndex;
    // }

    // if(abs(volumeDiff) > rotaryThreshold) {
    //     Serial.print("Volume = ");
    //     Serial.print(volumeDiff < 0 ? -1 : 1);
    //     Serial.println();

    //     float add = volumeDiff < 0 ? -volumeControlSpeed : volumeControlSpeed;
    //     volumeValue = max(min(volumeValue + add, 1), 0);
    //     prevVolumeIndex = volumeIndex;
    //     volumeChange = true;
    // }

    // Volume

    // Temp: PotPin 12
    float prevVolumeValue = volumeValue;
    potValue = analogRead(A0);
    int potDiff = prevPotValue - potValue;

    if(abs(potDiff) > 20) {
        float add = potDiff < 0 ? -volumeControlSpeed : volumeControlSpeed;
        volumeValue = max(min(volumeValue + add, 1), 0);
        prevPotValue = potValue;
    }

    boolean volumeChange = volumeValue != prevVolumeValue;

    if(!initial && volumeChange) {
        volume = true;
        volumeTimeout = millis() + volumeDelay;
    }

    if(volumeTimeout < millis()) {
        volume = false;
    }

    // Scroll
    boolean scrollChange = prevScrollIndex != scrollIndex;

    if(!initial && scrollChange) {
        scrolling = true;
        scrollTimeout = millis() + 5000;
    }

    if(scrollTimeout < millis()) {
        scrolling = false;
    }

    // Voice
    boolean voiceChange = false;

    while(Serial.available()) {
        int in = int(Serial.read());

        voiceChange = true;
        voice = in % 2 == 1;
        voiceIndex = round(in / 2);
    }

    // Idle
    boolean prevIdle = idle;
    idle = !scrolling && !voice && !volume;
    boolean idleChange = prevIdle != idle;

    // Draw screen
    if(volume && (volumeChange || forceDraw)) {
        boolean redraw = prevVolume != volume;

        Serial.print("Draw volume. Redraw: ");
        Serial.print(redraw);
        Serial.println();

        drawVolume(volumeValue, redraw);
    }

    if(scrolling && (scrollChange || forceDraw)) {
        Serial.println("Draw list");
        clearCenter();
        setTimezone(people[prevScrollIndex], false);
        setTimezone(people[scrollIndex], true);
        drawList();
    }

    if(voice && (voiceChange || forceDraw)) {
        Serial.println("Draw voice");
        clearCenter();
        drawVoice();
    }

    if(idle && (idleChange || forceDraw)) {
        Serial.println("Draw idle");
        clearScreen();
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
