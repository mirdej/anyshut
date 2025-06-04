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

#define MIN_RSSI -100
#define MAX_RSSI -55

static int calculateSignalLevel(int rssi, int numLevels)
{
    if (rssi <= MIN_RSSI)
    {
        return 0;
    }
    else if (rssi >= MAX_RSSI)
    {
        return numLevels - 1;
    }
    else
    {
        float inputRange = (MAX_RSSI - MIN_RSSI);
        float outputRange = (numLevels - 1);
        return (int)((float)(rssi - MIN_RSSI) * outputRange / inputRange);
    }
}

void intro()
{

    u8g2.clearBuffer();
    u8g2.drawBitmap(0, 0, 16, 16, bitmap_anyma);
    u8g2.drawBitmap(32, 16, 8, 48, bitmap_osh);
    u8g2.sendBuffer();
    delay(500);

    u8g2.clearBuffer();
    u8g2.drawBitmap(0, 0, 16, 16, bitmap_anyma);
    u8g2.setCursor(12, 45);
    u8g2.print("ANYSHUT");
    u8g2.setFont(u8g2_font_tallpixelextended_te);
    u8g2.setCursor(4, 64);
    u8g2.print("Version ");
    u8g2.print(" 0 ");
    u8g2.sendBuffer();
    delay(500);
}

void display_task(void *p)
{

    // init OLED display
    u8g2.begin();
    u8g2.setPowerSave(0);
    u8g2.setFont(u8g2_font_logisoso22_tr);

    intro();

    while (1){
  
        vTaskDelay(DISPLAY_TASK_DELAY / portTICK_RATE_MS);
    } 
}

void init_display()
{

    xTaskCreatePinnedToCore(
        display_task,            /* Function to implement the task */
        "DISPLAY Task",          /* Name of the task */
        DISPLAY_TASK_STACK_SIZE, /* Stack size in words */
        NULL,                    /* Task input parameter */
        DISPLAY_TASK_PRIORITY,   /* Priority of the task */
        &display_task_handle,    /* Task handle. */
        DISPLAY_TASK_CORE);      /* Core where the task should run */
}
