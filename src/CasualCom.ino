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
const unsigned int width = 282;
const unsigned int height = 240;
const unsigned int bgColor = BLACK;
const unsigned int accentColor = getColor(79, 227, 135);
const unsigned int volumeMinRadius = 20;
const unsigned int volumeMaxRadius = height / 2;

int franklin36;
int franklin26;

int personRadius = 5;
float personDistance = height / 2 - personRadius - 10;

// People (Should be in order of timezones)
long currentTime = 3942;
long timer;

int currentLocation = 1;
int people[] = { -5, -4, 0, 1, 3, 4, 5, 9 };
char peopleNames[][30] = { "Ara", "Jane", "Luke", "Ritika", "Takeshi", "Bob", "Julie", "Takeshi" };
char peoplePlaces[][30] = { "Honolulu", "Toronto", "London", "Seoul", "Tokyo", "Hell", "Shit", "Tokyo" };

// State
boolean initial = true;
boolean power = true;

boolean scrolling;
int scrollIndex;
unsigned long scrollTimeout;

boolean voice;
int voiceIndex;

boolean direct;

boolean volume;
float volumeValue = 0.5;
long volumeTimeout;

boolean idle = false;

boolean sdcard = false;
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

void clearScreen() {
    Display.gfx_BGcolour(bgColor);
    Display.gfx_Cls();
}

void clearCenter() {
    int r = getCenterRadius();
    Display.gfx_CircleFilled(width / 2, height / 2, r, bgColor);
}

int getCenterRadius() {
    return (personDistance - personRadius) - 2;
}

void setTimezone(float time, boolean active) {
    drawTimezone(time, active ? accentColor : getColor(100, 100, 100));
}

void clearTimezone(float time) {
    drawTimezone(time, bgColor);
}

void drawTimezone(float time, int color) {
    float angle = getTimezoneAngle(time);
    Display.gfx_CircleFilled(width / 2 + (sin(angle) * personDistance), height / 2 + (cos(angle) * personDistance), personRadius, color);
}

