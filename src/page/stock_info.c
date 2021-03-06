#include <pebble.h>
#include "stock_info.h"
#include "stock_getter.h"
#include "stock_list.h"
#include "util.h"

#define GRAPH_SIZE 8

/*
 * Static Function Prototypes
 */

static void translate_graph(void);
static void translate_point(GPoint* point);
static void graph_draw(struct Layer* layer, GContext* ctx);
static void update_text(void);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);
static void window_load(Window *window);
static void window_unload(Window *window);
static void callback(char* symbol, stock_t* info);
static void hist_callback(char* symbol, stock_t* info);


/*
 * Static Variables
 */

//Default text to display for stock info
static char* text_info_default =  "OP: ------    HI: ------ \nCL: ------    LO: ------";

//Struct containing UI elements
static struct {
   Window* window;
   TextLayer* text_symbol;
   TextLayer* text_value_diff;
   TextLayer* text_value_info;
   TextLayer* background_workaround;
} ui;

//Strings for each text layer
static char str_symbol[20];
static char str_value_diff[20];
static char str_value_info[80];

//Current symbol index
static int current_index;

//Stock info to store data
static stock_t stock_info;

//Struct containing graph info
static struct {
   float values[GRAPH_SIZE];
   GPoint points[GRAPH_SIZE];
   GRect bounds;
   float miny, maxy;
   bool draw;
} graph;


/*
 * API Function Definitions
 */

/*Initialize the stock info page*/
void page_stock_info_init(void)
{
   //Create window
   ui.window = window_create();

   window_set_click_config_provider(ui.window, click_config_provider);
   window_set_window_handlers(ui.window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
   });

   stock_info.valid = false;
}

/*Destroy the stock info page*/
void page_stock_info_deinit(void)
{
   window_destroy(ui.window);
   ui.window = NULL;
}

/*Show the stock info page window*/
void page_stock_info_show(int index)
{
  window_stack_push (ui.window, true); 
  page_stock_info_set_symbol_index(index);
}

/*Sets the symbol for the stock page*/
void page_stock_info_set_symbol_index(int index)
{
   int size = stock_list_get_size();
   if (index < 0)
      index = size-1;
   else if (index >= size)
      index = 0;
   char* symbol_name = stock_list_get_symbol(index);
   current_index = index;
   

   strncpy(str_symbol, symbol_name, SYMBOL_SIZE);
   text_layer_set_text(ui.text_symbol, str_symbol);
   stock_info.valid = true;
   page_stock_info_update();
}


/*Updates the information for the current stock symbol*/
void page_stock_info_update(void)
{
   if (stock_info.valid)
   {
      get_stock_info(str_symbol, &stock_info, callback);
      text_layer_set_text(ui.text_value_diff, "Loading");
      text_layer_set_text(ui.text_value_info, text_info_default);
      graph.draw = false;
      return;
   }
   //Something bad happened
   text_layer_set_text(ui.text_value_diff, "Invalid Symbol");
   graph.draw = false;
}

/*
 * Static Function Definitions
 */


static void translate_graph(void)
{
   int i;
   graph.maxy = graph.values[0];
   graph.miny = graph.values[0];
   for(i=1;i<GRAPH_SIZE;i++)
   {
      if (graph.maxy < graph.values[i])
         graph.maxy = graph.values[i];
      if (graph.miny > graph.values[i])
         graph.miny = graph.values[i];
   }
   if (graph.miny == graph.maxy)
      graph.draw = false;
   else
      for(i=0;i<GRAPH_SIZE;i++)
      {
         //Crazy math stuff. Don't worry about it
         graph.points[i].x = graph.bounds.origin.x +
                     i*graph.bounds.size.w/(GRAPH_SIZE-1);

         graph.points[i].y = graph.bounds.origin.y+graph.bounds.size.h-4 -
                     (int)((graph.values[i]-graph.miny)/(graph.maxy-graph.miny)*(graph.bounds.size.h-8));
      }
}

static void translate_point(GPoint* point)
{
   point->x = graph.bounds.origin.x +
               point->x*graph.bounds.size.w/(GRAPH_SIZE-1);
   point->y = graph.bounds.origin.y+graph.bounds.size.h-4 -
               (int)((point->y-graph.miny)/(graph.maxy-graph.miny)*(graph.bounds.size.h-8));
}

