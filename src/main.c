#include <pebble.h>
#include "effect_layer.h"
#include "main.h"
#include "languages.h"
#include "ctype.h"

Window *my_window;
Layer *window_layer;
GRect window_bound;

char s_date[] = "21  FEB  2015     "; //test
char s_time[] = "88.44mm"; //test
char s_dow[] = "WEDNESDAY     "; //test  
char s_battery[] = "100%"; //test

uint8_t flag_hoursMinutesSeparator, flag_dateFormat, flag_bluetoothBuzz, flag_language;

TextLayer *time_layer, *dow_layer, *date_layer, *battery_layer;
EffectLayer *time_effect, *dow_effect, *date_effect, *back_effect, *battery_effect, *logo_effect;
EffectOffset time_offset, dow_offset, date_offset, battery_offset, logo_offset;
EffectMask back_mask;
BitmapLayer *left_line, *right_line;

BitmapLayer *logo_layer;
GBitmap *logo_bitmap;

void uppercase(char *sPtr) {  
    while(*sPtr != '\0') {
         *sPtr = toupper ( ( unsigned char ) *sPtr );
         ++sPtr;
    }
}

//creates text layer at given coordinates, given font and alignment  
TextLayer* create_text_layer(GRect coords, int font, GTextAlignment align) {
  TextLayer *text_layer = text_layer_create(coords);
  text_layer_set_font(text_layer, fonts_load_custom_font(resource_get_handle(font)));
  text_layer_set_text_color(text_layer, GColorWhite);  
  text_layer_set_background_color(text_layer, GColorClear);  
  text_layer_set_text_alignment(text_layer, align);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  return text_layer;   
}  


static void bluetooth_handler(bool state) {
  
  if (flag_bluetoothBuzz == 1) vibes_short_pulse();
  
  layer_set_hidden(bitmap_layer_get_layer(logo_layer), !state);
  #ifdef PBL_COLOR
  layer_set_hidden(effect_layer_get_layer(logo_effect), !state);
  #endif
  
}


static void battery_handler(BatteryChargeState state) {
  snprintf(s_battery, sizeof("100%"), "%d%%", state.charge_percent);
  text_layer_set_text(battery_layer, s_battery);
  
  #ifdef PBL_COLOR
  
    static GColor battery_color, battery_text_color, date_color;  
    // doing battery color in ranges with fall thru:
    //       100% - 50% - GColorJaegerGreen
    //       49% - 20% - GColorChromeYellow
    //       19% - 0% - GColorDarkCandyAppleRed
    switch (state.charge_percent) {
         case 100: 
         case 90: 
         case 80: 
         case 70: 
         case 60: 
         case 50: battery_color = GColorJaegerGreen; battery_text_color=GColorMintGreen; date_color = GColorChromeYellow; break;
         case 40: 
         case 30: 
         case 20: battery_color = GColorChromeYellow; battery_text_color = GColorPastelYellow; date_color = GColorGreen; break;
         case 10: 
         case 0:  battery_color = GColorMagenta; battery_text_color = GColorRichBrilliantLavender; date_color = GColorChromeYellow; break;     
     }
       text_layer_set_text_color(time_layer, battery_text_color);
       battery_offset.offset_color = battery_color;
       time_offset.offset_color = battery_color;
       time_offset.orig_color = battery_text_color;
       date_offset.offset_color = date_color;
        
  #endif
  
}




