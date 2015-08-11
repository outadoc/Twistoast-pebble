#include <pebble.h>

#define STOP_INDEX_LONG_JUMP 3
#define LONG_CLICK_DELAY 800
#define SCHEDULE_STR_SIZE 32

//bus stop info structure
typedef struct {
	char *line;
	char *stop;
	char *direction;
	int32_t schedule1;
	int32_t schedule2;
    char *schedule1_dir;
    char *schedule2_dir;
} StopInfo;


void display_schedule_info(StopInfo);
void display_status_message();
void clear_status_message();
void set_example_stop_data();
void get_schedule_info();
void clear_labels();
void app_message_sent_fail_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context);
void app_message_received_fail_handler(AppMessageResult reason, void *context);
void app_message_received_handler(DictionaryIterator *iter, void *context);
void automatic_refresh_callback(struct tm *tick_time, TimeUnits units_changed);
void format_schedule_string(char* schedule_str, int32_t millis_before_bus, char* schedule_dir);

//enum for appmessage keys and values
enum {
	//message type key
	KEY_TWISTOAST_MESSAGE_TYPE = 0x00,
	
	//message type values
	BUS_STOP_REQUEST = 0x10,
	BUS_STOP_DATA_RESPONSE = 0x11,
	
	//stop values
	KEY_STOP_INDEX = 0x20,
	KEY_BUS_STOP_NAME = 0x21,
	KEY_BUS_DIRECTION_NAME = 0x22,
	KEY_BUS_LINE_NAME = 0x23,
	KEY_BUS_NEXT_SCHEDULE = 0x24,
    KEY_BUS_NEXT_SCHEDULE_DIR = 0x25,
	KEY_BUS_SECOND_SCHEDULE = 0x26,
    KEY_BUS_SECOND_SCHEDULE_DIR = 0x27,
	
	//will be sent as "1" if the pebble should vibrate when the message is received
	KEY_SHOULD_VIBRATE = 0x30
};


Window *window;
ActionBarLayer *actionBar;
TextLayer *txt_line, *txt_stop, *txt_direction, *txt_schedule1, *txt_schedule2, *txt_status;
GBitmap *bmp_upArrow, *bmp_downArrow, *bmp_refresh;

//current index of the displayed stop
uint8_t current_stop_index = 0;

char final_sch1[SCHEDULE_STR_SIZE];
char final_sch2[SCHEDULE_STR_SIZE];

//click up (previous item)
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index--;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "requested previous stop (%d)", current_stop_index);
	get_schedule_info();
}

//long click up (previous item - 3)
void up_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index -= STOP_INDEX_LONG_JUMP;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "requested waaay previous stop (%d)", current_stop_index);
	get_schedule_info();
}

//click down (next item)
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index++;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "requested next stop (%d)", current_stop_index);
	get_schedule_info();
}

//long click down (next item + 3)
void down_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index += STOP_INDEX_LONG_JUMP;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "requested waaay next stop (%d)", current_stop_index);
	get_schedule_info();
}

//click select (refresh current item)
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "refresh stop (%d)", current_stop_index);
	get_schedule_info();
}

//long click select (goto item 0)
void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index = 0;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "goto stop 0");
	get_schedule_info();
}

//click config
void click_config_provider(Window *window) {	
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) down_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) select_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_UP, LONG_CLICK_DELAY, (ClickHandler) up_long_click_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_DOWN, LONG_CLICK_DELAY, (ClickHandler) down_long_click_handler, NULL);
	window_long_click_subscribe(BUTTON_ID_SELECT, LONG_CLICK_DELAY, (ClickHandler) select_long_click_handler, NULL);
}

//called every minute
void automatic_refresh_callback(struct tm *tick_time, TimeUnits units_changed) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "automatic refresh (%d)", units_changed);
	get_schedule_info();
}