static void graph_draw(struct Layer* layer, GContext* ctx)
{
   graphics_draw_rect(ctx, (GRect) {.origin=graph.bounds.origin, .size= {.w=graph.bounds.size.w+1, .h=graph.bounds.size.h+1}});

   int i;
   GPoint p, p1, p2;
   for(i=0;i<GRAPH_SIZE-1;i++)
      graphics_draw_line(ctx, graph.points[i], graph.points[i+1]);

   graphics_context_set_fill_color(ctx, GColorBlack);
   graphics_context_set_stroke_color(ctx, GColorBlack);

   char label[10];

   for(i=0;i<GRAPH_SIZE-1;i++)
   {
      p.x = i+1; p.y = graph.miny;
      translate_point(&p);
      p1.x = p.x; p1.y = p.y+2;
      p2.x = p.x; p2.y = p.y-2;
      graphics_draw_line(ctx, p1, p2);
      snprintf(label, sizeof(label), "%d", GRAPH_SIZE-i);
      /*graphics_draw_text(ctx, label,
         fonts_get_system_font(FONT_KEY_GOTHIC_14),
         (GRect){
            .origin = {p.x-3, p1.y},
            .size = {30, 30}
         }, GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);*/
   }

   graphics_draw_text(ctx, "Hello", fonts_get_system_font(FONT_KEY_GOTHIC_14),
      (GRect) {
         .origin = {5, 20},
         .size = {80, 30}
      },
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void update_text(void)
{
   float percent = stock_t_percent(&stock_info);
   float difference = stock_t_difference(&stock_info);

   char str_percent[10];
   char str_difference[10];

   print_float(str_percent, sizeof(str_percent), percent, true);
   print_float(str_difference, sizeof(str_difference), difference, true);


   snprintf(str_value_diff, sizeof(str_value_diff),
            "%s (%s%%)", str_difference, str_percent);
   char str_op[10];
   char str_hi[10];
   char str_lo[10];
   char str_cl[10];

   print_float(str_op, sizeof(str_op), stock_info.open, false);
   print_float(str_hi, sizeof(str_hi), stock_info.high, false);
   print_float(str_lo, sizeof(str_lo), stock_info.low, false);
   print_float(str_cl, sizeof(str_cl), stock_info.current, false);

   snprintf(str_value_info, sizeof(str_value_info),
            "OP: %6s    HI: %6s\nCL: %6s    LO: %6s", str_op, str_hi, str_cl, str_lo);
   text_layer_set_text(ui.text_value_diff, str_value_diff);
   text_layer_set_text(ui.text_value_info, str_value_info);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context)
{
   page_stock_info_set_symbol_index(current_index-1);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context)
{
   page_stock_info_set_symbol_index(current_index+1);
}

static void click_config_provider(void *context)
{
   window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
   window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window)
{
   Layer *window_layer = window_get_root_layer(window);
   GRect bounds = layer_get_bounds(window_layer);

   //Stock name (at the top)
   ui.text_symbol = text_layer_create((GRect) {
         .origin = { 8, 8 },
         .size = { bounds.size.w-16, 20 }
   });
   text_layer_set_text(ui.text_symbol, "???");
   text_layer_set_text_alignment(ui.text_symbol, GTextAlignmentCenter);
   text_layer_set_background_color(ui.text_symbol, GColorBlack);
   text_layer_set_text_color(ui.text_symbol, GColorWhite);


   //Info stuff
   ui.text_value_info = text_layer_create((GRect) {
         .origin = { 8, 100 },
         .size = { bounds.size.w-16, 30 }
   });
   text_layer_set_text(ui.text_value_info, text_info_default);
   text_layer_set_text_alignment(ui.text_value_info, GTextAlignmentCenter);

   //Stock value (at the bottom)
   ui.text_value_diff = text_layer_create((GRect) {
         .origin = { 8, bounds.size.h-24 },
         .size = { bounds.size.w-16, 20 }
   });
   text_layer_set_text(ui.text_value_diff, "-----");
   text_layer_set_text_alignment(ui.text_value_diff, GTextAlignmentCenter);
   text_layer_set_font(ui.text_value_diff, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));

   ui.background_workaround = text_layer_create(bounds);

   graph.bounds = (GRect) {
      .origin = { 8, 30 },
      .size = { 144-16, 50 }
   };

   
   layer_add_child(window_layer, text_layer_get_layer(ui.background_workaround));
   layer_add_child(window_layer, text_layer_get_layer(ui.text_symbol));
   layer_add_child(window_layer, text_layer_get_layer(ui.text_value_diff));
   layer_add_child(window_layer, text_layer_get_layer(ui.text_value_info));
   layer_set_update_proc(text_layer_get_layer(ui.background_workaround), graph_draw);
}

static void window_unload(Window *window)
{
   //Destroy all UI elements
   text_layer_destroy(ui.text_symbol);
   text_layer_destroy(ui.text_value_diff);
   text_layer_destroy(ui.text_value_info);
   text_layer_destroy(ui.background_workaround);

   //Set to NULL (just in case)
   ui.text_symbol             = NULL;
   ui.text_value_diff         = NULL;
   ui.text_value_info         = NULL;
   ui.background_workaround   = NULL;
}

static void callback(char* symbol, stock_t* info)
{
   update_text();
}
static void hist_callback(char* symbol, stock_t* info)
{
   translate_graph();
   graph.draw = true;
}