//handling time
void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  
   char format[5];
     
   // building format 12h/24h
   if (clock_is_24h_style()) {
      strcpy(format, "%H:%M"); // e.g "14:46"
   } else {
      strcpy(format, "%l:%M"); // e.g " 2:46" -- with leading space
   }
  
  
   // if separator is dot = replacing colon with it
   if (flag_hoursMinutesSeparator == 1) format[2] = '.';
  
   if (units_changed & MINUTE_UNIT) { // on minutes change - change time
     strftime(s_time, sizeof(s_time), format, tick_time);
     
     if (s_time[0] == ' ') { // if in 12h mode we have leading space in time - don't display it (it will screw centering of text) start with next char
       text_layer_set_text(time_layer, &s_time[1]);
     } else {
       text_layer_set_text(time_layer, s_time);  
     }
     
   }  

  
   
   if (units_changed & DAY_UNIT) { // on day change - change date (format depends on flag)
     
     switch(flag_dateFormat){
       case 0:
         if (flag_language == LANG_RUSSIAN) { // if this is Russian - need double bytes
           strftime(s_date, sizeof(s_date), "%b    %d %Y", tick_time); // "DEC 10 2015"
           strncpy(&s_date[0], LANG_MONTH[flag_language][tick_time->tm_mon], 6);
         }  else {
           strftime(s_date, sizeof(s_date), "%b %d %Y", tick_time); // "DEC 10 2015"
           if (flag_language != LANG_DEFAULT) { // if custom language is set - pull from language array
             strncpy(&s_date[0], LANG_MONTH[flag_language][tick_time->tm_mon], 3);
           }  
         }  
       
         break;
       case 1:
         if (flag_language == LANG_RUSSIAN) { // if this is Russian - need double bytes
           strftime(s_date, sizeof(s_date), "%d %b    %Y", tick_time); // "DEC 10 2015"
           strncpy(&s_date[3], LANG_MONTH[flag_language][tick_time->tm_mon], 6);
         }  else {
           strftime(s_date, sizeof(s_date), "%d %b %Y", tick_time); // "DEC 10 2015"
           if (flag_language != LANG_DEFAULT) { // if custom language is set - pull from language array
             strncpy(&s_date[3], LANG_MONTH[flag_language][tick_time->tm_mon], 3);
           }  
         }  
       
         break;
       case 2:
         strftime(s_date, sizeof(s_date), "%Y-%m-%d", tick_time);  // "2015-12-10"
         break;
       
     }
  
     uppercase(s_date);
     text_layer_set_text(date_layer, s_date);
     
     if (flag_language != LANG_DEFAULT) { // if custom language is set - pull from language array
         strcpy(s_dow, LANG_DAY[flag_language][tick_time->tm_wday]);
     } else {  
         strftime(s_dow, sizeof(s_dow), "%A", tick_time);
     }
     
     uppercase(s_dow);
     text_layer_set_text(dow_layer, s_dow);

   }
  
}


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // APP_LOG(APP_LOG_LEVEL_INFO, "***** I am inside of 'inbox_received_callback()' Message from the phone received!");

  // Read first item
  Tuple *t = dict_read_first(iterator);
  
  bool need_time = 0;

    // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
      
      case KEY_HOURS_MINUTES_SEPARATOR:
        if (t->value->int32 != flag_hoursMinutesSeparator) {
          persist_write_int(KEY_HOURS_MINUTES_SEPARATOR, t->value->int32);
          flag_hoursMinutesSeparator = t->value->int32;
          need_time = 1;
        } 
        break;
      case KEY_DATE_FORMAT:
        if (t->value->int32 !=flag_dateFormat) {
          persist_write_int(KEY_DATE_FORMAT, t->value->int32);
          flag_dateFormat = t->value->int32;
          need_time = 1;
        }  
        break;
      case KEY_BLUETOOTH_BUZZ:
        if (t->value->int32 !=flag_bluetoothBuzz) {
          persist_write_int(KEY_BLUETOOTH_BUZZ, t->value->int32);
          flag_bluetoothBuzz = t->value->int32;
        }  
        break;
      case KEY_LANGUAGE:
        if (t->value->int32 !=flag_language) {
          persist_write_int(KEY_LANGUAGE, t->value->int32);
          flag_language = t->value->int32;
          need_time = 1;
        }  
        break;
      
      }   
    
    // Look for next item
    t = dict_read_next(iterator);
  }
  
  if (need_time) {
      //Get a time structure 
      time_t temp = time(NULL);
      struct tm *t = localtime(&temp);
 
      //Manually call the tick handler
      tick_handler(t, MINUTE_UNIT | DAY_UNIT);
  }
  
}



