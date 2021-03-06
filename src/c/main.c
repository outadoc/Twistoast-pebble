#include <pebble.h>
#include "main.h"

Window *window;
TextLayer *txt_line, *txt_stop, *txt_direction, *txt_schedule1, *txt_schedule2, *txt_status, *txt_header_bg;

// Current index of the displayed stop
uint8_t current_stop_index = 0;

// Final formatted schedule strings
char final_sch1[SCHEDULE_STR_SIZE];
char final_sch2[SCHEDULE_STR_SIZE];

// Click up (previous item)
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index--;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "requested previous stop (%d)", current_stop_index);
	get_schedule_info();
}

// Long click up (previous item - 3)
void up_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index -= STOP_INDEX_LONG_JUMP;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "requested waaay previous stop (%d)", current_stop_index);
	get_schedule_info();
}

// Click down (next item)
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index++;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "requested next stop (%d)", current_stop_index);
	get_schedule_info();
}

// Long click down (next item + 3)
void down_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index += STOP_INDEX_LONG_JUMP;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "requested waaay next stop (%d)", current_stop_index);
	get_schedule_info();
}

// Click select (refresh current item)
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "refresh stop (%d)", current_stop_index);
	get_schedule_info();
}

// Long click select (goto item 0)
void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index = 0;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "goto stop 0");
	get_schedule_info();
}

// Click config
void click_config_provider(Window *window) {	
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) down_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) select_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_UP, LONG_CLICK_DELAY, (ClickHandler) up_long_click_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_DOWN, LONG_CLICK_DELAY, (ClickHandler) down_long_click_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_SELECT, LONG_CLICK_DELAY, (ClickHandler) select_long_click_handler, NULL);
}

// Called every minute
void automatic_refresh_callback(struct tm *tick_time, TimeUnits units_changed) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "automatic refresh (%d)", units_changed);
	get_schedule_info();
}

// Displays a StopInfo structure on screen
void display_schedule_info(StopInfo info) {
	clear_status_message();

    format_schedule_string(final_sch1, info.schedule1, info.schedule1_dir);
    format_schedule_string(final_sch2, info.schedule2, info.schedule2_dir);
        
	text_layer_set_text(txt_line, info.line);
	text_layer_set_text(txt_stop, info.stop);
	text_layer_set_text(txt_direction, info.direction);
	text_layer_set_text(txt_schedule1, final_sch1);
	text_layer_set_text(txt_schedule2, final_sch2);
    
    #ifdef PBL_COLOR
        if(info.color == 0x00) {
            text_layer_set_background_color(txt_header_bg, GColorBlack);
    
            text_layer_set_text_color(txt_line, GColorWhite);
            text_layer_set_text_color(txt_stop, GColorWhite);
            text_layer_set_text_color(txt_direction, GColorWhite);
        } else {
            text_layer_set_background_color(txt_header_bg, GColorFromHEX(info.color));
    
            text_layer_set_text_color(txt_line, GColorBlack);
            text_layer_set_text_color(txt_stop, GColorBlack);
            text_layer_set_text_color(txt_direction, GColorBlack);
        }
    #endif
}

// Requests current schedule info
void get_schedule_info() {
	DictionaryIterator *iter;
	
	display_status_message(0);
	app_message_outbox_begin(&iter);

	// Iterator will be null on failure, so bail
	if(iter == NULL) return;

	// We're making a stop request, so add that, and also add the said index
	dict_write_int8(iter, KEY_TWISTOAST_MESSAGE_TYPE, (int8_t) BUS_STOP_REQUEST);
	dict_write_int16(iter, KEY_STOP_INDEX, (int16_t) current_stop_index);

	dict_write_end(iter);

	// Sends the outbound dictionary
	app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "request sent!");
}