//displays a StopInfo structure on screen
void display_schedule_info(StopInfo info) {
	clear_status_message();

    format_schedule_string(final_sch1, info.schedule1, info.schedule1_dir);
    format_schedule_string(final_sch2, info.schedule2, info.schedule2_dir);
        
	text_layer_set_text(txt_line, info.line);
	text_layer_set_text(txt_stop, info.stop);
	text_layer_set_text(txt_direction, info.direction);
	text_layer_set_text(txt_schedule1, final_sch1);
	text_layer_set_text(txt_schedule2, final_sch2);
}

//requests current schedule info
void get_schedule_info() {
	DictionaryIterator *iter;
	
	display_status_message(0);
	app_message_outbox_begin(&iter);

	//iterator will be null on failure, so bail
	if(iter == NULL) return;

	//we're making a stop request, so add that, and also add the said index
	dict_write_int8(iter, KEY_TWISTOAST_MESSAGE_TYPE, (int8_t) BUS_STOP_REQUEST);
	dict_write_int16(iter, KEY_STOP_INDEX, (int16_t) current_stop_index);

	dict_write_end(iter);

	//sends the outbound dictionary
	app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "request sent!");
}

void format_schedule_string(char* schedule_str, int32_t millis_before_bus, char* schedule_dir) {
    int seconds = (int) millis_before_bus / 1000;
    
    if(seconds <= 0) {
        snprintf(schedule_str, SCHEDULE_STR_SIZE, "Imminent");
    } else if(seconds <= 60) {
        snprintf(schedule_str, SCHEDULE_STR_SIZE, "En approche");
    } else {
        snprintf(schedule_str, SCHEDULE_STR_SIZE, "%d minutes", seconds / 60);
    }
}

//displays a status message. 0 = loading, 1 = error
void display_status_message(int status) {
	clear_labels();
	
	if(status == 0) {
		text_layer_set_text(txt_status, "Chargement...");
	} else {
		text_layer_set_text(txt_status, "Erreur.");
	}
	
	layer_set_hidden((Layer*) txt_status, 0);
}

//clears status message
void clear_status_message() {
	layer_set_hidden((Layer*) txt_status, 1);
}

//clears everything (except status label)
void clear_labels() {
	text_layer_set_text(txt_line, "");
	text_layer_set_text(txt_stop, "");
	text_layer_set_text(txt_direction, "");
	text_layer_set_text(txt_schedule1, "");
	text_layer_set_text(txt_schedule2, "");
}

//called when a message is received
void app_message_received_handler(DictionaryIterator *iter, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "message received");
	
	Tuple* type     = dict_find(iter, KEY_TWISTOAST_MESSAGE_TYPE);
	Tuple* line     = dict_find(iter, KEY_BUS_LINE_NAME);
	Tuple* dir      = dict_find(iter, KEY_BUS_DIRECTION_NAME);
	Tuple* stop     = dict_find(iter, KEY_BUS_STOP_NAME);
	Tuple* sch1     = dict_find(iter, KEY_BUS_NEXT_SCHEDULE);
    Tuple* sch1_dir = dict_find(iter, KEY_BUS_NEXT_SCHEDULE);
	Tuple* sch2     = dict_find(iter, KEY_BUS_SECOND_SCHEDULE);
    Tuple* sch2_dir = dict_find(iter, KEY_BUS_SECOND_SCHEDULE);
	Tuple* vibrate  = dict_find(iter, KEY_SHOULD_VIBRATE);
	
	if(type != NULL && type->value->int8 == BUS_STOP_DATA_RESPONSE 
	   && line != NULL && dir != NULL && stop != NULL && sch1 != NULL && sch2 != NULL 
       && sch1_dir != NULL && sch2_dir != NULL) {
		//decide if we should make the pebble vibrate
		if(vibrate != NULL && vibrate->value->int8 == 1) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "vibrating!");
			vibes_double_pulse();
		}
		
		//display what we got
		display_schedule_info((StopInfo) {
			.line = line->value->cstring,
			.direction = dir->value->cstring,
			.stop = stop->value->cstring,
			.schedule1 = sch1->value->int32,
			.schedule2 = sch2->value->int32,
            .schedule1_dir = sch1_dir->value->cstring,
            .schedule2_dir = sch2_dir->value->cstring
		});
	} else {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "received message was invalidated");
		display_status_message(1);
	}
}

