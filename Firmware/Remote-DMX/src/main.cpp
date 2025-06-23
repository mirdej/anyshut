#include <Arduino.h>
#include <FastLED.h>
#include "Display.h"
#include <Agora.h>
#include <esp_dmx.h>

#define PING_INTERVAL 1000

dmx_port_t dmxPort = 1;
#define RX_SIZE 512
byte data[RX_SIZE];

bool dmxIsConnected = false;

unsigned long lastInteraction = 0;
unsigned long lastUpdate = millis();

const int NUM_BTNS = 3;
const int PIN_BTN[NUM_BTNS] = {2, 3, 8};

const int NUM_PIXELS = 3;
CRGB pixel[NUM_PIXELS];
CRGB pixel_buf[NUM_PIXELS];
Preferences preferences;

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
int dmx_address = 1; // DMX address starts at 1
int shutter_closed = 1;

//--------------------------------------------------------------------------------

void updateDisplay()
{
  char buf[8];
  sprintf(buf, "%03d", dmx_address);
  if (dmxIsConnected)
  {
    display("    DMX OK    ", buf);
  }
  else
  {

    display("---No DMX---", buf);
  }
}

//--------------------------------------------------------------------------------
// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  Serial.printf("Message from Controller: %s\n", incomingData);
  if (!strcmp((const char *)incomingData, "OPEN"))
  {
    shutter_closed = 0;
  }
  else if (!strcmp((const char *)incomingData, "CLOSED"))
  {
    shutter_closed = 1;
  }
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

  log_v("Running Led task");

  while (1)
  {
    fill_solid(pixel, NUM_PIXELS, Agora.connected() ? CRGB(6, 6, 6) : CRGB::DarkBlue);
    pixel[1] = shutter_closed ? CRGB::Red : CRGB::Green; // Button 2 shows shutter status
    for (int i = 0; i < NUM_BTNS; i++)
    {
      if (!digitalRead(PIN_BTN[i]))
      {
        pixel[i] = CRGB::LightCyan;
      }
    }
    FastLED.show();
    vTaskDelay(pdMS_TO_TICKS(40)); // ~25 fps display
  }

  vTaskDelete(NULL); // we never get here
}

//----------------------------------------------------------------------------------------
void toggleShutter()
{
  char buf[20];
  memset(buf, 0, sizeof(buf));
  sprintf(buf, "%s", shutter_closed ? "OPEN" : "CLOSE");
  Agora.tell(buf, sizeof(buf));
}
//----------------------------------------------------------------------------------------
//																				                                      Button Task

void button_task(void *)
{
  int last_button_state[NUM_BTNS];
  unsigned long lastButtonPress[NUM_BTNS] = {0, 0, 0};
  int button_state;
  int step = 1;

  for (int i = 0; i < NUM_BTNS; i++)
  {
    pinMode(PIN_BTN[i], INPUT_PULLUP);
    last_button_state[i] = digitalRead(PIN_BTN[i]);
  }

  Serial.printf("Running Button task");
  while (1)
  {
    int lastDmxAddress = dmx_address;
    for (int i = 0; i < NUM_BTNS; i++)
    {
      button_state = digitalRead(PIN_BTN[i]);
      if (i == 1)
      {
        step = button_state ? 1 : 10; // Button 2 changes step size
      }

      if (button_state != last_button_state[i])
      {
        last_button_state[i] = button_state;
        if (!button_state)
        {
          Serial.printf("Button %d pressed\n", i);
          lastButtonPress[i] = millis();
          if (i == 0)
          {
            dmx_address -= step;
          }
          else if (i == 1) // Button 2
          {
            toggleShutter();
          }
          else if (i == 2)
          {
            dmx_address += step;
          }
          else
          {
            lastButtonPress[i] = 0;
          }
        }
        else
        {
          lastButtonPress[i] = 0;
        }
      }

      if (lastButtonPress[i] > 0 && (millis() - lastButtonPress[i]) > 126)
      {
        lastButtonPress[i] = millis(); // reset button press
        if (i == 0)                    // Button 1
        {
          dmx_address -= step;
        }
        else if (i == 2) // Button 3
        {
          dmx_address += step;
        }
      }
    }
    if (dmx_address < 1)
    {
      dmx_address += 512; // wrap around to 512
    }
    if (dmx_address > 512)
    {
      dmx_address -= 512; // wrap around to 1
    }
    if (dmx_address != lastDmxAddress)
    {
      lastInteraction = millis();
      updateDisplay();
    }
    vTaskDelay(pdMS_TO_TICKS(25));
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

  preferences.begin("DMX-To-Anyspot");
  dmx_address = preferences.getInt("dmx_address", 1);
  Serial.printf("DMX Address: %d\n", dmx_address);

  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_personality_t personalities[] = {
      {1, "Default Personality"}};
  int personality_count = 1;
  dmx_driver_install(dmxPort, &config, personalities, personality_count);
  dmx_set_pin(dmxPort, -1, PIN_DMX_RX, -1);

  init_display();
  Agora.begin("Anyshut-DMX");
  Agora.establish("anyshut", OnDataRecv);
  Agora.setPingInterval(PING_INTERVAL);

  log_v("Setup Done");
  log_v("________________________");
}
//--------------------------------------------------------------------------------

void loop()
{

  delay(10);
  static char lastVal;

  dmx_packet_t packet;

  if (dmx_receive(dmxPort, &packet, DMX_TIMEOUT_TICK))
  {

    /* We should check to make sure that there weren't any DMX errors. */
    if (!packet.err)
    {
      /* If this is the first DMX data we've received, lets log it! */
      if (!dmxIsConnected)
      {
        dmxIsConnected = true;
        updateDisplay();
      }

      dmx_read(dmxPort, data, RX_SIZE);
      uint8_t val = data[dmx_address] > 127 ? 1 : 0; // DMX values are 0-255, we use 0 for closed, 1 for open
      if (val != lastVal)
      {
        shutter_closed = lastVal;
        toggleShutter();
        lastVal = val;
      }
    }

    else
    {
      Serial.println("A DMX error occurred.");
    }
  }
  else if (dmxIsConnected)
  {

    Serial.println("DMX was disconnected.");
    updateDisplay();
    dmxIsConnected = false;
  }

  if (lastInteraction > 0 && (millis() - lastInteraction) > 3000)
  {
    lastInteraction = 0;
    preferences.putInt("dmx_address", dmx_address);
    Serial.printf("DMX Address saved: %d\n", dmx_address);
  }
}
