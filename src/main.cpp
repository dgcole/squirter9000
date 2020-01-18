#include <Arduino.h>
#include <RTClib.h>
#include <LiquidCrystal.h>

// Gap between the end of the scroll and the repeating start.
#define SCROLL_GAP 1

// Max length of scrolling info string
#define SCROLL_LENGTH 64

// Length of LCD (characters)
#define LCD_LENGTH 16

// Pins for LCD
#define LCD_RS  4
#define LCD_E   6
#define LCD_D4  8
#define LCD_D5  9
#define LCD_D6  15
#define LCD_D7  16

const char daysOfTheWeek[7] = {'S', 'M', 'T', 'W', 'R', 'F', 'S'};

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

enum state {
    STANDBY,
    SETTING_TIME,
    SETTING_DATE,
    SETTING_ALARM,
    SETTING_ALARM_DAYS
};

// Current state for state machine in loop()
state currState = STANDBY;

// Scrolling text on bottom row of 16x2 LCD
char scroll[SCROLL_LENGTH] = "";

// Real-time clock
RTC_PCF8523 rtc;

// Alarm time
DateTime alarm;

// Alarm set days
bool alarmDays[7] = {false, true, false, false, false, false, false};

// Represents selection number when setting day / time / alarm time / alarm days.
uint8_t selection = 0;

// Draw a string centered within a row on the 16x2 LCD
void drawCentered(const char* str, uint8_t row) {
    uint8_t len = strlen(str);

    if (len > LCD_LENGTH) {
        return;
    }

    uint8_t startCol = (LCD_LENGTH / 2) - (len / 2);

    lcd.setCursor(startCol, row);
    lcd.print(str);
}

// Displays a time, centered
void drawCenteredTime(DateTime time, uint8_t row) {
    char buf[9];
    snprintf(buf, 8, "%02d:%02d:%02d", time.hour(), time.minute(), time.second());
    drawCentered(buf, row);
}

// Draws scrolling text
void drawScroll(const char* str, uint8_t row, uint8_t offset) {
    uint8_t len = strlen(str);

    char buf[LCD_LENGTH + 1];
    strncpy(buf, &str[offset], LCD_LENGTH);
    uint8_t pos = min(LCD_LENGTH, len - offset);
    while (pos < (LCD_LENGTH - 1)) {
        uint8_t gapLength = min((LCD_LENGTH - 1) - pos, SCROLL_GAP);
        memset(&buf[pos], ' ', gapLength);
        pos += gapLength;

        if (pos < (LCD_LENGTH - 1)) {
            uint8_t nextLen = min((LCD_LENGTH - 1) - pos, len);
            strncpy(&buf[pos], str, nextLen);
            pos += nextLen;
        }
    }
    buf[LCD_LENGTH] = '\0';

    lcd.setCursor(0, row);
    lcd.print(buf);
}

void setScroll(char* str) {
    DateTime now = rtc.now();
    int pos = 0;

    snprintf(str, 11, "%02d/%02d/%04d ", now.month(), now.day(), now.year());
    pos += 11;

    snprintf(&str[pos], 16, "ALARM: %02d:%02d:%02d ", alarm.hour(), alarm.minute(), alarm.second());
    pos += 16;

    //TODO; Have all days displayed and days that are on blinking.
    char* end = &str[pos];
    for (int i = 0; i < 7; i++) {
        if (alarmDays[i]) {
            *end++ = daysOfTheWeek[i];
        } else {
            *end++ = ' ';
        }
    }
}

void setup() {
    Serial.begin(57600);
    while (!Serial) { //wait for
        delay(1);
    }
    Serial.print("Wire Initialized!");

    if (! rtc.begin()) {
        Serial.println("Couldn't find RTC");
        exit(0);
    }

    if (! rtc.initialized()) {
        Serial.println("RTC is NOT running!");
    }

    setScroll(scroll);
}

void loop() {
    // State Transitions

    // State Machine
    switch (currState) {
        case STANDBY: {
            lcd.clear();
            drawCenteredTime(rtc.now(), 0);

            static int offset = 0;
            drawScroll(scroll, 1, 0);
            offset++;
            if (offset == sizeof(scroll)) {
                offset = 0;
            }
            break;
        }
        case SETTING_TIME: {
            lcd.clear();
            drawCenteredTime(rtc.now(), 0);
            drawCentered("SET TIME", 1);

            lcd.setCursor(4 + selection * 3, 0);
            lcd.blink();
        }
        case SETTING_DATE: {
            DateTime now = rtc.now();
            char buf[11];

            snprintf(buf, 11, "%02d/%02d/%04d ", now.month(), now.day(), now.year());

            lcd.clear();
            drawCentered(buf, 0);
            drawCentered("SET DATE", 1);

            lcd.setCursor(4 + selection * 3, 0);
            lcd.blink();
        }
        case SETTING_ALARM: {
            lcd.clear();
            drawCenteredTime(alarm, 0);
            drawCentered("SET TIME", 1);

            lcd.setCursor(4 + selection * 3, 0);
            lcd.blink();
        }
        case SETTING_ALARM_DAYS: {
            lcd.clear();
            //TODO; Make selected days blink.
            drawCentered("S M T W R F S", 0);
            drawCentered("SET ALARM DAYS", 1);

            lcd.setCursor(2 + selection * 2, 0);
            lcd.blink();
        }
        default:
            break;
    }

    delay(50);
}
