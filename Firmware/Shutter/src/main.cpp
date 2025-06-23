/*========================================================================================

[ a n y m a ]
AnyShut - (DMX) Beamer Shutter


© 2024 Michael Egger AT anyma.ch
Licensed under GNU GPL 3.0

==========================================================================================*/
//                                                                                      LIB

#include <Arduino.h>
#include "FastLED.h"
#include "ServoEasing.hpp"
#include "Agora.h"

const int PIN_SERVO = 5;
const int PIN_PIXEL = 17;
const int PIN_BUTTON = 6;

int shutter_closed = 1;
int position_closed = 170;
int position_open = 30;
int shutter_speed = 360;

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

  if (shutter_closed == 0)
  {
    myservo.easeTo(position_open, SERVO_DEGREES_PER_SECOND); // set the servo position according to the scaled value
  }
  else
  {
    myservo.easeTo(position_closed, SERVO_DEGREES_PER_SECOND);
  }
  send_shutter_state(MESS_SET_SHUTTER);
  Serial.printf("Time to move: %dms", millis() - start);
}

//----------------------------------------------------------------------------------------
void myCallback(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
  Serial.printf("Message from Controller: %s\n", incomingData);
  if(!strcmp((const char *)incomingData, "OPEN")) {
    shutter_closed = 0;
    set_shutter();
  }
  else if(!strcmp((const char *)incomingData, "CLOSE")) {
    shutter_closed = 1;
    set_shutter();
  }
}

//----------------------------------------------------------------------------------------


void send_shutter_state(int message_type)
{
  char buf[20];
  memset(buf, 0, sizeof(buf));
  sprintf(buf, "%s", shutter_closed ? "CLOSED" : "OPEN");
  Agora.tell (buf,sizeof(buf));
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

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  last_btn = digitalRead(PIN_BUTTON);

  myservo.setEasingType(EASE_CUBIC_IN_OUT); // EASE_LINEAR is default
  myservo.attach(PIN_SERVO, position_closed);

  Agora.begin("Shutter", true);
  Agora.join("anyshut", myCallback);

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

      shutter_closed = shutter_closed ? 0 : 1;
      set_shutter();
    }
  }

  CRGB led_color;
  if (!shutter_closed)
  {
    led_color = Agora.connected() ? CRGB::DarkGray : CRGB::Orange;
  }
  else
  {
    led_color = Agora.connected() ? CRGB::Blue : CRGB::Red;
  }

  if (pixel[0] != led_color)
  {
    pixel[0] = led_color;
    FastLED.show();
  }
}
