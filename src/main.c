#include <pebble.h>

//message type key
#define TWISTOAST_MESSAGE_TYPE 0x00

//message type value
#define BUS_STOP_REQUEST 0x10
#define BUS_STOP_DATA_RESPONSE 0x11

//message keys
#define BUS_INDEX 0x20
#define BUS_STOP_NAME 0x21
#define BUS_DIRECTION_NAME 0x22
#define BUS_LINE_NAME 0x23
#define BUS_NEXT_SCHEDULE 0x24
#define BUS_SECOND_SCHEDULE 0x25

typedef struct {
	char *line;
	char *stop;
	char *direction;
	char *schedule1;
	char *schedule2;
} StopInfo;

void display_schedule_info(StopInfo);
void display_status_message();
void clear_status_message();
void set_example_stop_data();
void get_schedule_info();
void clear_labels();

Window *window;
ActionBarLayer *actionBar;
TextLayer *txt_line, *txt_stop, *txt_direction, *txt_schedule1, *txt_schedule2, *txt_status;
GBitmap *bmp_upArrow, *bmp_downArrow, *bmp_refresh;

int16_t current_stop_index = 0;

//click up (previous item)
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	if(current_stop_index-1 >= 0) {
		current_stop_index--;
		APP_LOG(APP_LOG_LEVEL_DEBUG, "requested previous stop (%d)", current_stop_index);
		get_schedule_info();
	}
}

//click down (next item)
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	current_stop_index++;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "requested next stop (%d)", current_stop_index);
	get_schedule_info();
}

//click select (refresh current item)
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "refresh stop (%d)", current_stop_index);
	get_schedule_info();
}

//click config
void click_config_provider(Window *window) {	
	window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) up_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, (ClickHandler) down_single_click_handler);
	window_single_click_subscribe(BUTTON_ID_SELECT, (ClickHandler) select_single_click_handler);
}

void display_schedule_info(StopInfo info) {
	clear_status_message();
	
	text_layer_set_text(txt_line, info.line);
	text_layer_set_text(txt_stop, info.stop);
	text_layer_set_text(txt_direction, info.direction);
	text_layer_set_text(txt_schedule1, info.schedule1);
	text_layer_set_text(txt_schedule2, info.schedule2);
}

void get_schedule_info() {
	display_status_message(0);
	
	DictionaryIterator *iter;
	
    //open up the message outbox for writing
    app_message_outbox_begin(&iter);

    //iterator will be null on failure, so bail
    if(iter == NULL)
        return;

    dict_write_int8(iter, TWISTOAST_MESSAGE_TYPE, (int8_t) BUS_STOP_REQUEST);
	dict_write_int16(iter, BUS_INDEX, (int16_t) current_stop_index);
	
    dict_write_end(iter);

    //sends the outbound dictionary
    app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "request sent!");
}

void display_status_message(int status) {
	clear_labels();
	clear_status_message();
	
	layer_set_hidden((Layer*) txt_status, 0);
	
	if(status == 0) {
		text_layer_set_text(txt_status, "Chargement...");
	} else {
		text_layer_set_text(txt_status, "Erreur.");
	}
}

void clear_status_message() {
	layer_set_hidden((Layer*) txt_status, 1);
}

void clear_labels() {
	text_layer_set_text(txt_line, "");
	text_layer_set_text(txt_stop, "");
	text_layer_set_text(txt_direction, "");
	text_layer_set_text(txt_schedule1, "");
	text_layer_set_text(txt_schedule2, "");
}

void app_message_received_handler(DictionaryIterator *iter, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "message received");
	
	Tuple* type = dict_find(iter, TWISTOAST_MESSAGE_TYPE);
	Tuple* line = dict_find(iter, BUS_LINE_NAME);
	Tuple* dir 	= dict_find(iter, BUS_DIRECTION_NAME);
	Tuple* stop = dict_find(iter, BUS_STOP_NAME);
	Tuple* sch1 = dict_find(iter, BUS_NEXT_SCHEDULE);
	Tuple* sch2 = dict_find(iter, BUS_SECOND_SCHEDULE);
	
	if(type != NULL && type->value->int8 == BUS_STOP_DATA_RESPONSE && line != NULL && dir != NULL && stop != NULL && sch1 != NULL && sch2 != NULL) {
		display_schedule_info((StopInfo) {
			.line = line->value->cstring,
			.direction = dir->value->cstring,
			.stop = stop->value->cstring,
			.schedule1 = sch1->value->cstring,
			.schedule2 = sch2->value->cstring
		});
	} else {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "received message was invalidated");
		display_status_message(1);
	}
}

void app_message_received_fail_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "incoming message was dropped (reason: %d)", reason);
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
	txt_direction = text_layer_create(GRect(5, 33, 140 - ACTION_BAR_WIDTH, 27));
	txt_stop = text_layer_create(GRect(5, 58, 140 - ACTION_BAR_WIDTH, 27));
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
		
	app_message_register_inbox_received(app_message_received_handler); 
	app_message_register_inbox_dropped(app_message_received_fail_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	get_schedule_info();
}

void deinit(void) {
  	text_layer_destroy(txt_line);
	text_layer_destroy(txt_direction);
	text_layer_destroy(txt_stop);
	text_layer_destroy(txt_schedule1);
	text_layer_destroy(txt_schedule2);
	
	gbitmap_destroy(bmp_upArrow);
	gbitmap_destroy(bmp_downArrow);
	gbitmap_destroy(bmp_refresh);
	
	action_bar_layer_destroy(actionBar);
	
	window_destroy(window);
}

int main(void) {
	init();
 	app_event_loop();
  	deinit();
}