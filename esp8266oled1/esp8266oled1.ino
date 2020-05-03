#include <SparkFunTSL2561.h> //TSL2561 valoanturi-kirjasto
#include <TimeLib.h> //Aika -kirjasto

#include <Wire.h>
#include <OneWire.h> // DS18B20 -kirjastot
#include "SSD1306.h" // alias `#include "SSD1306Wire.h"
#include <DallasTemperature.h> // DallasTemperature -kirjasto
#include "OLEDDisplayUi.h" // Oled -näytön kirjasto
#include "images.h" 

#define ONE_WIRE_BUS D6 // DS18B20 -data tulee nyt arduinolle pinnille D6

OneWire oneWire(ONE_WIRE_BUS); // Nimetään 1-Wire_BUS "OneWire":ksi
DallasTemperature sensors(&oneWire); // Nimetään DallasTempature "sensors"

SFE_TSL2561 light; // Valoanturi on nyt "light"
long previousMillis = 0; // Tarvitaan myöhemmin

// Alustetaan OLED -näyttö näyttämään luxeja
SSD1306  display(0x3c, D1, D2); //Pinnit D1 ja D2 TSL2561 datalle. D1 on SDA ja D2 SCL. Nämä erikseen lisätty "SparkFunTSL2561.h" -kirjastoon riveille 38 ja 39
OLEDDisplayUi ui ( &display );

// Optimoidaan OLED näyttö, pikseleiden käyttö:
int screenW = 128;
int screenH = 64;
int screenCenterX = screenW / 2;
int screenCenterY = ((screenH - 16) / 2) + 16;
int screenRadius = 23;
double lux = 10; // Valonvoimakkuuden oletusarvo
float temp = 23.0; // Lämpötilan oletusarvo
boolean gain; // Totuus arvo gain, jota käytetään alla TSL2561 
unsigned int ms; //Käytetään myös TSL2561

// Peittokuva
void screenOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
}

// Kehys 1
void emptyFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
}

// Kehys 2
// Info-kehys, lämpötilan ja luksien tiedot, sekä niiden näyttäminen
void infoFrame(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String tempnow = "Tmp: " +  String(temp);
  String luxnow = "Lux: " + String(lux);

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_24);
  display->drawString(screenCenterX + x , 10 + y, tempnow );
  display->drawString(screenCenterX + x , 40 + y, luxnow );
}

// Frames[] -array tallentaa toiminto-pointterit kaikkiin kehyksiin
// Kehys liukuu OLED näytölle kun arduino käynnistetään
FrameCallback frames[] = { emptyFrame, infoFrame };

// Montako kehystä?
int frameCount = 2;

// Peittokuva piirretään kehyksen päälle, tarvitaan vain yksi peittokuva
OverlayCallback overlays[] = { screenOverlay };
int overlaysCount = 1;

void setup() { // Asetukset

  light.begin(D1, D2); // TSL2561 käynnistyy ja alkaa lähettämään dataa pinneihin D1 ja D2
  gain = 0; // Arvo 0 tai 1. 0 tarkoittaa matala lisäys (1X), 1 taas korkea lisäys (16X). Tarvitaan matala lisäys
  unsigned char time = 2; // Aika 2ms
  light.setTiming(gain, time, ms); // SparkFun:in kirjaston funktioita
  light.setPowerUp();
  sensors.begin();
  light.getError();

  Serial.begin(9600);
  Serial.println();

  // Renderöidään 60fps, 80mhz
  ui.setTargetFPS(60);
  ui.disableAllIndicators();
  ui.disableAutoTransition();

  // Transitio "slide" -efekti
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Lisää kehykset
  ui.setFrames(frames, frameCount);

  // Lisää peittokuva
  ui.setOverlays(overlays, overlaysCount);

  // Alustetaan transitio
  ui.init();
  ui.transitionToFrame(1);

  // Käännetään näyttö vaakatasoon
  display.flipScreenVertically();

  // Saadaan tieto siitä, kuinka kauan laite on ollut päällä. Tätä voidaan käyttää siihen, kuinka usein sensoreilta pyydetään dataa pääloopissa
  unsigned long secsSinceStart = millis();
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  unsigned long epoch = secsSinceStart - seventyYears * SECS_PER_HOUR;
  setTime(epoch);
}

// Päälooppi
void loop() {
  int remainingTimeBudget = ui.update(); // remainingTimeBudget aina > 0

  if (remainingTimeBudget > 0) {
    unsigned int data0, data1; // Alustetaan TSL2561 varten data0 ja data1
    unsigned long currentMillis = millis(); // DS18B20 Sensorin mittausvälit
    if (currentMillis - previousMillis > 5000) {
      previousMillis = currentMillis;
      sensors.requestTemperatures(); // Pyydetään DS18B20 -anturilta dataa
      temp = sensors.getTempCByIndex(0);
    }
    if (light.getData(data0, data1)) {
      boolean good;  // Totta, jos sensori ei ole saturaatiossa
      good = light.getLux(gain, ms, data0, data1, lux); // Pyydetään TSL2561 -anturilta dataa
    }
    delay(remainingTimeBudget);
  }
}
