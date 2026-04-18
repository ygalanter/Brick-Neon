#include <pebble.h>
#include <pebble-effect-layer/pebble-effect-layer.h>
#include "main.h"
#include "languages.h"
#include "ctype.h"

#ifdef PBL_RECT  // on rectangular watches need to dynamically change DoW font based on length
  GFont font_neon_18, font_neon_22;
#endif

Window *my_window;
Layer *window_layer;
GRect window_bound;

char s_date[20]; //test

#define DEF_DATE_LEN 11 // length of default date e.g. "SEP 23 2016"
uint8_t date_length; //calculated date length

char s_time[] = "88.44mm"; //test
char s_dow[] = "WEDNESDAY     "; //test  
char s_battery[] = "100%"; //test

uint8_t flag_hoursMinutesSeparator, flag_dateFormat, flag_bluetooth_alert, flag_language, is_buzz_enabled;

TextLayer *time_layer, *dow_layer, *date_layer, *battery_layer;
EffectLayer *time_effect, *dow_effect, *date_effect, *back_effect, *battery_effect, *logo_effect;
EffectOffset time_offset, dow_offset, date_offset, battery_offset, logo_offset;
EffectMask back_mask;
BitmapLayer *left_line, *right_line;

#ifdef PBL_PLATFORM_EMERY
BitmapLayer *emery_bottom_cover;
#endif

BitmapLayer *logo_layer;
GBitmap *logo_bitmap;


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
  
  // if Bluetooth alert is totally disabled - hide BT and exit from here
  if (flag_bluetooth_alert == BLUETOOTH_ALERT_DISABLED) {
    layer_set_hidden(bitmap_layer_get_layer(logo_layer), true);
    #ifdef PBL_COLOR
    layer_set_hidden(effect_layer_get_layer(logo_effect), true);
    #endif
    return;  
  }
  
  //otherwise hide or show BR according to state
  layer_set_hidden(bitmap_layer_get_layer(logo_layer), !state);
  #ifdef PBL_COLOR
  layer_set_hidden(effect_layer_get_layer(logo_effect), !state);
  #endif
  
  // if buzz is disabled - exit from here
  if (flag_bluetooth_alert == BLUETOOTH_ALERT_SILENT || is_buzz_enabled == 0) {
     return;
  }  
  
  //otherwise buzz according to settings
  switch (flag_bluetooth_alert){
    case BLUETOOTH_ALERT_WEAK:
      vibes_enqueue_custom_pattern(VIBE_PATTERN_WEAK);
      break;
    case BLUETOOTH_ALERT_NORMAL:
      vibes_enqueue_custom_pattern(VIBE_PATTERN_NORMAL);
      break;
    case BLUETOOTH_ALERT_STRONG:
    vibes_enqueue_custom_pattern(VIBE_PATTERN_STRONG);
      break;
    case BLUETOOTH_ALERT_DOUBLE:
      vibes_enqueue_custom_pattern(VIBE_PATTERN_DOUBLE);
      break;    
  }

}