void drawTimezones() {
    int prev;

    for(int i = 0; i < sizeof(people) / sizeof(int); i++) {
        int current = people[i];
        boolean active = scrollIndex == i && scrolling;

        if(prev != current || active) {
            setTimezone(people[i], active);
        }

        prev = people[i];
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

void drawIdle() {
    Display.txt_FGcolour(WHITE);
    setFont(franklin36);

    int baseY = 70;

    char timeStr[15];
    sprintf(timeStr, "%02d:%02d", getHours(), getMinutes());

    Display.gfx_MoveTo(getTextCenterX(timeStr), baseY);
    Display.putstr(timeStr);

    setFont(franklin26);

    int nPeople = (sizeof(people) / sizeof(int));
    char str[15];
    sprintf(str, "%d connected", nPeople);
    int x = getTextCenterX(str) - 10;
    int y = baseY + 35;

    drawPersonIcon(x, y + 3, 4, accentColor);

    Display.gfx_MoveTo(x + 18, y);

    Display.putstr(str);
}

void drawPersonIcon(int x, int y, int r, int color) {
    float bodyDistance = 1.8;
    y = y + r;
    x = x + r;
    Display.gfx_CircleFilled(x, y, r / 1.5, color);
    Display.gfx_CircleFilled(x, y + r * bodyDistance, r, color);
    Display.gfx_MoveTo(x - r, y + r * bodyDistance);
    Display.gfx_RectangleFilled(x - r, y + r * bodyDistance, x + r, y + r * bodyDistance + r, color);
}

void drawPerson(int index, boolean talking, boolean direct) {
    char* name = peopleNames[index];
    char* place = peoplePlaces[index];

    if(direct) {
        Display.gfx_CircleFilled(width / 2, height / 2, getCenterRadius() - 15, accentColor);
        Display.gfx_CircleFilled(width / 2, height / 2, getCenterRadius() - 18, bgColor);
    }

    int y = talking ? 80 : 70;

    setFont(franklin36);
    Display.txt_FGcolour(accentColor);

    Display.gfx_MoveTo(getTextCenterX(name), y);
    Display.putstr(name);

    Display.txt_FGcolour(WHITE);
    setFont(franklin26);

    Display.gfx_MoveTo(getTextCenterX(place), y + 40);
    Display.putstr(place);

    char timeStr[15];
    sprintf(timeStr, "%02d:%02d", getHours(people[index]), getMinutes(people[index]));

    Display.gfx_MoveTo(getTextCenterX(timeStr), y + 65);
    Display.putstr(timeStr);

    if(talking) {
        drawVolumeIcon(width / 2, y -15, 5, WHITE, 1);
    }
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

    drawVolumeIcon(width / 2, height / 2, 5, WHITE, value);
}

void drawVolumeCircle(float value) {
    int shade = value * 80 + 20;
    int color = getColor(shade, shade, shade);
    int radius = value * (volumeMaxRadius - volumeMinRadius) + volumeMinRadius;

    Display.gfx_CircleFilled(width / 2, height / 2, radius, color);
}

void drawVolumeIcon(int x, int y, int s, int color, float volume) {
    x = x;

    int x1 = x - s;
    int y1 = y;
    int x2 = x;
    int y2 = y - s;
    int x3 = x;
    int y3 = y + s;

    Display.gfx_TriangleFilled(x1, y1, x2, y2, x3, y3, color);

    int lines = ((volume + 0.1) * 3);
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

int getHours() {
    return (currentTime / 3600) % 3600;
}

int getMinutes() {
    return (currentTime / 60) % 60;
}

int getHours(int t) {
    int h = getHours() + t;
    if(h < 0) h = 24 + h;
    return h;
}

int getMinutes(int t) {
    return getMinutes();
}

void loop() {
    if(timer < millis()) {
        currentTime++;
        timer = 0;

        timer = millis() + 1000;
    }

    // Power
    boolean prevPower = power;
    boolean forceDraw = false;

    boolean prevSdcard = sdcard;
    sdcard = digitalRead(sdPin);

    if(sdcard != prevSdcard) {
        Serial.println("SD card change");
        if(sdcard) power = !power;
    }

    boolean powerChange = power != prevPower;

    if(powerChange || initial) {
        if(!power) {
            clearScreen();
        } else {
            forceDraw = true;
        }
    }

    if(!power) {
        return;
    }

    // Microswitch
    boolean directChange = false;
    boolean microswitchRead = digitalRead(microswitchPin);

    if(microswitchRead != prevMicroswitch) {
        msLastDebounceTime = millis();
    }

    if ((millis() - msLastDebounceTime) > msDebounceDelay) {
        if(microswitchRead != microswitch) {
            microswitch = microswitchRead;

            if((scrolling || direct) && microswitch) {
                direct = !direct;
                directChange = true;

                if(direct) {
                    scrolling = false;
                } else {
                    scrolling = true;
                }
            }
        }
    }

    prevMicroswitch = microswitchRead;


    // Encoder
    int prevScrollIndex = scrollIndex;
    int positionIndex = positionKnob.read();
    int positionDiff = positionIndex - prevPositionIndex;

    boolean prevVolume = volume;
    float prevVolumeValue = volumeValue;

    int volumeIndex = volumeKnob.read();
    int volumeDiff = volumeIndex - prevVolumeIndex;

    if(abs(positionDiff) > rotaryThreshold) {
        if(!direct) {
            int dir = positionDiff < 0 ? -1 : 1;
            scrollIndex = wrapIndex(scrollIndex + dir, sizeof(people) / sizeof(int));
            scrolling = true;
        }

        prevPositionIndex = positionIndex;
    }

    if(abs(volumeDiff) > rotaryThreshold) {
        float add = volumeDiff < 0 ? -volumeControlSpeed : volumeControlSpeed;
        volumeValue = max(min(volumeValue + add, 1), 0);
        prevVolumeIndex = volumeIndex;
        volume = true;
        scrolling = false;
    }

    // Volume
    boolean volumeValueChange = volumeValue != prevVolumeValue;

    if(!initial && volumeValueChange) {
        volumeTimeout = millis() + volumeDelay;
    }

    if(volumeTimeout < millis() && volumeTimeout > 0) {
        volumeTimeout = 0;
        Serial.println("Volume timout. Set volume to false");
        volume = false;
    }

    boolean volumeChange = volumeValueChange || volume != prevVolume;

    // Scroll
    boolean scrollChange = prevScrollIndex != scrollIndex || (directChange && !direct);

    if(!initial && scrollChange) {
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
    idle = !scrolling && !voice && !volume && !direct;
    boolean idleChange = prevIdle != idle;

    // Draw screen
    if(volume && (volumeChange || forceDraw)) {
        boolean redraw = prevVolume != volume;
        drawVolume(volumeValue, redraw);
    }

    if(direct && (directChange || (volumeChange && !volume))) {
        if(volumeChange) {
            clearScreen();
            drawTimezones();
        }

        drawPerson(scrollIndex, false, true);
    }

    if(scrolling && !direct && (scrollChange || forceDraw)) {
        if(volume) {
            Serial.println("Scrolling. Set volume to false");
            volume = false;
            clearScreen();
            drawTimezones();
        } else {
            clearCenter();
            setTimezone(people[prevScrollIndex], false);
            setTimezone(people[scrollIndex], true);
        }

        drawPerson(scrollIndex, false, false);
    }

    if(voice && !scrolling && (voiceChange || forceDraw)) {
        clearCenter();
        drawPerson(voiceIndex, true, false);
        setTimezone(people[scrollIndex], true);
    }

    if(idle && (idleChange || forceDraw)) {
        Serial.print("Draw idle. Prev volume is ");
        Serial.println(prevVolume);

        if(prevVolume || initial || powerChange) {
            clearScreen();
            drawTimezones();
        } else {
            clearCenter();
            setTimezone(people[scrollIndex], false);
        }

        drawIdle();
    }

    initial = false;
}

float getTimezoneAngle(float time) {
    return -(((time + currentLocation) / 24.0) * TWO_PI);
}

int wrapIndex(int i, int imax) {
     return ((i % imax) + imax) % imax;
}
