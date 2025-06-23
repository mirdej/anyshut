#pragma once

#include "WiFi.h"
#include "Logo.h"
#include <iostream>
#include <queue>
#include <U8g2lib.h>

#define PIN_SCL 9
#define PIN_SDA 10

// +-----------------------------------------------------------------------+
//                                                      Display Task
TaskHandle_t display_task_handle;
#define DISPLAY_TASK_PRIORITY 2
#define DISPLAY_TASK_CORE 1
#define DISPLAY_TASK_DELAY 1000
#define DISPLAY_TASK_STACK_SIZE 4000

extern uint32_t device_delay;

unsigned long show_start = 0;
unsigned long show_end = 0;
int show_start_hour, show_start_minute;

std::queue<String> display_messages;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0,
                                         /* reset=*/U8X8_PIN_NONE,
                                         /* clock=*/PIN_SCL,
                                         /* data=*/PIN_SDA); // ESP32 Thing, HW I2C with pin remapping

#define LCDWidth u8g2.getDisplayWidth()
#define ALIGN_CENTER(t) ((LCDWidth - (u8g2.getUTF8Width(t))) / 2)
#define ALIGN_RIGHT(t) (LCDWidth - u8g2.getUTF8Width(t))
#define ALIGN_LEFT 0

void intro()
{

    u8g2.clearBuffer();
    u8g2.drawBitmap(0, 0, 16, 16, bitmap_anyma);
    u8g2.drawBitmap(32, 16, 8, 48, bitmap_osh);
    u8g2.sendBuffer();
    delay(500);

    u8g2.clearBuffer();
    u8g2.drawBitmap(0, 1, 16, 16, bitmap_anyma);
    char message[30];
    memset(message, 0, 30);
    sprintf(message, "ANYSHUT");
    u8g2.setCursor(ALIGN_CENTER(message), 46);
    u8g2.print(message);
    u8g2.setFont(u8g2_font_tallpixelextended_te);

    sprintf(message, "Version %s", FIRMWARE_VERSION);
    u8g2.setCursor(ALIGN_CENTER(message), 64);
    u8g2.print(message);
    u8g2.sendBuffer();
    delay(500);
}

void init_display()
{
    u8g2.begin();
    u8g2.setPowerSave(0);
    u8g2.setFont(u8g2_font_logisoso22_tr);

    intro();
}

void fontForLength(const char *m)
{
    int l = strlen(m);
    if (l < 10)
    {
        u8g2.setFont(u8g2_font_logisoso22_tr);
    }
    else if (l < 13)
    {
        u8g2.setFont(u8g2_font_logisoso16_tr);
    }
    else
    {
        u8g2.setFont(u8g2_font_unifont_tr);
    }
}
void display(const char *message)
{
    u8g2.clearBuffer();
    fontForLength(message);
    u8g2.setCursor(ALIGN_CENTER(message), 45);
    u8g2.print(message);
    u8g2.sendBuffer();
/*     Serial.println(message);
 */}

void display(const char *message, char *message2)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_logisoso16_tr);

    fontForLength(message);
    u8g2.setCursor(ALIGN_CENTER(message), 18);
    u8g2.print(message);
    fontForLength(message2);
    u8g2.setCursor(ALIGN_CENTER(message2), 63);
    u8g2.print(message2);

    u8g2.sendBuffer();
    //Serial.printf("%s %s\n", message, message2);
}

void display(const char *message, const char *message2)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_logisoso16_tr);

    fontForLength(message);
    u8g2.setCursor(ALIGN_CENTER(message), 18);
    u8g2.print(message);
    fontForLength(message2);
    u8g2.setCursor(ALIGN_CENTER(message2), 64);
    u8g2.print(message2);

    u8g2.sendBuffer();
//    Serial.printf("%s %s\n", message, message2);
}

void display(const char *message, const char *message2, const char *message3)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_logisoso16_tr);

    u8g2.setCursor(ALIGN_CENTER(message), 17);
    u8g2.print(message);

    u8g2.setCursor(ALIGN_CENTER(message2), (64 + 17) / 2);
    u8g2.print(message2);

    u8g2.setCursor(ALIGN_CENTER(message3), 64);
    u8g2.print(message3);

    u8g2.sendBuffer();
    //Serial.printf("%s %s %s\n", message, message2, message3);
}

void display(const char *message, char *message2, int p)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_logisoso16_tr);

    u8g2.setCursor(ALIGN_CENTER(message), 18);
    u8g2.print(message);

    u8g2.setCursor(ALIGN_CENTER(message2), (64 + 18) / 2);
    u8g2.print(message2);

    int w1 = LCDWidth - 4;
    int w2 = round((float)w1 * (float)p / 100.);
    u8g2.drawBox(2, 56, w2, 5);
    u8g2.drawFrame(1, 55, w1, 7);
    u8g2.sendBuffer();
}