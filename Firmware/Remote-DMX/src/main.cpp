#include <Arduino.h>
#include <FastLED.h>
#include "Display.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

#define PING_INTERVAL 2000

uint8_t broadcastAddress[] = {0xf4, 0x12, 0xfa, 0xc1, 0x68, 0xc4};

const int PIN_BTN[] = {2, 3, 4};
#define PIN_BUTTON 3

#define PIN_SERVO 5
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

bool peer_registered = false;
esp_now_peer_info_t peerInfo;
esp_now_message incoming_message;

uint8_t baseMac[6];

int last_btn;
int shutterstate = 0;
unsigned long last_ping;
unsigned long last_response;
unsigned long last_button_press;
bool ping_waiting;
bool shutter_moving;

// Insert your SSID
constexpr char WIFI_SSID[] = "anyshut";

int32_t getWiFiChannel(const char *ssid)
{
  if (int32_t n = WiFi.scanNetworks())
  {
    for (uint8_t i = 0; i < n; i++)
    {
      if (!strcmp(ssid, WiFi.SSID(i).c_str()))
      {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

#define NUM_PIXELS 3
CRGB pixel[NUM_PIXELS];

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

//--------------------------------------------------------------------------------
// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&incoming_message, incomingData, sizeof(incoming_message));
  //  log_v("Bytes received: %d", len);

  if (incoming_message.message == MESS_GET_SHUTTER)
  {
    last_response = millis();
    log_v("Ping: %dms", last_response - last_ping);
    ping_waiting = false;
    shutterstate = incoming_message.param;
  }
  else if (incoming_message.message == MESS_SET_SHUTTER)
  {
    shutterstate = incoming_message.param;
    shutter_moving = false;
  }

  //  log_v("Shutter state %d", shutterstate);
  if (shutterstate == 0)
  {
    pixel[0] = CRGB::DarkGray;
    FastLED.show();
  }
  else
  {
    pixel[0] = CRGB::Blue;
    FastLED.show();
  }
}

//--------------------------------------------------------------------------------

void setup()
{

  Serial.begin(115200);

  FastLED.addLeds<SK6812, PIN_PIX, GRB>(pixel, NUM_PIXELS);
  FastLED.setBrightness(100);
  for (int hue = 0; hue < 360; hue++)
  {
    fill_rainbow(pixel, NUM_PIXELS, hue, 7);
    delay(5);
    FastLED.show();
  }

  Serial.println("Hello from [ a n y m a ]");
  init_display();

  log_v("Enable Wifi");
  WiFi.mode(WIFI_STA);
  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  WiFi.begin();
  delay(3000);
  log_v("[DEFAULT] ESP32 Board MAC Address: ");
  readMacAddress();
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  last_btn = digitalRead(PIN_BUTTON);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    log_e("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  Serial.println("Setup Done");
}
//--------------------------------------------------------------------------------

void loop()
{

  int btn = digitalRead(PIN_BUTTON);
  /* log_v("Btn %d", btn);
  delay(1000);
   */

  if ((millis() - last_button_press > 200) && (btn != last_btn))
  {

    last_btn = btn;
    if (!btn)
    {
      log_v("Btn pressed");

      shutterstate = shutterstate ? 0 : 1;

      pixel[0] = CRGB::DarkViolet;
      FastLED.show();

      last_button_press = millis();
      shutter_moving = true;

      esp_now_message out_message;
      out_message.message = MESS_SET_SHUTTER;
      out_message.param = shutterstate;
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&out_message, sizeof(out_message));
    }
  }

  if (!shutter_moving && (millis() - last_ping > 3000))
  {
    log_v("Send Ping");
    esp_now_message out_message;
    out_message.message = MESS_GET_SHUTTER;
    out_message.param = 0;
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&out_message, sizeof(out_message));
    last_ping = millis();
    ping_waiting = true;
  }

  if (shutter_moving && (millis() - last_button_press > 2000))
  {
    log_v("Shutter move should long be finished !!");
    pixel[0] = CRGB::Red;
    FastLED.show();
  }

  if (ping_waiting && (millis() - last_ping > 1000))
  {
    ping_waiting = false;
    log_v("Lost connection");
    pixel[0] = CRGB::Red;
    FastLED.show();
  }
  delay(10);
}