void format_schedule_string(char* schedule_str, int32_t millis_before_bus, char* schedule_dir) {
    int seconds = (int) millis_before_bus / 1000;
    
    // Reset the schedule string
    schedule_str[0] = '\0';
        
    // If schedule_dir[0] is a capital letter, add it to the front of the string (it's the line)
    if(schedule_dir[0] >= 65 && schedule_dir[0] <= 90) {
        schedule_str[0] = schedule_dir[0];
        schedule_str[1] = ':';
        schedule_str[2] = ' ';
        schedule_str[3] = '\0';
    }
    
    // Get the actual absolute time of the schedule
    time_t stop_time = time(NULL) + seconds;
    struct tm *stop_time_info = localtime(&stop_time);
        
    if(millis_before_bus == -1) {
        // No stop for this bus
        strncpy(schedule_str, "Pas d'arrêt", SCHEDULE_STR_SIZE);
    } else if(seconds <= 0) {
        // Currently at stop
        strncat(schedule_str, "À l'arrêt", SCHEDULE_STR_SIZE);
    } else if(seconds <= 60) {
        // Bus is incoming
        strncat(schedule_str, "Imminent", SCHEDULE_STR_SIZE);
    } else if(seconds > 3600) {
        // Display absolute time because the bus isn't coming any time soon
        char time_str[] = "00:00 AM";
        
        if(clock_is_24h_style()) {
            // Display as 15:00
            strftime(time_str, sizeof(time_str), "%R", stop_time_info);
        } else {
            // Display as 03:00 PM
            strftime(time_str, sizeof(time_str), "%I:%M %p", stop_time_info);
        }
        
        strncpy(schedule_str, time_str, SCHEDULE_STR_SIZE);
    } else {
        // Just display a countdown!
        // Write the remaining minutes to time_str
        char time_str[] = "00 ";
        snprintf(time_str, sizeof(time_str), "%d ", seconds / 60);
        
        // Copy time_str to schedule_str ("A: 00 ")
        strncat(schedule_str, time_str, SCHEDULE_STR_SIZE);
        
        // Add "minutes" at the end of the string
        strncat(schedule_str, "minutes", SCHEDULE_STR_SIZE);
    }
}

// Displays a status message. 0 = loading, 1 = error
void display_status_message(int status) {
	clear_labels();
	
	if(status == 0) {
		text_layer_set_text(txt_status, "Chargement\u2026");
	} else {
		text_layer_set_text(txt_status, "Erreur. \U0001F628");
	}
	
	layer_set_hidden((Layer*) txt_status, 0);
}

// Clears status message
void clear_status_message() {
	layer_set_hidden((Layer*) txt_status, 1);
}

// Clears everything (except status label)
void clear_labels() {
	text_layer_set_text(txt_line, "");
	text_layer_set_text(txt_stop, "");
	text_layer_set_text(txt_direction, "");
	text_layer_set_text(txt_schedule1, "");
	text_layer_set_text(txt_schedule2, "");
}

// Called when a message is received
void app_message_received_handler(DictionaryIterator *iter, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "message received");
	
    // Retrieve all needed data
	Tuple* type     = dict_find(iter, KEY_TWISTOAST_MESSAGE_TYPE);
	Tuple* line     = dict_find(iter, KEY_BUS_LINE_NAME);
	Tuple* dir      = dict_find(iter, KEY_BUS_DIRECTION_NAME);
	Tuple* stop     = dict_find(iter, KEY_BUS_STOP_NAME);
	Tuple* sch1     = dict_find(iter, KEY_BUS_NEXT_SCHEDULE);
    Tuple* sch1_dir = dict_find(iter, KEY_BUS_NEXT_SCHEDULE_DIR);
	Tuple* sch2     = dict_find(iter, KEY_BUS_SECOND_SCHEDULE);
    Tuple* sch2_dir = dict_find(iter, KEY_BUS_SECOND_SCHEDULE_DIR);
	Tuple* vibrate  = dict_find(iter, KEY_SHOULD_VIBRATE);
    Tuple* color    = dict_find(iter, KEY_BACKGROUND_COLOR);
	
	if(type != NULL && type->value->int8 == BUS_STOP_DATA_RESPONSE 
	   && line != NULL && dir != NULL && stop != NULL && sch1 != NULL && sch2 != NULL 
       && sch1_dir != NULL && sch2_dir != NULL) {
		// Decide if we should make the pebble vibrate
		if(vibrate != NULL && vibrate->value->int8 == 1) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "vibrating!");
			vibes_double_pulse();
		}
		
		// Display what we got
		display_schedule_info((StopInfo) {
			.line = line->value->cstring,
			.direction = dir->value->cstring,
			.stop = stop->value->cstring,
			.schedule1 = sch1->value->int32,
			.schedule2 = sch2->value->int32,
            .schedule1_dir = sch1_dir->value->cstring,
            .schedule2_dir = sch2_dir->value->cstring,
            .color = color->value->int32
		});
	} else {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "received message was invalidated");
		display_status_message(1);
	}
}

