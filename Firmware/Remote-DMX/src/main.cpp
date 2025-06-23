#include <Arduino.h>
#include <FastLED.h>
#include "Display.h"
#include <Agora.h>

#define PING_INTERVAL 2000

const int NUM_BTNS = 3;
const int PIN_BTN[NUM_BTNS] = {2, 3, 8};

const int NUM_PIXELS = 3;
CRGB pixel[NUM_PIXELS];
CRGB pixel_buf[NUM_PIXELS];


#define PIN_DMX_RX 6
#define PIN_PIX 7

#define MESS_GET_SHUTTER 1
#define MESS_SET_SHUTTER 2

typedef struct esp_now_message
{
  //  uint8_t mac[6];
  uint8_t message;
  uint8_t param;
} esp_now_message;

int last_btn[NUM_BTNS];

//--------------------------------------------------------------------------------
// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
}


//----------------------------------------------------------------------------------------
//																				                                        LED Task

void led_task(void *)
{

  FastLED.addLeds<SK6812, PIN_PIX, GRB>(pixel, NUM_PIXELS);
  FastLED.setBrightness(250);
  for (int hue = 0; hue < 360; hue++)
  {
    fill_rainbow(pixel, NUM_PIXELS, hue, 7);
    delay(5);
    FastLED.show();
  }
  fill_solid(pixel, NUM_PIXELS, CRGB::DarkBlue);
  fill_solid(pixel_buf, NUM_PIXELS, CRGB::DarkBlue);

  log_v("Running Led task");

  while (1)
  {
    for (int i = 0; i < NUM_BTNS; i++)
    {
      if (!digitalRead(PIN_BTN[i]))
      {
        pixel[i + 1] = CRGB::LightCyan;
      }
      else
      {
        pixel[i + 1] = pixel_buf[i];
      }
    }
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(40)); // ~25 fps display
  }

  vTaskDelete(NULL); // we never get here
}



//----------------------------------------------------------------------------------------
//																				                                      Button Task
void button_task(void *)
{
  int last_button_state[NUM_BTNS];
  int button_state;
  uint8_t buf[2];

  for (int i = 0; i < NUM_BTNS; i++)
  {
    pinMode(PIN_BTN[i], INPUT_PULLUP);
    last_button_state[i] = digitalRead(PIN_BTN[i]);
  }

  log_v("Running Button task");
  while (1)
  {
    for (int i = 0; i < NUM_BTNS; i++)
    {
      button_state = digitalRead(PIN_BTN[i]);
      if (button_state != last_button_state[i])
      {
        last_button_state[i] = button_state;
        if (!button_state)
        {
          Serial.printf("Button %d pressed\n", i);
          buf[0] = 127;
        }
        else
        {
          buf[0] = 0;
        }
        buf[1] = 100 + i;
        Agora.tell(buf, 2);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

//--------------------------------------------------------------------------------

void setup()
{


   xTaskCreate(led_task, "LED Task", 8192, NULL, 1, NULL);

  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  log_v("________________________");
  log_v("Setup");

  xTaskCreate(button_task, "Button", 8192, NULL, 2, NULL);

  /* Agora.begin("Anyshut-DMX");
  Agora.establish("anyshut", OnDataRecv); */
  init_display();


  log_v("Setup Done");
  log_v("________________________");
}
//--------------------------------------------------------------------------------

void loop()
{

  delay(10);
}