static void battery_handler(BatteryChargeState state) {
  snprintf(s_battery, sizeof("100%"), "%d%%", state.charge_percent);
  text_layer_set_text(battery_layer, s_battery);
  
  #ifdef PBL_COLOR
  
    static GColor battery_color, battery_text_color, date_color;
    // Bucket by decile so finer-grained values (e.g. Emery's 85%) still match.
    // 100%-50% JaegerGreen, 49%-20% ChromeYellow, 19%-0% Magenta.
    switch (state.charge_percent / 10) {
         case 10:
         case 9:
         case 8:
         case 7:
         case 6:
         case 5: battery_color = GColorJaegerGreen; battery_text_color=GColorMintGreen; date_color = GColorChromeYellow; break;
         case 4:
         case 3:
         case 2: battery_color = GColorChromeYellow; battery_text_color = GColorPastelYellow; date_color = GColorGreen; break;
         default: battery_color = GColorMagenta; battery_text_color = GColorRichBrilliantLavender; date_color = GColorChromeYellow; break;
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
  
   char format[6];
     
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
     
     uint8_t next_char_no;
     
     switch(flag_dateFormat){
       case 0:
         if (flag_language == LANG_RUSSIAN) { // if this is Russian - need double bytes
           strftime(s_date, sizeof(s_date), "%b    %d %Y", tick_time); // "DEC 10 2015"
           strncpy(&s_date[0], LANG_MONTH[flag_language][tick_time->tm_mon], 6);
         }  else {
           
             if (flag_language != LANG_DEFAULT) { // if custom language is set
               strcpy(s_date, LANG_MONTH[flag_language][tick_time->tm_mon]);  // pull month from language array
               utf8_str_to_upper(s_date, 0); // converting month to uppercase
               next_char_no = 3;  // next date part will be inserted at byte position 3
             }  else { // otherwise use 3-char month abbreviation
               strftime(s_date, 7, "%b", tick_time); // inserting abbr. month name in current locale 
               next_char_no = utf8_str_to_upper(s_date, 3); // converting it to uppercase, limiting to 3 chars and returning byte position where next date part will be inserted
             }
           
             // addiing day and year
             strftime(&s_date[next_char_no], 9, " %d %Y", tick_time);
             
         }  
       
         break;
       case 1:
         if (flag_language == LANG_RUSSIAN) { // if this is Russian - need double bytes
           strftime(s_date, sizeof(s_date), "%d %b    %Y", tick_time); // "DEC 10 2015"
           strncpy(&s_date[3], LANG_MONTH[flag_language][tick_time->tm_mon], 6);
         }  else {
           
           strftime(s_date, 3, "%d", tick_time);  // inserting numeric day of the month
           s_date[2] = ' ';  //replacing 0 terminator with space so the string can continue
           
           if (flag_language != LANG_DEFAULT) { // if custom language is set
               strcpy(&s_date[3], LANG_MONTH[flag_language][tick_time->tm_mon]); //  pull month from language array
               utf8_str_to_upper(s_date, 0); // converting it to uppercase
               next_char_no = 7; // year will be inserted at position 7
           } else { // otherwise use 3-char month abbreviation
               strftime(&s_date[3], 7, "%b", tick_time); // inserting abbr. month name in current locale 
               next_char_no = utf8_str_to_upper(s_date, 6); // converting it to uppercase, limiting to 6 chars (3 for day + 2 for month )and returning byte position where year will be inserted
           }
           
           
          s_date[next_char_no] = ' ';  //replacing 0 terminator with space so the string can continue
          strftime(&s_date[next_char_no+1], 5, "%Y", tick_time); // inserting year part
           
          
         }  
       
         break;
       case 2:
         strftime(s_date, sizeof(s_date), "%Y-%m-%d", tick_time);  // "2015-12-10"
         break;
       
     }
     

     text_layer_set_text(date_layer, s_date);
     
     if (flag_language != LANG_DEFAULT) { // if custom language is set - pull from language array
         strcpy(s_dow, LANG_DAY[flag_language][tick_time->tm_wday]);
     } else {  
         strftime(s_dow, sizeof(s_dow), "%A", tick_time);
     }
     
     utf8_str_to_upper(s_dow, 0);
     
     #ifdef PBL_RECT  // on rectangular watches need to dynamically change DoW font based on length
      if (strlen(s_dow) >= 10) {
          text_layer_set_font(dow_layer, font_neon_18);
      } else {
          text_layer_set_font(dow_layer, font_neon_22);        
      }
     #endif

     
     
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
      case KEY_BLUETOOTH_ALERT:
         if (flag_bluetooth_alert != t->value->uint8){
              persist_write_int(KEY_BLUETOOTH_ALERT, t->value->uint8);
              flag_bluetooth_alert = t->value->uint8;
              is_buzz_enabled = 0;
              bluetooth_handler(bluetooth_connection_service_peek());
              is_buzz_enabled = 1;
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

  // Per-platform layout (Emery gets its own native 200x228 design).
  GRect logo_rect, time_rect, dow_rect, date_rect, battery_rect;
  GRect left_line_rect, right_line_rect;
  uint32_t time_font_id, dow_font_id, date_font_id, battery_font_id;

  #ifdef PBL_PLATFORM_EMERY
    logo_rect          = GRect(42, 4, 115, 30);
    time_rect          = GRect(0, 28, window_bound.size.w, 75);
    dow_rect           = GRect(0, 122, window_bound.size.w, 38);
    date_rect          = GRect(0, 163, window_bound.size.w, 38);
    battery_rect       = GRect(window_bound.size.w/2 - 30, 199, 60, 26);
    left_line_rect     = GRect(window_bound.origin.x + 4, 212, window_bound.size.w/2 - 35, 2);
    right_line_rect    = GRect(window_bound.size.w/2 + 31, 212, window_bound.size.w/2 - 35, 2);
    time_font_id       = RESOURCE_ID_NEON_72;
    dow_font_id        = RESOURCE_ID_NEON_30;
    date_font_id       = RESOURCE_ID_NEON_30;
    battery_font_id    = RESOURCE_ID_NEON_24;
  #else
    logo_rect          = PBL_IF_RECT_ELSE(GRect(14, 1, 115, 35), GRect(36, 18, 115, 35));
    time_rect          = PBL_IF_RECT_ELSE(GRect(0, 25, window_bound.size.w, 55), GRect(0, 38, window_bound.size.w, 55));
    dow_rect           = PBL_IF_RECT_ELSE(GRect(0, 90, window_bound.size.w, 30), GRect(0, 94, window_bound.size.w, 30));
    date_rect          = GRect(0, 120, window_bound.size.w, 30);
    battery_rect       = GRect(window_bound.size.w/2 - 47/2, 147, 47, 20);
    left_line_rect     = GRect(window_bound.origin.x + 4, 157, window_bound.size.w/2 - 28, 2);
    right_line_rect    = GRect(window_bound.size.w/2 + 25, 157, window_bound.size.w/2 - 30, 2);
    time_font_id       = RESOURCE_ID_NEON_53;
    dow_font_id        = RESOURCE_ID_NEON_22;
    date_font_id       = PBL_IF_RECT_ELSE(RESOURCE_ID_NEON_22, RESOURCE_ID_NEON_20);
    battery_font_id    = RESOURCE_ID_NEON_18;
  #endif

  //creating Pebble Logo
  logo_layer = bitmap_layer_create(logo_rect);
  bitmap_layer_set_compositing_mode(logo_layer, GCompOpSet); //make transparent
  logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LOGO);
  bitmap_layer_set_bitmap(logo_layer, logo_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(logo_layer));

  // creating time layer with effects
  time_layer = create_text_layer(time_rect, time_font_id, GTextAlignmentCenter);
  #ifdef PBL_COLOR
    text_layer_set_text_color(time_layer, GColorMintGreen);
  #endif

  // creating DOW layer with effects
  dow_layer = create_text_layer(dow_rect, dow_font_id, GTextAlignmentCenter);

  // creating date layer with effects
  date_layer = create_text_layer(date_rect, date_font_id, GTextAlignmentCenter);

  // creating battery layer with effects
  battery_layer = create_text_layer(battery_rect, battery_font_id, GTextAlignmentCenter);

  //creating lines for surrounding battery percentage
  left_line = bitmap_layer_create(left_line_rect);
  bitmap_layer_set_background_color(left_line, GColorWhite);
  layer_add_child(window_layer, bitmap_layer_get_layer(left_line));

  right_line = bitmap_layer_create(right_line_rect);
  bitmap_layer_set_background_color(right_line, GColorWhite);
  layer_add_child(window_layer, bitmap_layer_get_layer(right_line));

  // applying outline & blur to create "neon" effect
  #ifdef PBL_COLOR

    logo_effect = effect_layer_create(logo_rect);
    logo_offset = (EffectOffset){.orig_color = GColorWhite, .offset_color = GColorCyan, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(logo_effect, effect_outline, &logo_offset);
    effect_layer_add_effect(logo_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(logo_effect));

    time_effect = effect_layer_create(time_rect);
    time_offset = (EffectOffset){.orig_color = GColorMintGreen, .offset_color = GColorGreen, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(time_effect, effect_outline, &time_offset);
    effect_layer_add_effect(time_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(time_effect));

    dow_effect = effect_layer_create(dow_rect);
    dow_offset = (EffectOffset){.orig_color = GColorWhite, .offset_color = GColorCyan, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(dow_effect, effect_outline, &dow_offset);
    effect_layer_add_effect(dow_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(dow_effect));

    date_effect = effect_layer_create(date_rect);
    date_offset = (EffectOffset){.orig_color = GColorWhite, .offset_color = GColorChromeYellow, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(date_effect, effect_outline, &date_offset);
    effect_layer_add_effect(date_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(date_effect));

    battery_effect = effect_layer_create(GRect(0, battery_rect.origin.y, window_bound.size.w, battery_rect.size.h));
    battery_offset = (EffectOffset){.orig_color = GColorWhite, .offset_color = GColorGreen, .offset_x = 1, .offset_y = 1 };
    effect_layer_add_effect(battery_effect, effect_outline, &battery_offset);
    effect_layer_add_effect(battery_effect, effect_blur, (void *)1);
    layer_add_child(window_layer, effect_layer_get_layer(battery_effect));


     back_effect = effect_layer_create(window_bound);
     back_mask = (EffectMask){.bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_BACK), .background_color = GColorClear, .bitmap_mask = NULL, .text = NULL };
     back_mask.mask_colors = malloc(sizeof(GColor)*2);
     back_mask.mask_colors[0] = GColorBlack;
     back_mask.mask_colors[1] = GColorClear;
     effect_layer_add_effect(back_effect, effect_mask, &back_mask);
     layer_add_child(window_layer, effect_layer_get_layer(back_effect));

     #ifdef PBL_PLATFORM_EMERY
       emery_bottom_cover = bitmap_layer_create(GRect(0, 224, window_bound.size.w, 4));
       bitmap_layer_set_background_color(emery_bottom_cover, GColorBlack);
       layer_add_child(window_layer, bitmap_layer_get_layer(emery_bottom_cover));
     #endif
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
   #ifdef PBL_PLATFORM_EMERY
     bitmap_layer_destroy(emery_bottom_cover);
   #endif
}


void handle_init(void) {
  
  #ifdef PBL_RECT  // on rectangular watches need to dynamically change DoW font based on length
    #ifdef PBL_PLATFORM_EMERY
      font_neon_18 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NEON_24));
      font_neon_22 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NEON_30));
    #else
      font_neon_18 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NEON_18));
      font_neon_22 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NEON_22));
    #endif
  #endif
  
  
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
  flag_language = persist_exists(KEY_LANGUAGE)? persist_read_int(KEY_LANGUAGE) : LANG_DEFAULT;
  flag_bluetooth_alert = persist_exists(KEY_BLUETOOTH_ALERT)? persist_read_int(KEY_BLUETOOTH_ALERT) : BLUETOOTH_ALERT_WEAK;
  
  // initial bluetooth check
  is_buzz_enabled = 0;
  bluetooth_handler(bluetooth_connection_service_peek());
  is_buzz_enabled = 1;
  bluetooth_connection_service_subscribe(bluetooth_handler);
  
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
  app_message_open(500, 500); 
  
}

void handle_deinit(void) {
  window_destroy(my_window);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  app_message_deregister_callbacks();
  #ifndef PBL_SDK_2
    app_focus_service_unsubscribe();
  #endif
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
