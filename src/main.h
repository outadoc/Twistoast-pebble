#include <pebble.h>

#define STOP_INDEX_LONG_JUMP 3
#define LONG_CLICK_DELAY 800
#define SCHEDULE_STR_SIZE 32
    
#define DISPLAY_WIDTH 144
#define DISPLAY_HEIGHT 168

// Bus stop info structure
typedef struct {
	char *line;
	char *stop;
	char *direction;
	int32_t schedule1;
	int32_t schedule2;
    char *schedule1_dir;
    char *schedule2_dir;
    int32_t color;
} StopInfo;

// Enum for appmessage keys and values
enum {
	// Message type key
	KEY_TWISTOAST_MESSAGE_TYPE = 0x00,
	
	// Message type values
	BUS_STOP_REQUEST = 0x10,
	BUS_STOP_DATA_RESPONSE = 0x11,
	
	// Stop values
	KEY_STOP_INDEX = 0x20,
	KEY_BUS_STOP_NAME = 0x21,
	KEY_BUS_DIRECTION_NAME = 0x22,
	KEY_BUS_LINE_NAME = 0x23,
	KEY_BUS_NEXT_SCHEDULE = 0x24,
    KEY_BUS_NEXT_SCHEDULE_DIR = 0x25,
	KEY_BUS_SECOND_SCHEDULE = 0x26,
    KEY_BUS_SECOND_SCHEDULE_DIR = 0x27,
	
	// Will be sent as "1" if the pebble should vibrate when the message is received
	KEY_SHOULD_VIBRATE = 0x30,
    KEY_BACKGROUND_COLOR = 0x31
};

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