//called when message failed to be received
void app_message_received_fail_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "incoming message was dropped (reason: %d)", reason);
	display_status_message(1);
}

//called when message failed to send
void app_message_sent_fail_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "outgoing message failed to send (reason: %d)", reason);
	display_status_message(1);
}

//initialize app
void init() {
	//init window	
	window = window_create();
	window_stack_push(window, true /* Animated */);
	
	Layer *window_layer = window_get_root_layer(window);

	//set text placement
	txt_line = text_layer_create(GRect(5, 0, 140 - ACTION_BAR_WIDTH, 32));
	txt_direction = text_layer_create(GRect(5, 33, 140 - ACTION_BAR_WIDTH, 30));
	txt_stop = text_layer_create(GRect(5, 60, 140 - ACTION_BAR_WIDTH, 30));
	txt_schedule1 = text_layer_create(GRect(5, 90, 140 - ACTION_BAR_WIDTH, 28));
	txt_schedule2 = text_layer_create(GRect(5, 118, 140 - ACTION_BAR_WIDTH, 28));
	txt_status = text_layer_create(GRect(5, 58, 140 - ACTION_BAR_WIDTH, 28));
	
	//set text font
	text_layer_set_font(txt_line, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
	text_layer_set_font(txt_stop, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_font(txt_direction, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_font(txt_schedule1, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	text_layer_set_font(txt_schedule2, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	text_layer_set_font(txt_status, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	
	//add text to window
	layer_add_child(window_layer, text_layer_get_layer(txt_line));
	layer_add_child(window_layer, text_layer_get_layer(txt_direction));
	layer_add_child(window_layer, text_layer_get_layer(txt_stop));
	layer_add_child(window_layer, text_layer_get_layer(txt_schedule1));
	layer_add_child(window_layer, text_layer_get_layer(txt_schedule2));
	layer_add_child(window_layer, text_layer_get_layer(txt_status));
	
	//load bitmaps
	bmp_upArrow = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ARROW_UP);
	bmp_downArrow = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ARROW_DOWN);
	bmp_refresh = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_REFRESH);

	//initialize action bar
	actionBar = action_bar_layer_create();
	action_bar_layer_add_to_window(actionBar, window);
	action_bar_layer_set_click_config_provider(actionBar, (ClickConfigProvider) click_config_provider);
	
	//initialize click handlers
	action_bar_layer_set_icon(actionBar, BUTTON_ID_UP, bmp_upArrow);
	action_bar_layer_set_icon(actionBar, BUTTON_ID_SELECT, bmp_refresh);
	action_bar_layer_set_icon(actionBar, BUTTON_ID_DOWN, bmp_downArrow);
	
	//get bus index in persistant memory
	current_stop_index = persist_read_int(KEY_STOP_INDEX);
	
	//initialize app message handlers
	app_message_register_inbox_received(app_message_received_handler); 
	app_message_register_inbox_dropped(app_message_received_fail_handler);
	app_message_register_outbox_failed(app_message_sent_fail_handler);
	
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	//register automatic refresh to refresh every minute (will also fire once it's registered)
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
	
	gbitmap_destroy(bmp_upArrow);
	gbitmap_destroy(bmp_downArrow);
	gbitmap_destroy(bmp_refresh);
	
	action_bar_layer_destroy(actionBar);
	app_message_deregister_callbacks();
	window_destroy(window);
}

int main(void) {
	init();
 	app_event_loop();
  	deinit();
}
