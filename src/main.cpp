// SPDX-FileCopyrightText: 2023 ThingPulse Ltd., https://thingpulse.com
// SPDX-License-Identifier: MIT

#include <LittleFS.h>

#include <OpenFontRender.h>
#include <TJpg_Decoder.h>

#include "fonts/open-sans.h"
#include "GfxUi.h"

#include <JsonListener.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>
#include <SunMoonCalc.h>
// #include <OneButton.h>

#include "connectivity.h"
#include "display.h"
#include "persistence.h"
#include "settings.h"
#include "util.h"


// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------
OpenFontRender ofr;
FT6236 ts = FT6236(TFT_HEIGHT, TFT_WIDTH);
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite timeSprite = TFT_eSprite(&tft);
GfxUi ui = GfxUi(&tft, &ofr);

// time management variables
int updateIntervalMillis = UPDATE_INTERVAL_MINUTES * 60 * 1000;
unsigned long lastTimeSyncMillis = 0;
unsigned long lastUpdateMillis = 0;

const int16_t centerWidth = tft.width() / 2;
int16_t currCondTop = 0;
int16_t forecastCondTop = 130;
int16_t astroCondTop = 235;

String prevTimeStr;

OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapForecastData forecasts[NUMBER_OF_FORECASTS];

// ----------------------------------------------------------------------------
// Function prototypes (declarations)
// ----------------------------------------------------------------------------
void drawAstro();
void drawCurrentWeather();
void drawForecast();
void drawProgress(const char *text, int8_t percentage);
void drawTimeAndDate(bool repaint);
void drawTimeAndDateTask(void * parameter);
String getWeatherIconName(uint16_t id, bool today);
void initJpegDecoder();
void initOpenFontRender();
bool pushImageToTft(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
void syncTime();
void repaint(void * parameter);
void updateData(boolean updateProgressBar);
bool repaintInProgress = true;

uint32_t currentBrightness;
void setBrightness(uint32_t brightness);

void setBrightness(uint32_t brightness)
{
  #ifdef TFT_BL
    if (!currentBrightness) {
      // Init brightness pin
      ledcSetup(0, 5000, 8);
      ledcAttachPin(TFT_BL, 0);
    }  

    currentBrightness = brightness;
    ledcWrite(0, currentBrightness);
  #endif
}


// OneButton buttonOk(PIN_BUTTON_OK, false, false);

// ----------------------------------------------------------------------------
// setup() & loop()
// ----------------------------------------------------------------------------
void setup(void) {
  #ifdef PIN_LED1
  pinMode(PIN_LED1, OUTPUT);
  digitalWrite(PIN_LED1, LOW);
  #endif
  #ifdef PIN_LED2
  pinMode(PIN_LED2, OUTPUT);
  digitalWrite(PIN_LED2, LOW);
  #endif

  Serial.begin(115200);
  Serial.println("Starting");
  delay(1000);

  // buttonOk.attachClick([]() {
  //   Serial.println("Click");
  //   digitalWrite(PIN_LED1, HIGH);
  //   repaint();
  // });


  // buttonOk.attachLongPressStart ([]() {
  //   Serial.println("Click [attachLongPressStart]");
  // });


  // logBanner();
  // logMemoryStats();

  initJpegDecoder();
  // initTouchScreen(&ts);
  initTft(&tft);
  setBrightness(TFT_LED_BRIGHTNESS);

  timeSprite.createSprite(timeSpritePos.width, timeSpritePos.height);
  // logDisplayDebugInfo(&tft);

  initFileSystem();
  initOpenFontRender();

  xTaskCreate(
    repaint,          /* Task function. */
    "repaintTask",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    10,                /* Priority of the task. */
    NULL);

  xTaskCreate(
    drawTimeAndDateTask,          /* Task function. */
    "drawTimeAndDateTask",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL);
}

void loop(void) {
  // buttonOk.tick();

  delay(10);
  
  // update if
  // - never (successfully) updated before OR
  // - last sync too far back
  // if (lastTimeSyncMillis == 0 ||
  //     lastUpdateMillis == 0 ||
  //     (millis() - lastUpdateMillis) > updateIntervalMillis) {
  //   repaint();
  // } else {
  //   drawTimeAndDate(false);
  // }

  // delay(15 * 1000); // 30 sec to reload

  // esp_sleep_enable_timer_wakeup(updateIntervalMillis * 1000); //usec
  // esp_deep_sleep_start();
  // esp_light_sleep_start();
  // delay(updateIntervalMillis);


  // if (ts.touched()) {
  //   TS_Point p = ts.getPoint();

  //   uint16_t touchX = p.x;
  //   uint16_t touchY = p.y;

  //   log_d("Touch coordinates: x=%d, y=%d", touchX, touchY);
  //   // Debouncing; avoid returning the same touch multiple times.
  //   delay(50);
  // }
}

// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------
void drawAstro() {
  time_t tnow = time(nullptr);
  struct tm *nowUtc = gmtime(&tnow);

  SunMoonCalc smCalc = SunMoonCalc(mkgmtime(nowUtc), currentWeather.lat, currentWeather.lon);
  const SunMoonCalc::Result result = smCalc.calculateSunAndMoonData();

  ofr.setFontSize(18);
  ofr.cdrawString(SUN_MOON_LABEL[0].c_str(), 30, astroCondTop);
  ofr.cdrawString(SUN_MOON_LABEL[1].c_str(), tft.width() - 55, astroCondTop);

  ofr.setFontSize(14);
  // Sun
  strftime(timestampBuffer, 26, UI_TIME_FORMAT_NO_SECONDS, localtime(&result.sun.rise));
  ofr.cdrawString(timestampBuffer, 30, 20+astroCondTop);
  
  strftime(timestampBuffer, 26, UI_TIME_FORMAT_NO_SECONDS, localtime(&result.sun.set));
  ofr.cdrawString(timestampBuffer, 30, 35+astroCondTop);

  // Moon
  strftime(timestampBuffer, 26, UI_TIME_FORMAT_NO_SECONDS, localtime(&result.moon.rise));
  ofr.cdrawString(timestampBuffer, tft.width() - 55, 20+astroCondTop);
  strftime(timestampBuffer, 26, UI_TIME_FORMAT_NO_SECONDS, localtime(&result.moon.set));
  ofr.cdrawString(timestampBuffer, tft.width() - 55, 35+astroCondTop);

  // Moon icon
  int imageIndex = round(result.moon.age * NUMBER_OF_MOON_IMAGES / LUNAR_MONTH);
  if (imageIndex == NUMBER_OF_MOON_IMAGES) imageIndex = NUMBER_OF_MOON_IMAGES - 1;
  ui.drawBmp("/moon/m-phase-" + String(imageIndex) + ".bmp", centerWidth - 37, 5+astroCondTop);

  // ofr.setFontSize(12);
  // ofr.cdrawString(MOON_PHASES[result.moon.phase.index].c_str(), centerWidth, 40+astroCondTop);

  log_i("Moon phase: %s, illumination: %f, age: %f -> image index: %d",
        result.moon.phase.name.c_str(), result.moon.illumination, result.moon.age, imageIndex);
}

void drawCurrentWeather() {
  // re-use variable throughout function
  String text = "";

  // icon
  String weatherIcon = getWeatherIconName(currentWeather.weatherId, true);
  ui.drawBmp("/weather/" + weatherIcon + ".bmp", 10, 30+currCondTop);
  // tft.drawRect(5, 125, 100, 100, 0x4228);

  // condition string
  ofr.setFontSize(22);
  ofr.cdrawString(currentWeather.description.c_str(), centerWidth, currCondTop);

  // temperature incl. symbol, slightly shifted to the right to find better balance due to the ° symbol
  ofr.setFontSize(32);

  text = String(currentWeather.temp, 1) + "°";
  ofr.cdrawString(text.c_str(), centerWidth + 10, 30+currCondTop);

  ofr.setFontSize(16);

  // Min - Max
  // ofr.cdrawString(String(String(currentWeather.tempMin, 0) + "-" + String(currentWeather.tempMax, 0) + "°").c_str(), centerWidth - 75, 105+currCondTop);

  // humidity
  text = String(currentWeather.humidity) + " %";
  ofr.cdrawString(text.c_str(), centerWidth+20, 68+currCondTop);

  // pressure
  text = String(currentWeather.pressure) + " hPa";
  ofr.cdrawString(text.c_str(), centerWidth+5, 90+currCondTop);

  // wind rose icon
  int windAngleIndex = round(currentWeather.windDeg * 8 / 360);
  if (windAngleIndex > 7) windAngleIndex = 0;
  ui.drawBmp("/wind/" + WIND_ICON_NAMES[windAngleIndex] + ".bmp", tft.width() - 60, 35+currCondTop);
  // tft.drawRect(tft.width() - 80, 125, 75, 75, 0x4228);

  // wind speed
  text = String(currentWeather.windSpeed, 0);
  if (IS_METRIC) text += " m/s";
  else text += " mph";
  ofr.cdrawString(text.c_str(), tft.width() - 40, 90+currCondTop);
}

void drawForecast() {
  DayForecast* dayForecasts = calculateDayForecasts(forecasts);
  for (int i = 0; i < NUMBER_OF_DAY_FORECASTS; i++) {
    log_i("[%d] condition code: %d, hour: %d, temp: %.1f/%.1f", dayForecasts[i].day,
          dayForecasts[i].conditionCode, dayForecasts[i].conditionHour, dayForecasts[i].minTemp,
          dayForecasts[i].maxTemp);
  }

  int widthEigth = tft.width() / 8;
  for (int i = 0; i < NUMBER_OF_DAY_FORECASTS; i++) {
    int x = widthEigth * ((i * 2) + 1);
    ofr.setFontSize(18);
    ofr.cdrawString(WEEKDAYS_ABBR[dayForecasts[i].day].c_str(), x, forecastCondTop);
    ofr.setFontSize(16);
    ofr.cdrawString(String(String(dayForecasts[i].minTemp, 0) + "-" + String(dayForecasts[i].maxTemp, 0) + "°").c_str(), x, 25+forecastCondTop);
    ui.drawBmp("/weather-small/" + getWeatherIconName(dayForecasts[i].conditionCode, false) + ".bmp", x - 25, 45+forecastCondTop);
  }
}

void drawProgress(const char *text, int8_t percentage) {
  ofr.setFontSize(18);
  int pbWidth = tft.width() - 100;
  int pbX = (tft.width() - pbWidth)/2;
  int pbY = 260;
  int progressTextY = 210;

  tft.fillRect(0, progressTextY, tft.width(), 40, TFT_BLACK);
  ofr.cdrawString(text, centerWidth, progressTextY);
  ui.drawProgressBar(pbX, pbY, pbWidth, 15, percentage, TFT_WHITE, TFT_TP_BLUE);
}

void drawSeparator(uint16_t y) {
  tft.drawFastHLine(10, y, tft.width() - 2 * 15, 0x4228);
}


void drawTimeAndDateTask(void * pvParameters) {
  for(;;){
    if (!repaintInProgress) {
      drawTimeAndDate(false);
    }
    vTaskDelay(30*1000/portTICK_PERIOD_MS);
  }
}

void drawTimeAndDate(bool repaint) {
  String currTimeStr = getCurrentTimestamp(UI_TIME_FORMAT);
  if (repaint || !prevTimeStr || prevTimeStr != currTimeStr) {
    timeSprite.fillSprite(TFT_BLACK);
    ofr.setDrawer(timeSprite);

    // Time
    ofr.setFontSize(64);
    // centering that string would look optically odd for 12h times -> manage pos manually
    ofr.cdrawString(currTimeStr.c_str(), centerWidth, -15);
    
    // Date
    ofr.setFontSize(12);
    ofr.cdrawString(
      String(WEEKDAYS[getCurrentWeekday()] + ", " + getCurrentTimestamp(UI_DATE_FORMAT)).c_str(),
      centerWidth,
      65
      // TFT_DARKGREY
    );

    timeSprite.pushSprite(timeSpritePos.x, timeSpritePos.y+astroCondTop);
    // set the drawer back since we temporarily changed it to the time sprite above
    ofr.setDrawer(tft);

    prevTimeStr = currTimeStr;
  }
}

String getWeatherIconName(uint16_t id, bool today) {
  // Weather condition codes: https://openweathermap.org/weather-conditions#Weather-Condition-Codes-2

  // For the 8xx group we also have night versions of the icons.
  // Switch to night icons? This could be written w/o if-else but it'd be less legible.
  if ( today && id/100 == 8) {
    if (today && (currentWeather.observationTime < currentWeather.sunrise ||
                  currentWeather.observationTime > currentWeather.sunset)) {
      id += 1000;
    } else if(!today && false) {
      // NOT-SUPPORTED-YET
      // We currently don't need the night icons for forecast.
      // Hence, we don't even track those properties in the DayForecast struct.
      // forecast->dt[0] < forecast->sunrise || forecast->dt[0] > forecast->sunset
      id += 1000;
    }
  }

  if (id/100 == 2) return "thunderstorm";
  if (id/100 == 3) return "drizzle";
  if (id == 500) return "light-rain";
  if (id == 504) return "extrem-rain";
  else if (id == 511) return "sleet";
  else if (id/100 == 5) return "rain";
  if (id >= 611 && id <= 616) return "sleet";
  else if (id/100 == 6) return "snow";
  if (id/100 == 7) return "fog";
  if (id == 800) return "clear-day";
  if (id >= 801 && id <= 803) return "partly-cloudy-day";
  else if (id/100 == 8) return "cloudy";
  // night icons
  if (id == 1800) return "clear-night";
  if (id == 1801) return "partly-cloudy-night";
  else if (id/100 == 18) return "cloudy";

  return "unknown";
}

void initJpegDecoder() {
    // The JPEG image can be scaled by a factor of 1, 2, 4, or 8 (default: 0)
  TJpgDec.setJpgScale(1);
  // The decoder must be given the exact name of the rendering function
  TJpgDec.setCallback(pushImageToTft);
}

void initOpenFontRender() {
  ofr.loadFont(opensans, sizeof(opensans));
  ofr.setDrawer(tft);
  ofr.setFontColor(TFT_WHITE);
  ofr.setBackgroundColor(TFT_BLACK);
}

// Function will be called as a callback during decoding of a JPEG file to
// render each block to the TFT.
bool pushImageToTft(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  // Stop further decoding as image is running off bottom of screen
  if (y >= tft.height()) {
    return 0;
  }

  // Automatically clips the image block rendering at the TFT boundaries.
  tft.pushImage(x, y, w, h, bitmap);

  // Return 1 to decode next block
  return 1;
}

void syncTime() {
  if (initTime()) {
    lastTimeSyncMillis = millis();
    setTimezone(TIMEZONE);
    log_i("Current local time: %s", getCurrentTimestamp(SYSTEM_TIMESTAMP_FORMAT).c_str());
  }
}

void repaint(void * parameter) {
  for(;;){
    repaintInProgress = true;

    tft.fillScreen(TFT_BLACK);
    // ui.drawLogo();

    ofr.setFontSize(14);
    // ofr.cdrawString(APP_NAME, centerWidth, tft.height() - 50);
    // ofr.cdrawString(VERSION, centerWidth, tft.height() - 30);

    drawProgress("Starting WiFi...", 10);
    if (WiFi.status() != WL_CONNECTED) {
      startWiFi();
    }

    drawProgress("Synchronizing time...", 30);
    syncTime();

    updateData(true);

    drawProgress("Ready", 100);
    lastUpdateMillis = millis();

    tft.fillScreen(TFT_BLACK);

    drawCurrentWeather();
    drawSeparator(120+currCondTop);

    drawForecast();
    drawSeparator(astroCondTop-5);

    // drawAstro();
    drawTimeAndDate(true);

    repaintInProgress = false;

    vTaskDelay(updateIntervalMillis/ portTICK_PERIOD_MS);
  }
}

void updateData(boolean updateProgressBar) {
  if(updateProgressBar) drawProgress("Updating weather...", 70);
  OpenWeatherMapCurrent *currentWeatherClient = new OpenWeatherMapCurrent();
  currentWeatherClient->setMetric(IS_METRIC);
  currentWeatherClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient->updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_API_KEY, OPEN_WEATHER_MAP_LOCATION_ID);
  delete currentWeatherClient;
  currentWeatherClient = nullptr;
  log_i("Current weather in %s: %s, %.1f°", currentWeather.cityName, currentWeather.description.c_str(), currentWeather.feelsLike);

  if(updateProgressBar) drawProgress("Updating forecast...", 90);
  OpenWeatherMapForecast *forecastClient = new OpenWeatherMapForecast();
  forecastClient->setMetric(IS_METRIC);
  forecastClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  forecastClient->setAllowedHours(forecastHoursUtc, sizeof(forecastHoursUtc));
  forecastClient->updateForecastsById(forecasts, OPEN_WEATHER_MAP_API_KEY, OPEN_WEATHER_MAP_LOCATION_ID, NUMBER_OF_FORECASTS);
  delete forecastClient;
  forecastClient = nullptr;
}
