/**
 * NeoPixel Uhr mit NTP-Synchronisierung
 * Dieses Skript steuert einen NeoPixel LED-Ring, um die aktuelle Zeit anzuzeigen,
 * synchronisiert über NTP (Network Time Protocol).
 *
 * Erstellt am: 04.11.2023
 * Letzte Aktualisierung: 04.11.2023 V1.0
 *
 * Autor: Alf Müller / purecrea gmbh Schweiz / www.bastelgarage.ch
 * Updated by Paul J Chase
 *
 *
 * I've added English comments where I thought necessary - most of the setup is the same
 * I modified the main loop to update every LED every time - I prefer constant-time loops
 * on micros - but for the most part the logic is Alf's.
 *
 *
 * Copyright (c) 2023 Alf Müller. Alle Rechte vorbehalten.
 * Veröffentlichung, Verbreitung und Modifikation dieses Skripts sind unter der
 * Bedingung der Namensnennung erlaubt und lizenzfrei.
 *
 * Modifications Copyright (c) 2023 Paul Chase. All rights reserved.
 * Publication, distribution and modification of this script are under the
 * Condition of attribution permitted and royalty-free.
 */
#include <ESP8266WiFi.h>
#include "WiFiManager.h"                    //https://github.com/tzapu/WiFiManager
#include <Timezone.h>                       //https://github.com/JChristensen/Timezone
#include <TimeLib.h>                        //https://github.com/PaulStoffregen/Time
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#define PIN D6
#define NUMPIXELS 120


//#define DEBUG 1    //comment this out to remove serial port chatter

// Initialisierung der NeoPixel-Bibliothek
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Initialisierung des NTP-Clients
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000);

// Zeitzone für Zentraleuropa definieren
//TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};   // Sommerzeit: Letzter Sonntag im März um 2 Uhr morgens +2 Stunden
//TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};      // Standardzeit: Letzter Sonntag im Oktober um 3 Uhr morgens +1 Stunde
//Timezone CE(CEST, CET);
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};    // eastern daylight savings time: -240 minutes or -4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};     // eastern standard time : -300 minutes or -5 hours
Timezone usEastern(usEDT, usEST);

void setup() {
  #ifdef DEBUG
    Serial.begin(500000);  // be sure to set your serial monitor to 500000!
    Serial.println("Serial is working");
  #endif
  
  pixels.begin();
  WiFiManager wifiManager;
  wifiManager.autoConnect("NeoPixelClock");
  timeClient.begin();
}

void loop() {
  // Aktualisiere die NTP-Zeit  // Update NTP time
  timeClient.update();

    // Hole die aktuelle Zeit in UTC  // get the time in UTC
  time_t utc = timeClient.getEpochTime();

  // Wandele die UTC-Zeit in lokale Zeit um  // convert UTC to local time
  time_t local = usEastern.toLocal(utc);

  // Hole die aktuelle Stunde, Minute und Sekunde von der NTP-Zeit  // get the hour, minute and seconds
  int stunden = hour(local);
  int minuten = minute(local);
  int sekunde = second(local);  // Translated into German to keep the code feeling right


  // Setze die LEDs basierend auf der aktuellen Zeit  // this function displays the time
  setTimeOnLEDs(stunden, minuten, sekunde, 0, 0, 127, 0, 127, 0, 255, 0, 0);  // Blau für Minuten, Grün für Stunden, red for seconds
  delay(100);  // Warte eine Sekunde bevor der nächste Durchlauf beginnt  // waiting 1/10 second so we can do seconds
}

/* int clamp(int num)
 *
 * simple function added to ensure we never ask for brightness that's too bright
 * also a good reason to use ints and not bytes everywhere, so overflow can be handled easily
*/
int clamp(int num) {
  num>255 ? return 255 : return num;
}

/* void setTimeOnLEDs(int stunden, int minuten, int sekunde, byte rMinuten, byte gMinuten, byte bMinuten, byte rStunden, byte gStunden, byte bStunden, byte rSekunde, byte gSekunde, byte bSekunde )
 *
 * this sets every LED to the current time.
 */
void setTimeOnLEDs(int stunden, int minuten, int sekunde, byte rMinuten, byte gMinuten, byte bMinuten, byte rStunden, byte gStunden, byte bStunden, byte rSekunde, byte gSekunde, byte bSekunde ) {
  //pixels.clear();  // Schalte alle LEDs aus  //we'll set every LED manually, thankyouverymuch.

  #ifdef DEBUG
    Serial.println("Displaying Time: " + stunden + ":" + minuten + ":" + sekunde);
  #endif
 
  //calculate the number of LEDs to light for the hours
  stunden = stunden % 12;  // Using a 12 hour clock
  int stundenAnzahl = stunden * 5 + minuten / 12;  // Jede Stunde hat 5 LEDs, jede Minute fügt 1/12 einer Stunde hinzu //Each hour has 5 LEDs, each minute adds 1/12 of an hour
  
  #ifdef DEBUG
    Serial.println("Light up hour LEDs 1->" + stundenAnzahl);
  #endif

   // This goes around the minute and hour rings at the same time
  for (int i = 0; i < 60; i++) {
    // i = minutes - and also there are 60 LEDs... so that's confusing.  but for this loop: i = minutes = stundenAnzahl positions
    // We'll calculate the LED indices later.
    #ifdef DEBUG
      Serial.print("LED " + i + ":");
    #endif

    //for each pixel - should it turn on?
    byte rm = 0; byte gm = 0; byte bm = 0;  // minute colors
    byte rh = 0; byte gh = 0; byte bh = 0;  // hour colors

    //this sets the second LED's color.  It mixes the second color with the hour/minute color later.  Note the second lights on both rings.
    if(i == sekunde){
        rm += rSekunde; gm += gSekunde; bm += bSekunde;  // minute colors
        rh += rSekunde; gh += gSekunde; bh += bSekunde;  // hour colors
    }

    //figure out if this LED should be lit as one of the minutes
    if(i < minuten){
        rm += rMinuten; gm += gMinuten; bm += bMinuten;  // minute colors
    }

    //figure out if this LED should be lit as one of the hours
    if(i < stundenAnzahl){
        rh += rStunden; gh += gStunden; bh += bStunden;  // hour colors
    }
   
    // the LED ring starts at the bottom, and the hours go counterclockwise... cuz the ring faces the wrong way.
    // so Minute 0 lights up LED 29 - do that calculation at the lighting
    // all minutes from 0->cur minute light as well.
    // For hours, they start at 90-i and count down to 60 before popping over to 119
   
    // set the color on the minute ring
    #ifdef DEBUG
      Serial.print("Pixel Translation - M: " + ((i+29) % 60) + " ");
    #endif
    pixels.setPixelColor((i+29) % 60, pixels.Color(clamp(rm), clamp(gm), clamp(bm)));

    //set the color on the hour ring
    #ifdef DEBUG
      Serial.println("Pixel Translation - H: " + (((90-i) % 60) + 60));
    #endif
    pixels.setPixelColor(((90-i) % 60) + 60, pixels.Color(clamp(rh), clamp(gh), clamp(bh)));
  }

  pixels.show();  // Aktualisiere den Strip, um die Änderungen anzuzeigen  // yeah let's turn it on :-)
}
