#ifndef __ANYMA_ESP_NETWWORKING_INCLUDED__
#define __ANYMA_ESP_NETWWORKING_INCLUDED__
#include <esp_wifi.h>
#include <esp_now.h>

#include <Arduino.h>
#include <Preferences.h>

#include "FS.h"
#include <LittleFS.h>

#include <ArduinoJson.h>
#include <WiFi.h>

#include <aWOT.h>
#include "MimeTypes.h"
#include <ESPmDNS.h>

#include "AnymaEspSettings.h"
#include "AnymaEspAPI.h"

#include "ESP-FTP-Server-Lib.h"
#include "FTPFilesystem.h"

#define ANYMA_ESP_SERVICE_NAME "anyma_esp32"
#define ANYMA_ESP_SERVICE_PORT 3101

#define WEBROOT "/webroot"
#define READ_BUFFER_SIZE 512
const char *NTP_SERVER = "pool.ntp.org";
const char *TZ_STRING = "CET-1CEST,M3.5.0/2,M10.5.0/3";

uint8_t broadcastAddress[] = {0xf4, 0x12, 0xfa, 0xc1, 0x68, 0x80};

extern void set_shutter();
extern void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);

esp_now_peer_info_t peerInfo;

FTPServer ftp;
WiFiServer server(80);
Application app;

struct AnymaEspNetworking
{
public:
    void begin();
    int num_client_connections = 0;

private:
};

extern AnymaEspNetworking networking;

#if __APP_USE_CORS_HEADERS
void cors_headers(Request &req, Response &res)
{
    res.set("Access-Control-Allow-Origin", "*");
    res.set("Access-Control-Allow-Methods", "GET, POST, PUT, HEAD, OPTIONS");
    res.set("Access-Control-Allow-Headers", "Content-Type");
    //  log_v("_____________________\nPath %s, Method %d", req.path(), req.method());
    // res.status(204);
    if (req.method() == 7)
        res.sendStatus(204);
}
#endif

const char *indexFile = "/index.html";

/** ------------------------------------------------------------------------------------------------------------------------------------
 * @brief  Serve static files from MAIN_FILE_SYSTEM
 *
 *
 *
 * @param  req          aWot Request
 * @param  res          aWot Response
 */
void fileServer(Request &req, Response &res)
{

    if (req.method() != Request::GET)
    {
        return;
    }

    const char *path = req.path();

    if (strcmp(path, "/") == 0)
    {
        path = indexFile;
    }

    char local_path[120];
    strcpy(local_path, WEBROOT);
    strcat(local_path, path);

    log_v("Request file %s -> %s", path, local_path);

    if (!MAIN_FILE_SYSTEM.exists(local_path))
    {
        return;
    }

    File file = MAIN_FILE_SYSTEM.open(local_path);

    if (file.isDirectory())
    {
        file.close();
        return;
    }

    uint8_t readBuffer[READ_BUFFER_SIZE];
    const char *mimeType = MimeTypes::getType(file.name());
    res.set("Content-Type", mimeType);

    res.set("Connection", "close"); // from CardFiles.ino exanple

    size_t length = file.size();

    while (length)
    {
        size_t toRead = length > READ_BUFFER_SIZE ? READ_BUFFER_SIZE : length;
        file.read(readBuffer, toRead);
        res.write(readBuffer, toRead);
        length = length - toRead;
        taskYIELD();
    }
    file.close();
    res.end();
}

String escapedMac()
{
    String s = WiFi.macAddress();
    s.replace(":", "");
    s.toLowerCase();
    return s;
}

void findFriends()
{

    int n = MDNS.queryService(ANYMA_ESP_SERVICE_NAME, "tcp");
    if (n == 0)
    {
        Serial.println("no services found");
    }
    else
    {

        JsonDocument doc;

        Serial.print(n);
        Serial.println(" service(s) found");
        for (int i = 0; i < n; ++i)
        {
            JsonObject server_info;
            ;
            server_info["name"] = String(MDNS.hostname(i));
            Serial.println(MDNS.hostname(i));

            server_info["ip"] = String(MDNS.IP(i).toString());
            server_info["port"] = String(MDNS.port(i));
            doc.add(server_info);
        }

        char data[2084];
        size_t len = serializeJson(doc, Serial);
        /*   Serial.print("Data size: ");
          Serial.println(len,DEC);
          ws.textAll(data, len); */
    }
}
//----------------------------------------------------------------------------------------
//																				                                       WiFi Task
void TaskClientSocket(void *pvParameters)
{
    WiFiClient clientHandle = *((WiFiClient *)pvParameters);
    int client_id = networking.num_client_connections++;

    if (clientHandle.connected())
    {
        //   log_v("New Client Connected. Client#: %d", client_id);
        app.process(&clientHandle);
        clientHandle.stop();
    }
    //  log_v("Client #%d disconnected", client_id);
    networking.num_client_connections--;
    vTaskDelete(NULL);
}