static void window_load(Window *window) {
  
  // creating time layer with effects
  time_layer = create_text_layer(GRect(0, 28, window_bound.size.w, 55), RESOURCE_ID_NEON_53, GTextAlignmentCenter);
  #ifdef PBL_COLOR
    text_layer_set_text_color(time_layer, GColorMintGreen);
  #endif
 
  // creating DOW layer with effects
  dow_layer = create_text_layer(GRect(0, 90, window_bound.size.w, 25), RESOURCE_ID_NEON_22, GTextAlignmentCenter);
  
  // creating date layer with effects
  date_layer = create_text_layer(GRect(0, 120, window_bound.size.w, 25), RESOURCE_ID_NEON_22, GTextAlignmentCenter);
  
  // creating battery layer with effects
  battery_layer = create_text_layer(GRect(49, 147, 47, 20), RESOURCE_ID_NEON_18, GTextAlignmentCenter);

  
  //creating Pebble Logo
  logo_layer = bitmap_layer_create(GRect(14,1,115,35));
  bitmap_layer_set_compositing_mode(logo_layer, GCompOpSet); //make transparent
  logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LOGO);
  bitmap_layer_set_bitmap(logo_layer, logo_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(logo_layer));
  
  //creating lines for surrounding battery percentage
  left_line = bitmap_layer_create(GRect(4, 157, 43, 2));
  bitmap_layer_set_background_color(left_line, GColorWhite);
  layer_add_child(window_layer, bitmap_layer_get_layer(left_line));
  
  right_line = bitmap_layer_create(GRect(97, 157, 43, 2));
  bitmap_layer_set_background_color(right_line, GColorWhite);
  layer_add_child(window_layer, bitmap_layer_get_layer(right_line));
  
  #ifdef PBL_COLOR
  
    time_effect = effect_layer_create(GRect(0, 30, window_bound.size.w, 55));
    time_offset = (EffectOffset){.orig_color = GColorMintGreen, .offset_color = GColorGreen, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(time_effect, effect_outline, &time_offset);
    effect_layer_add_effect(time_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(time_effect));
  
    dow_effect = effect_layer_create(GRect(0, 90, window_bound.size.w, 30));
    dow_offset = (EffectOffset){.orig_color = GColorWhite, .offset_color = GColorCyan, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(dow_effect, effect_outline, &dow_offset);
    effect_layer_add_effect(dow_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(dow_effect));
  
    date_effect = effect_layer_create(GRect(0, 120, window_bound.size.w, 30));
    date_offset = (EffectOffset){.orig_color = GColorWhite, .offset_color = GColorChromeYellow, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(date_effect, effect_outline, &date_offset);
    effect_layer_add_effect(date_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(date_effect));
  
    battery_effect = effect_layer_create(GRect(0, 147, 144, 20));
    battery_offset = (EffectOffset){.orig_color = GColorWhite, .offset_color = GColorGreen, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(battery_effect, effect_outline, &battery_offset);
    effect_layer_add_effect(battery_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(battery_effect));
  
    logo_effect = effect_layer_create(GRect(14,0,115,35));
    logo_offset = (EffectOffset){.orig_color = GColorWhite, .offset_color = GColorCyan, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(logo_effect, effect_outline, &logo_offset);
    effect_layer_add_effect(logo_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(logo_effect));
 
  #endif
  
  
  // background bitmap & blur effect
  #ifndef PBL_PLATFORM_APLITE
     back_effect = effect_layer_create(window_bound);
     back_mask = (EffectMask){.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_BACK), .background_color = GColorClear, .bitmap_mask = NULL, .text = NULL };
     back_mask.mask_colors = malloc(sizeof(GColor)*2);
     back_mask.mask_colors[0] = GColorBlack;
     back_mask.mask_colors[1] = GColorClear;
     effect_layer_add_effect(back_effect, effect_mask, &back_mask);
     layer_add_child(window_layer, effect_layer_get_layer(back_effect));
  #endif
 
  
}

static void window_unload(Window *window) {
   text_layer_destroy(time_layer);
   text_layer_destroy(dow_layer);
   text_layer_destroy(date_layer);
   text_layer_destroy(battery_layer);
   bitmap_layer_destroy(logo_layer);
   bitmap_layer_destroy(left_line);
   bitmap_layer_destroy(right_line);
   effect_layer_destroy(time_effect);
   effect_layer_destroy(dow_effect);
   effect_layer_destroy(date_effect);
   effect_layer_destroy(battery_effect);
   if (back_mask.bitmap_background) gbitmap_destroy(back_mask.bitmap_background);
   effect_layer_destroy(back_effect);
}


void handle_init(void) {
  //going international
  setlocale(LC_ALL, "");
  
  my_window = window_create();
  window_set_background_color(my_window, GColorBlack);
  window_layer = window_get_root_layer(my_window);
  window_bound = layer_get_bounds(window_layer);
  
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
 
  
  window_stack_push(my_window, true);
  
  flag_hoursMinutesSeparator = persist_exists(KEY_HOURS_MINUTES_SEPARATOR)? persist_read_int(KEY_HOURS_MINUTES_SEPARATOR) : 0;
  flag_dateFormat = persist_exists(KEY_DATE_FORMAT)? persist_read_int(KEY_DATE_FORMAT) : 0;
  flag_bluetoothBuzz = persist_exists(KEY_BLUETOOTH_BUZZ)? persist_read_int(KEY_BLUETOOTH_BUZZ) : 0;
  flag_language = persist_exists(KEY_LANGUAGE)? persist_read_int(KEY_LANGUAGE) : LANG_DEFAULT;
  
  // initial bluetooth check
  flag_bluetoothBuzz = 0;
  bluetooth_connection_service_subscribe(bluetooth_handler);
  bluetooth_handler(bluetooth_connection_service_peek());
  flag_bluetoothBuzz = persist_exists(KEY_BLUETOOTH_BUZZ)? persist_read_int(KEY_BLUETOOTH_BUZZ) : 0;
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
   //Get a time structure so that the face doesn't start blank
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
 
  //Manually call the tick handler when the window is loading
  tick_handler(t, DAY_UNIT | MINUTE_UNIT);
  
  //getting battery info
  battery_state_service_subscribe(battery_handler);
  battery_handler(battery_state_service_peek());
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  
  // Open AppMessage
  app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM); 
  
}

void handle_deinit(void) {
  window_destroy(my_window);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  app_message_deregister_callbacks();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
