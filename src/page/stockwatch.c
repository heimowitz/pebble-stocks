#include "util.h"
#include "pebble.h"
#include "stock_info.h"
#include "stock_list.h"
#include "stock_t.h"

#define NUM_MENU_SECTIONS 1

static Window *window;

enum {
    STOCK_NAME = 0x0
};


// This is a menu layer
// You have more control than with a simple menu layer
static MenuLayer *menu_layer;

// You can draw arbitrary things in a menu item such as a background
static GBitmap *menu_background;

stock_list_t* stock_list;

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return stock_list_get_size();
}

// A callback is used to specify the height of the section header
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  // This is a define provided in pebble.h that you may use for the default height
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Here we draw what each header is
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Draw title text in the section header
  menu_cell_basic_header_draw(ctx, cell_layer, "Stocks");
}

// This is the menu item draw callback where you specify what each item should look like
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    char* symbol_name = stock_list_get_symbol(cell_index->row);
    //float difference = 32.11;
    //float percent = 12.11;
    stock_t* quote = stock_list_get_quote(cell_index->row);
    float difference = stock_t_difference(quote);
    float percent = stock_t_percent(quote);

    char diff_string[25];
    char percent_string[25];
    char str[50];
    print_float(diff_string, 25, difference, true);
    print_float(percent_string, 25, percent, true);
    snprintf(str, 50, "%s (%s%%)",diff_string,percent_string);
    menu_cell_basic_draw(ctx, cell_layer,symbol_name, str,NULL);
}

// Here we capture when a user selects a menu item
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  page_stock_info_show(cell_index->row);
}

// This initializes the menu upon window load
void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  menu_layer = menu_layer_create(bounds);
/*
  ui.text_symbol = text_layer_create((GRect) {
         .origin = { 8, 8 },
         .size = { bounds.size.w-16, 20 }
   });
   text_layer_set_text(ui.text_symbol, "No Stocks");
*/
  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(menu_layer, window);

  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}


void in_received_handler(DictionaryIterator *received, void *context) {
    // incoming message received
    Tuple *stocks_tuple = dict_find(received, STOCK_NAME);
    //Tuple *passwd_tuple = dict_find(received, CONFIG_PASSWD);
    //text_layer_set_text(ui.text_symbol, "");
    stock_list_set_symbols(stocks_tuple->value->cstring);
    menu_layer_destroy(menu_layer);
    Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
    menu_layer = menu_layer_create(bounds);
/*
  ui.text_symbol = text_layer_create((GRect) {
         .origin = { 8, 8 },
         .size = { bounds.size.w-16, 20 }
   });
   text_layer_set_text(ui.text_symbol, "No Stocks");
*/
  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(menu_layer, window);

  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
    
}


void window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(menu_layer);

  // And cleanup the background
  gbitmap_destroy(menu_background);
}

int main(void) {
  page_stock_info_init();
  app_message_register_inbox_received(in_received_handler);
     const uint32_t inbound_size = 64;
   const uint32_t outbound_size = 64;
   app_message_open(inbound_size, outbound_size);
  
  window = window_create();

  // Setup the window handlers
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true );


  app_event_loop();

  window_destroy(window);
  page_stock_info_deinit();
}
