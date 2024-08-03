/*========================================================================================

[ a n y m a ]
AnyShut - (DMX) Beamer Shutter


© 2024 Michael Egger AT anyma.ch
Licensed under GNU GPL 3.0

==========================================================================================*/
//                                                                                      LIB

#include <Arduino.h>
#include "FS.h"

#include "FastLED.h"

#include "AnymaEspSettings.h"
#include "AnyShutEspNetworking.h"
#include "AnymaEspPins.h"
#include "AnymaESPUtils.h"

// TODO: Add FS Switch
#include <LittleFS.h>

// #include <ESP32Servo.h>
#include "ServoEasing.hpp"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

AnymaEspSettings settings;
AnymaEspNetworking networking;

//----------------------------------------------------------------------------------------
//																				                                     User Globals
#define NUM_PIXEL 1
CRGB pixel[NUM_PIXEL];
ServoEasing myservo;
int last_btn;

#define SERVO_DEGREES_PER_SECOND 360

#define PING_INTERVAL 2000

#define MESS_GET_SHUTTER 1
#define MESS_SET_SHUTTER 2
#define MESS_DMX 3

uint8_t baseMac[6];

typedef struct esp_now_message
{
  //  uint8_t mac[6];
  uint8_t message;
  uint8_t param;
} esp_now_message;

bool peer_registered = false;
esp_now_message incoming_message;

void send_shutter_state(int message_type);
unsigned long last_ping_received;
bool connected;

//--------------------------------------------------------------------------------
void set_shutter()
{
  unsigned long start = millis();

  pixel[0] = CRGB::DarkViolet;
  FastLED.show();
  delay(20);

  if (settings.shutter_closed == 0)
  {

    //   myservo.attach(PIN_SERVO, ON_POS);
    myservo.easeTo(settings.position_open, SERVO_DEGREES_PER_SECOND); // set the servo position according to the scaled value
                                                                      // myservo.detach();
  }
  else
  {
    //  myservo.attach(PIN_SERVO, settings.position_open);
    myservo.easeTo(settings.position_closed, SERVO_DEGREES_PER_SECOND);
    //  myservo.detach();
  }
  send_shutter_state(MESS_SET_SHUTTER);
  log_v("Time to move: %dms", millis() - start);
}

//--------------------------------------------------------------------------------

void readMacAddress()
{
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK)
  {
    log_i("%02x:%02x:%02x:%02x:%02x:%02x\n",
          baseMac[0], baseMac[1], baseMac[2],
          baseMac[3], baseMac[4], baseMac[5]);
  }
  else
  {
    log_e("Failed to read MAC address");
  }
}

void send_shutter_state(int message_type)
{
  esp_now_message out_message;
  out_message.message = message_type;
  out_message.param = settings.shutter_closed;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&out_message, sizeof(out_message));

  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  /*  Serial.print("\r\nLast Packet Send Status:\t");
   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
   if (status == 0)
   {
     success = "Delivery Success :)";
   }
   else
   {
     success = "Delivery Fail :(";
   } */
}

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&incoming_message, incomingData, sizeof(incoming_message));
  log_v("Bytes received: %d", len);
  log_v("Message: %d", incoming_message.message);
  if (incoming_message.message == MESS_SET_SHUTTER)
  {
    settings.shutter_closed = incoming_message.param;
    set_shutter();
  }
  else if (incoming_message.message == MESS_GET_SHUTTER)
  {
    send_shutter_state(MESS_GET_SHUTTER);
    last_ping_received = millis();
  }
  else if (incoming_message.message == MESS_DMX)
  {
    if (incoming_message.param == 0)
    {
      settings.shutter_closed = 0;
      set_shutter();
    }
    else if (incoming_message.param == 255)
    {
      settings.shutter_closed = 1;
      set_shutter();
    }
    else
    {
      settings.shutter_closed = incoming_message.param > 127;
      int servoVal = map(incoming_message.param, 0, 255, settings.position_open, settings.position_closed);
      myservo.write(servoVal);
    }

    send_shutter_state(MESS_GET_SHUTTER);
    last_ping_received = millis();
  }
}

//========================================================================================
//----------------------------------------------------------------------------------------
//																				                                           Setup
void setup()
{

  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));
  log_v("%s", PROJECT_PATH);
  log_v("Version %s", FIRMWARE_VERSION);

  log_v("________________________");
  log_v("Setup");

  FastLED.addLeds<SK6812, PIN_PIXEL, GRB>(pixel, NUM_PIXEL);
  FastLED.setBrightness(100);
  for (int hue = 0; hue < 360; hue++)
  {
    fill_rainbow(pixel, NUM_PIXEL, hue, 7);
    delay(5);
    FastLED.show();
  }

  // Format if there is no Filesystem, Max open files = 10 for better Webserver stability
  MAIN_FILE_SYSTEM.begin(true, "/littlefs", 10U);
  file_list();
  settings.begin();
  networking.begin();

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  last_btn = digitalRead(PIN_BUTTON);

  myservo.setEasingType(EASE_CUBIC_IN_OUT); // EASE_LINEAR is default
  myservo.attach(PIN_SERVO, settings.position_closed);
  /*
    log_v("Enable Wifi");
    WiFi.mode(WIFI_STA);s
    WiFi.begin();
    delay(3000); */

  log_v("Setup Done");
  log_v("________________________");
}

//========================================================================================
//----------------------------------------------------------------------------------------
//																				Loop

void loop()
{
  int btn = digitalRead(PIN_BUTTON);
  /* log_v("Btn %d", btn);
  delay(1000);
   */

  if (btn != last_btn)
  {

    last_btn = btn;
    if (!btn)
    {
      log_v("Btn pressed");

      settings.shutter_closed = settings.shutter_closed ? 0 : 1;
      set_shutter();
    }
  }

  if ((millis() - last_ping_received) < (1.5 * PING_INTERVAL))
  {
    connected = true;
  }
  else
  {
    connected = false;
  }
  delay(40);

  CRGB led_color;
  if (!settings.shutter_closed)
  {
    led_color = connected ? CRGB::DarkGray : CRGB::Orange;
  }
  else
  {
    led_color = connected ? CRGB::Blue : CRGB::Red;
  }

  if (pixel[0] != led_color)
  {
    pixel[0] = led_color;
    FastLED.show();
  }
}
