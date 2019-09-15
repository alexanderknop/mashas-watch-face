#include <pebble.h>

//windows
static Window *s_main_window;

//layers
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

static TextLayer *s_battery_layer;

static Layer *s_canvas_layer;

static TextLayer *s_temperature_layer;
static TextLayer *s_rate_layer;
static BitmapLayer *s_conditions_picture_layer;

static GBitmap *s_sunny_bitmap;
static GBitmap *s_clouds_bitmap;
static GBitmap *s_rain_bitmap;
static GBitmap *s_mist_bitmap;


// Store incoming information
static char temperature_buffer[8];
static char conditions_buffer[32];
static char rate_buffer[32];

//Battery level
static char battery_buffer[5];

static void battery_callback(BatteryChargeState state) {
  // Update meter
  snprintf(battery_buffer, sizeof(battery_buffer), "%d%%", state.charge_percent);
  text_layer_set_text(s_battery_layer, battery_buffer);
}

//draw back of the watchface
static void canvas_update_proc(Layer *this_layer, GContext *ctx) {
/*  GPoint beginH = GPoint(12, 139);
  GPoint endH = GPoint(132, 139);
  
  graphics_draw_line(ctx, beginH, endH);
  
  GPoint beginV = GPoint(93, 139);
  GPoint endV = GPoint(93, 160);
  
  graphics_draw_line(ctx, beginV, endV);
*/
}
 
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);

  // Show the date
  text_layer_set_text(s_date_layer, date_buffer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  Tuple *rate_tuple = dict_find(iterator, MESSAGE_KEY_RATE);

  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%+dC", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    
    if (strcmp(conditions_buffer, "Clouds") == 0) {
      bitmap_layer_set_bitmap(s_conditions_picture_layer, s_clouds_bitmap);
    } else if (strcmp(conditions_buffer, "Clear") == 0) {
      bitmap_layer_set_bitmap(s_conditions_picture_layer, s_sunny_bitmap);
    } else if (strcmp(conditions_buffer, "Rain") == 0) {
      bitmap_layer_set_bitmap(s_conditions_picture_layer, s_rain_bitmap);
    } else if (strcmp(conditions_buffer, "Mist") == 0) {
      bitmap_layer_set_bitmap(s_conditions_picture_layer, s_mist_bitmap);
    }
    // Display
    text_layer_set_text(s_temperature_layer, temperature_buffer);
  }
  if(rate_tuple) {
    snprintf(rate_buffer, sizeof(rate_buffer), "$=%ld.%ld", rate_tuple->value->int32 / 100, 
             rate_tuple->value->int32 % 100);
    // Display
    text_layer_set_text(s_rate_layer, rate_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(GRect(45, 76, 90, 35));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter );

  //Create layer to draw
  s_canvas_layer = layer_create(bounds);
  
  // Create temperature Layer
  s_temperature_layer = text_layer_create(
      GRect(145, 76, 30, 35));

  // Style the text
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_text_color(s_temperature_layer, GColorBlue);
  text_layer_set_font(s_temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentLeft );
  text_layer_set_text(s_temperature_layer, "..");
  
  // Create conditions Layer
  s_rate_layer = text_layer_create(
      GRect(45, 131, 90, 20));

  // Style the text
  text_layer_set_background_color(s_rate_layer, GColorClear);
  text_layer_set_text_color(s_rate_layer, GColorFromHEX(0xA7A9AC));
  text_layer_set_font(s_rate_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_rate_layer, GTextAlignmentCenter );
  text_layer_set_text(s_rate_layer, "..");
  
  // Create GBitmaps
  s_sunny_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_SUNNY);
  s_clouds_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_CLOUDS);
  s_rain_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_RAIN);
  s_mist_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WEATHER_MIST);

  // Create BitmapLayer to display the GBitmap
  s_conditions_picture_layer = bitmap_layer_create(GRect(10, 76, 20, 20));

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_compositing_mode(s_conditions_picture_layer, GCompOpAssign);

  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(45, 66, 90, 20));
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  // Create battery TextLayer
  s_battery_layer = text_layer_create(GRect(45, 19, 90, 20));
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(s_battery_layer, GColorFromHEX(0xA7A9AC));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, s_canvas_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_rate_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_conditions_picture_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  
  // Set init time
  update_time();
  
  // Set the update procedure for our layer
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_temperature_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_rate_layer);
  text_layer_destroy(s_battery_layer);
  
  layer_destroy(s_canvas_layer);
  
  gbitmap_destroy(s_clouds_bitmap);
  gbitmap_destroy(s_sunny_bitmap);
  
  bitmap_layer_destroy(s_conditions_picture_layer);

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  layer_mark_dirty(s_canvas_layer);
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void init() {
  
  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
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
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  
  battery_callback(battery_state_service_peek());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}