#include <pebble.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DURATION_OF_SECONDS_AFTER_TAP 3
  
static Window *s_main_window;
static TextLayer *s_hr_layer;
static TextLayer *s_min_layer;
static TextLayer *s_sec_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static Window *windowMain;

bool isShowingSeconds = false;
time_t timeOfLastTap = 0;

static uint8_t supportedColors[60] = {
  GColorMalachiteARGB8,
  GColorScreaminGreenARGB8,
  GColorGreenARGB8,
  GColorBrightGreenARGB8,
  GColorSpringBudARGB8,
  GColorInchwormARGB8,
  GColorMediumAquamarineARGB8,
  GColorMediumSpringGreenARGB8,
  GColorMintGreenARGB8,
  GColorCelesteARGB8,
  GColorElectricBlueARGB8,
  GColorCyanARGB8,
  GColorTiffanyBlueARGB8,
  GColorCadetBlueARGB8,
  GColorMidnightGreenARGB8,
  GColorDarkGreenARGB8,
  GColorKellyGreenARGB8,
  GColorIslamicGreenARGB8,
  GColorJaegerGreenARGB8,
  GColorMayGreenARGB8,
  GColorBrassARGB8,
  GColorArmyGreenARGB8,
  GColorLimerickARGB8,
  GColorPastelYellowARGB8,
  GColorIcterineARGB8,
  GColorYellowARGB8,
  GColorWindsorTanARGB8,
  GColorChromeYellowARGB8,
  GColorRajahARGB8,
  GColorOrangeARGB8,
  GColorRedARGB8,
  GColorFollyARGB8,
  GColorSunsetOrangeARGB8,
  GColorMelonARGB8,
  GColorRichBrilliantLavenderARGB8,
  GColorBrilliantRoseARGB8,
  GColorShockingPinkARGB8,
  GColorMagentaARGB8,
  GColorFashionMagentaARGB8,
  GColorRoseValeARGB8,
  GColorBulgarianRoseARGB8,
  GColorDarkCandyAppleRedARGB8,
  GColorJazzberryJamARGB8,
  GColorPurpleARGB8,
  GColorVividVioletARGB8,
  GColorLavenderIndigoARGB8,
  GColorPurpureusARGB8,
  GColorBabyBlueEyesARGB8,
  GColorLibertyARGB8,
  GColorIndigoARGB8,
  GColorImperialPurpleARGB8,
  GColorOxfordBlueARGB8,
  GColorDukeBlueARGB8,
  GColorBlueARGB8,
  GColorBlueMoonARGB8,
  GColorElectricUltramarineARGB8,
  GColorVeryLightBlueARGB8,
  GColorPictonBlueARGB8,
  GColorVividCeruleanARGB8,
  GColorCobaltBlueARGB8
};

static void battery_handler(BatteryChargeState new_state) {
  // Write to buffer and display
  static char s_battery_buffer[] = "00%";
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%%", new_state.charge_percent);
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}