// Called when message failed to be received
void app_message_received_fail_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "incoming message was dropped (reason: %d)", reason);
	display_status_message(1);
}

// Called when message failed to send
void app_message_sent_fail_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "outgoing message failed to send (reason: %d)", reason);
	display_status_message(1);
}

// Initialize app
void init() {
	// Init window	
	window = window_create();
	window_stack_push(window, true);
	
	Layer *window_layer = window_get_root_layer(window);

	// Set text placement
	txt_line = text_layer_create(GRect(5, 0, DISPLAY_WIDTH - 5, 32));
    txt_stop = text_layer_create(GRect(5, 30, DISPLAY_WIDTH - 5, 35));
	txt_direction = text_layer_create(GRect(5, 57, DISPLAY_WIDTH - 5, 35));
	txt_schedule1 = text_layer_create(GRect(10, 95, DISPLAY_WIDTH - 10, 35));
	txt_schedule2 = text_layer_create(GRect(10, 125, DISPLAY_WIDTH - 10, 35));
	txt_status = text_layer_create(GRect(5, 115, DISPLAY_WIDTH - 5, 28));
    
    // Text layer used as a header background to give the top half some color
    txt_header_bg = text_layer_create(GRect(0, 0, DISPLAY_WIDTH, 93));
	
	// Set text font
	text_layer_set_font(txt_line, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_font(txt_stop, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_font(txt_direction, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_font(txt_schedule1, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_font(txt_schedule2, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_font(txt_status, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	
	// Add text to window
    layer_add_child(window_layer, text_layer_get_layer(txt_header_bg));
	layer_add_child(window_layer, text_layer_get_layer(txt_line));
	layer_add_child(window_layer, text_layer_get_layer(txt_stop));
    layer_add_child(window_layer, text_layer_get_layer(txt_direction));
	layer_add_child(window_layer, text_layer_get_layer(txt_schedule1));
	layer_add_child(window_layer, text_layer_get_layer(txt_schedule2));
	layer_add_child(window_layer, text_layer_get_layer(txt_status));
    
    // Set text layer backgrounds to be transparent, so that we can see the window background color
    text_layer_set_background_color(txt_line, GColorClear);
    text_layer_set_background_color(txt_stop, GColorClear);
    text_layer_set_background_color(txt_direction, GColorClear);

    // If we're not on a color Pebble, display the header as white on black
    #ifndef PBL_COLOR
        text_layer_set_background_color(txt_header_bg, GColorBlack);
    
        text_layer_set_text_color(txt_line, GColorWhite);
        text_layer_set_text_color(txt_stop, GColorWhite);
        text_layer_set_text_color(txt_direction, GColorWhite);
    #endif
	
    // Set click listener
	window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider);
	
	// Get bus index in persistant memory
	current_stop_index = persist_read_int(KEY_STOP_INDEX);
	
	// Initialize app message handlers
	app_message_register_inbox_received(app_message_received_handler); 
	app_message_register_inbox_dropped(app_message_received_fail_handler);
	app_message_register_outbox_failed(app_message_sent_fail_handler);
	
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	// Register automatic refresh to refresh every minute (will also fire once it's registered)
	tick_timer_service_subscribe(MINUTE_UNIT, automatic_refresh_callback);
}

void deinit(void) {
	persist_write_int(KEY_STOP_INDEX, current_stop_index);
	
  	text_layer_destroy(txt_line);
	text_layer_destroy(txt_direction);
	text_layer_destroy(txt_stop);
	text_layer_destroy(txt_schedule1);
	text_layer_destroy(txt_schedule2);
	text_layer_destroy(txt_status);

	app_message_deregister_callbacks();
	window_destroy(window);
}

int main(void) {
	init();
 	app_event_loop();
  	deinit();
}