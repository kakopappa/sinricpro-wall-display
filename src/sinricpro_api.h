#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

struct SinricProDevice {
  String name;
  String id;
  bool isOnline;
  String powerState;

  SinricProDevice(const String & _name,
    const String & _id,
      const bool & _isOnline,
        const String & _powerState): name(_name), id(_id), isOnline(_isOnline), powerState(_powerState) {}
};

class SinricProAPI {
  private: const char * api_key;
  const char * base_url = "https://api.sinric.pro/api/v1";
  String access_token;

  public: SinricProAPI(const char * _api_key): api_key(_api_key) {}

  bool authenticate() {
    HTTPClient http;
    http.begin(String(base_url) + "/auth");
    http.addHeader("x-sinric-api-key", api_key);

    int httpResponseCode = http.POST("");
    if (httpResponseCode == 200) {
      String payload = http.getString();
      JsonDocument doc;
      deserializeJson(doc, payload);
      access_token = doc["accessToken"].as < String > ();
      http.end();
      return true;
    } else {
      Serial.printf("Authentication failed. Error code: %d\n", httpResponseCode);
      http.end();
      return false;
    }
  }

  std::vector < SinricProDevice > getDevices() {
    std::vector < SinricProDevice > devices;

    if (access_token.isEmpty()) {
      if (!authenticate()) {
        return devices; // Return empty vector if authentication fails
      }
    }

    HTTPClient http;
    http.begin(String(base_url) + "/devices");
    http.addHeader("Authorization", "Bearer " + access_token);

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String payload = http.getString();
      JsonDocument doc;
      deserializeJson(doc, payload);

      if (doc["success"].as < bool > ()) {
        JsonArray devicesArray = doc["devices"];
        for (JsonVariant device: devicesArray) {
          String name = device["name"].as < String > ();
          String id = device["id"].as < String > ();
          String powerState = device["powerState"].as < String > ();
          bool isOnline = device["isOnline"].as < bool > ();
          devices.emplace_back(name, id, isOnline, powerState);
        }
      }
    } else {
      Serial.printf("Failed to get devices. Error code: %d\n", httpResponseCode);
    }

    http.end();
    return devices;
  }
};