static void update_color(struct tm *tick_time) {
  // Update main bg color
  window_set_background_color(windowMain, (GColor)supportedColors[tick_time->tm_min]);
  
  // Update battery and date font colors to match main bg
  text_layer_set_text_color(s_date_layer, (GColor)supportedColors[tick_time->tm_min]);
  text_layer_set_text_color(s_battery_layer, (GColor)supportedColors[tick_time->tm_min]);

  int r = ((GColor)supportedColors[tick_time->tm_min]).r;
  int g = ((GColor)supportedColors[tick_time->tm_min]).g;
  int b = ((GColor)supportedColors[tick_time->tm_min]).b;
  
  // Update accent colors depending on luminance
  if((((r+r+b+g+g+g)/6.0) >= 1.9)) {
    // Bright main color
    text_layer_set_text_color(s_hr_layer, GColorDarkGray);
    text_layer_set_text_color(s_min_layer, GColorDarkGray);
    text_layer_set_text_color(s_sec_layer, GColorDarkGray);
    text_layer_set_background_color(s_date_layer, GColorDarkGray);    
  } else {
    text_layer_set_text_color(s_hr_layer, GColorWhite);
    text_layer_set_text_color(s_min_layer, GColorWhite);
    text_layer_set_text_color(s_sec_layer, GColorWhite);
    text_layer_set_background_color(s_date_layer, GColorWhite);
  }
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char bufferHr[] = "#00";
  static char bufferMin[] = "00";
  static char bufferDate[] = "00.00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(bufferHr, sizeof("#00"), "#%H", tick_time);
  } else {
    //Use 12 hour format
    strftime(bufferHr, sizeof("#00"), "#%I", tick_time);
  }
  
  strftime(bufferMin, sizeof("00"), "%M", tick_time);
  strftime(bufferDate, sizeof("00.00"), "%m.%d", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_hr_layer, bufferHr);
  text_layer_set_text(s_min_layer, bufferMin);
  text_layer_set_text(s_date_layer, bufferDate);

  static char bufferSec[3];
  if (isShowingSeconds) {
    // isShowingSeconds is toggled by a watch bump to save battery.
    strftime(bufferSec, sizeof("00"), "%S", tick_time);
    text_layer_set_text(s_sec_layer, bufferSec);
  } else {
    // Clear out the seconds field.
    bufferSec[0] = 0;
    text_layer_set_text(s_sec_layer, bufferSec);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if(isShowingSeconds && (difftime(time(NULL), timeOfLastTap) > DURATION_OF_SECONDS_AFTER_TAP)) {
    isShowingSeconds = false;
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  }

  update_time();
  
  update_color(tick_time);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  // Save time of last tap.
  timeOfLastTap = time(NULL);
  
  if(!isShowingSeconds) {
    // Switch to the second_unit timer subscription
    isShowingSeconds = true;
      
    // Immediatley update the time so our tap looks responsive
    struct tm *tick_time = localtime(&timeOfLastTap);
    update_time(tick_time);

    // Resubscribe to the tick timer at every second
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  }
}

static void main_window_load(Window *window) {
  windowMain = window;
  // Create time TextLayer
  s_hr_layer = text_layer_create(GRect(0, 10, 126, 50));
  text_layer_set_background_color(s_hr_layer, GColorClear);
  text_layer_set_text_color(s_hr_layer, GColorBlack);
  text_layer_set_text(s_hr_layer, "#00");
  
  s_min_layer = text_layer_create(GRect(0, 45, 155, 50));
  text_layer_set_background_color(s_min_layer, GColorClear);
  text_layer_set_text_color(s_min_layer, GColorBlack);
  text_layer_set_text(s_min_layer, "00");
  
  s_sec_layer = text_layer_create(GRect(0, 80, 155, 50));
  text_layer_set_background_color(s_sec_layer, GColorClear);
  text_layer_set_text_color(s_sec_layer, GColorBlack);
  text_layer_set_text(s_sec_layer, "00");
  
  s_date_layer = text_layer_create(GRect(0, 145, 144, 25));
  text_layer_set_background_color(s_date_layer, GColorWhite);
  text_layer_set_text_color(s_date_layer, GColorClear);
  text_layer_set_text(s_date_layer, "0000");
  
  s_battery_layer = text_layer_create(GRect(4, 145, 144, 25));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorClear);
  text_layer_set_text(s_battery_layer, "00%");

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_hr_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BLOCK_42)));
  text_layer_set_text_alignment(s_hr_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_hr_layer, GColorWhite);
  
  text_layer_set_font(s_min_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BLOCK_42)));
  text_layer_set_text_alignment(s_min_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_min_layer, GColorWhite);
  
  text_layer_set_font(s_sec_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BLOCK_42)));
  text_layer_set_text_alignment(s_sec_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_sec_layer, GColorWhite);
  
  text_layer_set_font(s_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BLOCK_20)));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  
  text_layer_set_font(s_battery_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BLOCK_20)));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);
  text_layer_set_text_color(s_battery_layer, GColorWhite);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_hr_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_min_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_sec_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
  
  // Get the current battery level
  battery_handler(battery_state_service_peek());
  
  // Make sure the time is displayed from the start
  update_time();
}

static void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(s_hr_layer);
  text_layer_destroy(s_min_layer);
  text_layer_destroy(s_sec_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
}
  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Subscribe to the Battery State Service
  battery_state_service_subscribe(battery_handler);

  // Subscribe to gestures
  accel_tap_service_subscribe(tap_handler);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);

  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  accel_tap_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
