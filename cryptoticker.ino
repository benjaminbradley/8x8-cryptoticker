/** \file
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#include "Badge.h"
#include "matrixAnimation.h"
#include "sample-animation-spinner-small.h"
#include "frame-letters.h"
#include "matrixScroller.h"

#define BTC_UPDATE_FREQUENCY_MS 60000

MatrixScroller* scroller;


Badge badge;

const char* userAgent = "ESP8266 Arduino";

// TODO: fill in your WiFi credentials
const char* ssid = "";
const char* password = "";

const char* host = "blockchain.info";
String url = "/ticker";
const int httpsPort = 443;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "0F 71 69 8A 21 AD 7A 58 5D 9B 85 04 63 DE 02 3F";

WiFiClientSecure client;

uint32_t last_update_millis;
char last_btcvalue[10] = "0";
uint32_t color = 0xffffff;

void refresh_btc_value() {
  last_update_millis = millis();
  
  // Use WiFiClientSecure class to create TLS connection
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: " + userAgent + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
    Serial.print("Response header: ");
    Serial.println(line);
  }

  String line;
  do {
    line = client.readStringUntil('\n');
  } while(line[0] != ' ');
  Serial.print("parse this: ");
  Serial.println(line);
  int index = 0;
  int colonCount = 0;
  boolean done = false;
  while(!done) {
    if(line[index] == ':') {
      colonCount++;
      if(colonCount == 2) done = true;
    }
    index++;
  }
  index++;  // for the space
  // read until the decimal
  const String spacer = "  "; // add 2 leading spaces at the beginning so it will scroll on from the side
  char btcvalue[10];
  int btcindex = 0;
  do {
    btcvalue[btcindex] = line[index];
    btcindex++;
    index++;
  } while(line[index] != '.');
  btcvalue[btcindex] = 0; // null terminator
  // compare current to last value
  Serial.print("last: ");
  Serial.println(last_btcvalue);
  Serial.print("new: ");
  Serial.println(btcvalue);
  int8_t diff;
  if(strcmp("0", last_btcvalue) == 0) {
    diff = 0;
  } else {
    diff = strcmp(btcvalue, last_btcvalue);
  }
  if(diff < 0) {
    Serial.println("down");
    color = 0xff0000;
  } else if(diff > 0) {
    Serial.println("up");
    color = 0x00ff00;
  } else {
    Serial.println("no change");
    color = 0xffffff;
  }
  // copy current to last for comparison next round
  strcpy(last_btcvalue, btcvalue);


  Serial.print("BTC value is: '");
  Serial.print(btcvalue);
  Serial.println("'");
  scroller = new MatrixScroller(spacer + btcvalue);
  scroller->setColor(color);
}


void setup() {
  badge.begin();
  badge.matrix.setBrightness(100);

  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  refresh_btc_value();
}

uint32_t last_draw_millis;

void loop()
{
  const uint32_t now = millis();
  if (now - last_draw_millis < 100)   // refresh at 100Hz
    return;
  last_draw_millis = now;
  
  scroller->draw(badge.matrix);
  if(scroller->getPosition() == 0) {
    // the scroller has completed one play-through, update the BTC value if stale
    if(now - last_update_millis > BTC_UPDATE_FREQUENCY_MS) {
      delete scroller;
      refresh_btc_value();
    }
  }
}