void wifi_task(void *)
{
    long last_mdns_lookup; //= millis();
    configTime(0, 0, NTP_SERVER);
    setenv("TZ", TZ_STRING, 1);
    tzset();
    log_v("WIFi dsconnect returne: %d", WiFi.disconnect(true, true));
    // WiFi.mode(WIFI_AP_STA);
        WiFi.mode(WIFI_AP_STA);


   // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(settings.hostname.c_str());


    //WiFi.setSleep(false); // better resonsiveness, more power consumption
    // wifiMulti.addAP(settings.ssid.c_str(), settings.pass.c_str());
    Serial.println("\n[*] Creating ESP32 AP");
    WiFi.softAP("anyshut", "anyshut-shut", 11);
        Serial.print("[+] AP Created with IP Gateway ");
    Serial.println(WiFi.softAPIP());



  WiFi.begin("Anymair", "Mot de passe pas complique");
Serial.println("\n[*] Connecting to WiFi Network");

    while(WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(100);
    }

    Serial.print("\n[+] Connected to the WiFi network with local IP : ");
    Serial.println(WiFi.localIP());
Serial.print("Wi-Fi Channel: ");
Serial.println(WiFi.channel());


    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    log_v("[DEFAULT] ESP32 Board MAC Address: ");
    // readMacAddress();

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
    // esp_now_register_send_cb(OnDataSent);

    // From WLED:
    //  "end" must be called before "begin" is called a 2nd time
    //  see https://github.com/esp8266/Arduino/issues/7213
    MDNS.end();

    if (!MDNS.begin(settings.hostname.c_str()))
    {
        log_e("Error setting up MDNS responder!");
    }
    else
    {
        log_i("mDNS responder started");
        log_i("Hostname: %s", settings.hostname);
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("ftp", "tcp", 21);
        MDNS.addService(ANYMA_ESP_SERVICE_NAME, "tcp", ANYMA_ESP_SERVICE_PORT);
        MDNS.addServiceTxt(ANYMA_ESP_SERVICE_NAME, "tcp", "mac", escapedMac().c_str());
    }

    // setup_webserver();
    findFriends();

#if __APP_USE_CORS_HEADERS
    app.use(&cors_headers);
#endif

    setup_api();
    app.get(&fileServer);

    server.begin();

    log_i("Webserver started.");

    ftp.addUser(FTP_USER, FTP_PASSWORD);
    ftp.addFilesystem("LittleFS", &LittleFS);
    ftp.begin();
    log_i("FTP server started.");

    while (1)
    {

        if (millis() - last_mdns_lookup > 60000)
        {
            findFriends();
            /*
            IPAddress shouldBeMyIP = MDNS.queryHost(settings.hostname + ".local",1000);
            if (WiFi.localIP() == shouldBeMyIP) {
                log_v("MDNS still working");
            } else {
                log_e("I have disappeared from mDNS? (Probe returned %s). Am I still alive??? (My IP: %s)",shouldBeMyIP,WiFi.localIP().toString().c_str());
            } */
            last_mdns_lookup = millis();
        }

        ftp.handle();

        WiFiClient client = server.available();
        if (client)
        {
            xTaskCreatePinnedToCore(
                TaskClientSocket, "TaskClientSocket", 8192, &client, 2, NULL, ARDUINO_RUNNING_CORE);
        }
        taskYIELD();
    }
}

void AnymaEspNetworking::begin()
{
    xTaskCreate(
        wifi_task,    // Function that implements the task.
        "WIFI-Setup", // Text name for the task.
        8192,         // Stack size in words, not bytes.
        NULL,         // Parameter passed into the task.
        0,            // Priority at which the task is created.
        NULL);
}
#endif