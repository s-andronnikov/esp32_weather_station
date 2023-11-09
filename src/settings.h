// SPDX-FileCopyrightText: 2023 ThingPulse Ltd., https://thingpulse.com
// SPDX-License-Identifier: MIT

#pragma once

// ****************************************************************************
// User settings
// ****************************************************************************
// WiFi
#ifdef WIFI_SSID
const char *SSID = WIFI_SSID;
#else
const char *SSID = "NETGEAR29";
#endif

#ifdef WIFI_PWD
const char *WIFI_PWD = WIFI_PWD;
#else
const char *WIFI_PWD = "purpleplum557";
#endif

// timezone Europe/Zurich as per https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TIMEZONE "EET-2EEST,M3.5.0/3,M10.5.0/4"

#define UPDATE_INTERVAL_MINUTES 10

// uncomment to get "08/23/2022 02:55:02 pm" instead of "23.08.2022 14:55:02"
// #define DATE_TIME_FORMAT_US

// values in metric or imperial system?
bool IS_METRIC = true;

// OpenWeatherMap Settings
// Sign up here to get an API key: https://docs.thingpulse.com/how-tos/openweathermap-key/
const String OPEN_WEATHER_MAP_API_KEY = "ecdbd0bb458af2b2f1f3bde9f098be0d";

/*
Go to https://openweathermap.org/find?q= and search for a location. Go through the
result set and select the entry closest to the actual location you want to display
data for. It'll be a URL like https://openweathermap.org/city/2657896. The number
at the end is what you assign to the constant below.
 */
const String OPEN_WEATHER_MAP_LOCATION_ID = "706483";
const String DISPLAYED_LOCATION_NAME = "Kharkiv";
//String OPEN_WEATHER_MAP_LOCATION_ID = "3833367";
//String DISPLAYED_LOCATION_NAME = "Ushuaia";
//String OPEN_WEATHER_MAP_LOCATION_ID = "2147714";
//String DISPLAYED_LOCATION_NAME = "Sydney";
//String OPEN_WEATHER_MAP_LOCATION_ID = "5879400";
//String DISPLAYED_LOCATION_NAME = "Anchorage";

// Supported languages: https://openweathermap.org/current#multi
const String OPEN_WEATHER_MAP_LANGUAGE = "en";

const String WEEKDAYS[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const String WEEKDAYS_ABBR[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

const String SUN_MOON_LABEL[] = {"Sun", "Moon"};
const String MOON_PHASES[] = {"New Moon", "Waxing Crescent", "First Quarter", "Waxing Gibbous",
                              "Full Moon", "Waning Gibbous", "Third quarter", "Waning Crescent"};



// ****************************************************************************
// System settings - do not modify unless you understand what you are doing!
// ****************************************************************************
typedef struct RectangleDef {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
} RectangleDef;

typedef struct DayForecast {
  float minTemp;
  float maxTemp;
  int conditionCode;
  int conditionHour;
  int day;
} DayForecast;

RectangleDef timeSpritePos = {0, 0, 240, 88};
RectangleDef timeLargeSpritePos = {30, 0, 180, 320};
RectangleDef lightSpritePos = {10, 95, 70, 20};

const String WIND_ICON_NAMES[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

// average approximation for the actual length of the synodic month
const double LUNAR_MONTH = 29.530588853;
const uint8_t NUMBER_OF_MOON_IMAGES = 32;

// 2: portrait, on/off switch right side -> 0/0 top left
// 3: landscape, on/off switch at the top -> 0/0 top left
#ifndef TFT_ROTATION
#define TFT_ROTATION 2
#endif
// all other TFT_xyz flags are defined in platformio.ini as PIO build flags

// 0: portrait, on/off switch right side -> 0/0 top left
// 1: landscape, on/off switch at the top -> 0/0 top left
#define TOUCH_ROTATION 0
#define TOUCH_SENSITIVITY 40
#define TOUCH_SDA 23
#define TOUCH_SCL 22
// Initial LCD Backlight brightness
#define TFT_LED_BRIGHTNESS 100

// the medium blue in the TP logo is 0x0067B0 which converts to 0x0336 in 16bit RGB565
#define TFT_TP_BLUE 0x0336

// format specifiers: https://cplusplus.com/reference/ctime/strftime/
#ifdef DATE_TIME_FORMAT_US
  int timePosX = 29;
  #define UI_DATE_FORMAT "%m/%d/%Y"
  #define UI_TIME_FORMAT "%I:%M:%S %P"
  #define UI_TIME_FORMAT_NO_SECONDS "%I:%M %P"
  #define UI_TIMESTAMP_FORMAT (UI_DATE_FORMAT + " " + UI_TIME_FORMAT)
#else
  int timePosX = 84;
  #define UI_DATE_FORMAT "%d.%m.%Y"
  // #define UI_TIME_FORMAT "%H:%M:%S"
  #define UI_TIME_FORMAT "%H:%M"
  #define UI_TIME_FORMAT_NO_SECONDS "%H:%M"
  #define UI_TIMESTAMP_FORMAT (UI_DATE_FORMAT + " " + UI_TIME_FORMAT)
#endif

#define SYSTEM_TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"

// every 3h (UTC), we need all to be able to calculate daily min/max temperatures
const uint8_t forecastHoursUtc[] = {0, 3, 6, 9, 12, 15, 18, 21};
// 5 day / 3 hour forecast data => 8 forecasts/day => 40 total
#define NUMBER_OF_FORECASTS 40
#define NUMBER_OF_DAY_FORECASTS 4

#define APP_NAME "ESP32 Weather Station Touch"
#define VERSION "1.0.0"

#define PIN_BUTTON_OK 0

#define PAGE_WEATHER 0
#define PAGE_CLOCK 1;