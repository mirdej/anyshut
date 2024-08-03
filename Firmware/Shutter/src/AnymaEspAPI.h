#ifndef __ANYMA_ESP_API_INCLUDED__
#define __ANYMA_ESP_API_INCLUDED__

#include "WiFi.h"
#include "AnymaEspSettings.h"
#include "AnymaESPUtils.h"
#include "ArduinoJson.h"
#include <aWOT.h>
#include "MimeTypes.h"
#include "ServoEasing.hpp"

extern Application app;
extern AnymaEspSettings settings;
extern void set_shutter();
extern ServoEasing myservo;

#define __APP_USE_CORS_HEADERS 1
// set to 1 to be able to run a development server on a remote machine
/*Cross-origin resource sharing (CORS) is a mechanism for integrating applications.
CORS defines a way for client web applications that are loaded in one domain to interact with resources in a different domain.
*/

void setup_api()
{
  //--------------------------------------------------------------------------------------------------------------------------------
  //-------------------------------------------------------------------------------------------------------------------------- GET

  app.get("/api/reboot", [](Request &req, Response &res)
          {
              res.status(204);
              res.set("Content-Type", "application/json");
              res.end(); 
              xTaskCreate(reboot_task, "Reboot",    4096,       NULL,     0,          NULL); });

  app.get("/api/settings", [](Request &req, Response &res)
          {
              res.status(200);
              res.set("Content-Type", "application/json");
              serializeJson(settings.get_json(), req); 
              res.end(); });

 app.get("/api/servo", [](Request &req, Response &res)
          { log_v("Servo reads %d", myservo.read());
          res.print(myservo.read()); });

  app.get("/api/shutter", [](Request &req, Response &res)
          { res.print(settings.shutter_closed); });

  /* app.get("/api/shutter/on") , [](Request &req, Response &res)
          {
              res.status(200);
              res.set("Content-Type", "application/json");
              serializeJson(settings.get_json(), req);
              res.end(); });
 */
  app.get("/api/deviceinfo", [](Request &req, Response &res)
          {
              JsonDocument doc;

              doc["message"] = "deviceinfo";
              doc["firmware"] = PROJECT_PATH;
              doc["version"] = FIRMWARE_VERSION;

              doc["chip"] = ESP.getChipModel();
              doc["chip_rev"] = ESP.getChipRevision();
              doc["chip_cores"] = ESP.getChipCores();
              doc["chip_fcpu"] = ESP.getCpuFreqMHz();
              doc["sdk_version"] = ESP.getSdkVersion();

              doc["flash_size"] = ESP.getFlashChipSize();
              doc["flash_speed"] = ESP.getFlashChipSpeed();
              doc["flash_mode"] = ESP.getFlashChipMode();
              doc["flash_free_sketch_space"] = ESP.getFreeSketchSpace();

              doc["hostname"] = settings.hostname;
              doc["ip"] = WiFi.localIP().toString();
              doc["mac"] = WiFi.macAddress();
              doc["millis"] = millis();

              multi_heap_info_t info;
              heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

              doc["ram_free"] = info.total_free_bytes;
              doc["ram_lowest"] = info.minimum_free_bytes;
              doc["ram_largest_free_block"] = info.largest_free_block;

              doc["psram_size"] = ESP.getPsramSize();
              doc["psram_free"] = ESP.getFreePsram();
              doc["psram_max_alloc"] = ESP.getMaxAllocPsram();

              doc["fs_used"] = LittleFS.usedBytes();
              doc["fs_total"] = LittleFS.totalBytes();

              res.status(200);
              res.set("Content-Type", "application/json");
              serializeJson(doc, req); 
              res.end(); });

  //--------------------------------------------------------------------------------------------------------------------------------
  //-------------------------------------------------------------------------------------------------------------------------- PUT
  app.put("/api/servo", [](Request &req, Response &res)
          { 
            int val = req.parseInt();
            log_v("Servo. Got %d",val);
            if (val < 0) val = 0;
            if (val > 180) val = 180;

            myservo.write(val);
            res.status(200); });

  app.put("/api/setposition/open", [](Request &req, Response &res)
          { 
            settings.position_open = myservo.read();
            res.status(200); });

  app.put("/api/setposition/closed", [](Request &req, Response &res)
          { 
            settings.position_closed = myservo.read();
            res.status(200); });


  app.put("/api/shutter", [](Request &req, Response &res)
          { 
            if (req.read() == '0') {
              settings.shutter_closed = 0;
            } else {
              settings.shutter_closed = 1;
            }
            set_shutter();
            res.print(settings.shutter_closed); });

  app.put("/api/settings", [](Request &req, Response &res)
          {
              JsonDocument doc;
              DeserializationError error = deserializeJson(doc, req);
              // Test if parsing succeeds.
              if (error)
              {
                log_e("DeserializeJson() failed: %s", error.f_str());
                res.sendStatus(500);
                return;
              }
              //serializeJson(doc, Serial);

              SETTING_AS_STRING(hostname)
              SETTING_AS_STRING(ssid)
              SETTING_AS_STRING(pass)

              SETTING_AS_COLORSTRING(blink_color)
              SETTING_AS_INT(dmx_address);
              SETTING_AS_INT(shutter_closed);
              SETTING_AS_INT(position_closed);
              SETTING_AS_INT(position_open);
              SETTING_AS_INT(shutter_speed);

              settings.dirty = true;
              res.sendStatus(204);
              res.end(); });
}

#endif