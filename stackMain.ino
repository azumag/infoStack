#include <M5Stack.h>

#include <WiFi.h>
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "ArduinoJson.h"
#include <HTTPClient.h>
#include <base64.h>

// Enter your WiFi setup here:
const char *SSID = "xxxx";
const char *PASSWORD = "xxxx";

AudioGeneratorMP3 *mp3;
AudioFileSourceHTTPStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2S *out;

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}


void setup()
{
  M5.begin();
  Serial.begin(115200);
  delay(1000);
  Serial.println("Connecting to WiFi");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextDatum(MC_DATUM);

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawString("Connecting to WiFi", 160, 120);

  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(SSID, PASSWORD);

  // Try forever
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("...Connecting to WiFi");
    delay(1000);
  }

  M5.Speaker.setVolume(1);
  M5.Speaker.update();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawString("Connected", 160, 120);

//  ConnectToClient();


  M5.Lcd.fillScreen(BLACK);

  
}

void say(String text) {
  // mp3 url
  String URL="http://<cloudFunctions url>/textToSpeech/output?text=";
  String encoded = base64::encode(text);
  encoded.replace("+", "-");
  encoded.replace("/", "_");
  encoded.replace("=", "");
  String url = URL + encoded;
//  M5.Lcd.drawString(URL, 0, 0);
//  String url = URL + "たにくん　だいすき";
//  const char *url_complete = url.c_str();
  const char *url_complete = url.c_str();
  
  file = new AudioFileSourceHTTPStream(url_complete);
  buff = new AudioFileSourceBuffer(file, 40960);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetGain(0.1);
  out->SetOutputModeMono(true);
  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
  mp3->begin(buff, out);

}


String getFromAPI() {
  HTTPClient client;

  client.begin("http://newsapi.org/v2/top-headlines?country=jp&pageSize=6&apiKey=wwwwwwww");

  int httpCode = client.GET();
  
  String result = client.getString();
  
  client.end();
 
  return result;
}


static int lastms = 0;
static bool saying = false;
int newsIndex = 0;

void loop()
{

  M5.Lcd.drawString("loading json", 160, 120);

  if (saying) {
    if (mp3->isRunning()) {
      M5.Lcd.drawString("running", 160, 120);
      if (!mp3->loop()){
        out->stop();
        mp3->stop();
        buff->close();
        delete buff;
        buff = NULL;
        file->close();
        delete file;
        file = NULL;
      }
    } else {
      M5.Lcd.drawString("fin", 160, 140);
      delay(10000);
      saying = false;
      lastms = 0;
      if (newsIndex < 6){
        newsIndex++;
      } else {
        delay(1800000);
        newsIndex = 0;
      }
    }
  } else {
    String json = getFromAPI();
    delay(2000);
    M5.Lcd.drawString("loaded", 160, 120);
    M5.Lcd.drawString("parse json", 160, 120);
    M5.update();

    M5.Lcd.drawString((String)json.length(), 160, 100);

    DynamicJsonDocument doc(json.length()+1000);

    M5.Lcd.drawString(json, 0, 0);
    
    DeserializationError er = deserializeJson(doc, json);

    if (er) {
      M5.Lcd.drawString(String(er.c_str()), 160, 140);
      return;
    }
    
    const char* st = doc["status"];

    const char* title = doc["articles"][newsIndex]["title"];

    M5.Lcd.drawString(String(title), 0, 20);

    saying = true;
    say(String(title));
  }   
}
