/*

Arduino IDE tools settings:

CPU freq: 40 Mhz (trying to save battery)
Flash size: 16 MB (128 Mb)
Partition Scheme: 16 M Flash (3MB APP / 9.9 MB FATFS)
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <time.h>

int number_of_beeps = 0;

#if ARDUINO
#include <Arduino.h>
#include <lvgl.h>
#include "globals.h"
#include "HWCDC.h"

#define TT_TYPE int64_t

void setBrightness(int b) {
  ((Arduino_CO5300 *)gfx)->setBrightness(b);
}

TT_TYPE get_time_millis() {
  return millis();
}

void enableAmp() {
  pinMode(PA_PIN, OUTPUT);
  digitalWrite(PA_PIN, HIGH);
}

void disableAmp() {
  pinMode(PA_PIN, OUTPUT);
  digitalWrite(PA_PIN, LOW);
}

void beep_on() {
  printf("Beep_on!\n");
  enableAmp();
  number_of_beeps = 3;
}

void beep_off() {
  printf("Beep_off!\n");
  number_of_beeps = 0;
  disableAmp();  
}

#endif

#ifdef __linux__
#include <lvgl/lvgl.h>

#define TT_TYPE int64_t

TT_TYPE get_time_millis() {
  struct timespec ts;
    
  // Use CLOCK_MONOTONIC for intervals, CLOCK_REALTIME for wall clock time
  clock_gettime(CLOCK_MONOTONIC, &ts);
    
  // Calculate total milliseconds
  TT_TYPE ms = (TT_TYPE) ((uint64_t)(ts.tv_sec) * 1000 + (uint64_t)(ts.tv_nsec) / 1000000);
  return ms;
}

void beep_on() {
  printf("Beep on\n");
  fflush(stdout);
}

void beep_off() {
  printf("Beep off\n");
  fflush(stdout);
}

int last_brightness = 0;
void setBrightness(int i) {
  if (last_brightness != i) {
    printf("brightness changed: %d -> %d\n", last_brightness, i);
  last_brightness = i;
  }
}

#endif

#define OFF 40
#define DIM 40
#define BRIGHT 100

LV_FONT_DECLARE(IBMPlexMonoBold_96)
LV_FONT_DECLARE(IBMPlexMonoBold_60)

typedef struct TT_s TT;

struct TT_s {
  TT_TYPE start_value;
  TT_TYPE accumulated;
  TT_TYPE started;
  bool running;
};

typedef struct Tab_s Tab;

typedef struct TimerTabData_s TimerTabData;

typedef struct TimerBump_s TimerBump;

typedef void (*TickFunction) (Tab*);

struct Tab_s {
  char * name;
  void * tab_data;
  TickFunction tick;
};

#define TIMER_LABEL_L 20
struct TimerTabData_s {
  Tab * tab;

  TT tt;
  
  int * beep_points;
  
  lv_obj_t * button;
  lv_obj_t * label;
  lv_obj_t * up_button;
  lv_obj_t * down_button;

  char last_mmss_text[TIMER_LABEL_L];
};

struct TimerBump_s {
  TimerTabData * timer_tab_data;
  int inc;
  int consecutive_long_presses;
};

TimerBump * makeTimerBump(TimerTabData * t, int inc) {
  TimerBump * rv = (TimerBump *) malloc(sizeof(TimerBump));
  rv->timer_tab_data = t;
  rv->inc = inc;
  rv->consecutive_long_presses = 0;
  return rv;
}

void tt_init (TT * tt, TT_TYPE start_value) {
  tt->running = false;
  tt->accumulated = 0;
  tt->start_value = start_value;
}

void tt_reset (TT * tt) {
  tt->accumulated = 0;
}

void tt_dump (TT * tt, char * s) {
#if __linux__ && 0
  printf ("TT dump: %s, running = %d, start_value = %ld, accumulated = %ld, started = %ld\n",
     s, tt->running, tt->start_value, tt->accumulated, tt->started);
  fflush(stdout);
#else
  (void) tt;
  (void) s;
#endif
}

void tt_start (TT * tt) {
  if (tt->running) return;
  tt->started = get_time_millis();
  tt->running = true;
  tt_dump(tt, "start");
}

void tt_stop (TT * tt) {
  if (!tt->running) return;
  tt_dump(tt, "stop 1");
  TT_TYPE now = get_time_millis();
  tt->accumulated += (now - tt->started);
  tt->running = false;
  tt_dump(tt, "stop 2");
}

void tt_bump (TT * tt, TT_TYPE millis) {
  if (tt->running) return;
  tt_dump(tt, "bump 1");
  tt->accumulated += millis;
  tt_dump(tt, "bump 2");
  if (tt->accumulated > tt->start_value) tt->accumulated = tt->start_value;
  tt_dump(tt, "bump 3");
}

TT_TYPE tt_get (TT * tt) {
  if (!tt->running) return tt->accumulated;
  TT_TYPE now = get_time_millis();
  return tt->accumulated + (now - tt->started);
}

TT_TYPE tt_remaining (TT * tt) {
  TT_TYPE rv = tt->start_value - tt_get(tt);
  return rv <= 0 ? 0L : rv;
}

bool is_i_in_list(int i, const int * lp) {
  int l_index = 0;
  while (true) {
    int l1 = lp[l_index];
    // printf("is %d = %d\n", i, l1);
    // fflush(stdout);
    if (l1 == -1) return false;
    if (l1 == i) return true;
    l_index++;
  }
}

Tab tabs[6];

void xxxx(char * c) {
#if 0
  for (int i = 0; i < 4; i++) {
    printf("%s: tab %d name = '%s'\n", c, i, tabs[i].name);
  }
#else
  (void) c;
#endif
}

uint32_t visible_tab_index = 0;

static void tabview_event_cb(lv_event_t * e) {
  lv_obj_t * tabview = (lv_obj_t *) lv_event_get_target(e);
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_VALUE_CHANGED) {
    visible_tab_index = lv_tabview_get_tab_act(tabview);
    
    Tab visibleTab = tabs[visible_tab_index];
    visibleTab.tick(&visibleTab);
    
    printf("\nChanged to tab %d\n", visible_tab_index);
    fflush(stdout);
  }
}

char * event_name (int code) {
  // static char event_name_other[20];
  if (code == LV_EVENT_LONG_PRESSED) {
    return "LV_EVENT_LONG_PRESSED";
  } else if (code == LV_EVENT_PRESSING) {
    return "LV_EVENT_LONG_PRESSING";
  } else if (code == LV_EVENT_SHORT_CLICKED) {
    return "LV_EVENT_SHORT_CLICKED";
  } else if (code == LV_EVENT_SINGLE_CLICKED) {
    return "LV_EVENT_SINGLE_CLICKED";
  } else if (code == LV_EVENT_DOUBLE_CLICKED) {
    return "LV_EVENT_DOUBLE_CLICKED";
  } else if (code == LV_EVENT_TRIPLE_CLICKED) {
    return "LV_EVENT_TRIPLE_CLICKED";
  } else if (code == LV_EVENT_LONG_PRESSED) {
    return "LV_EVENT_LONG_PRESSED";
  } else if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
    return "LV_EVENT_LONG_PRESSED_REPEAT";
  } else if (code == LV_EVENT_CLICKED) {
    return "LV_EVENT_CLICKED";
  } else if (code == LV_EVENT_RELEASED) {
    return "LV_EVENT_RELEASED";
  } else if (code == LV_EVENT_HOVER_OVER) {
    return "LV_EVENT_HOVER_OVER";
  } else if (code == LV_EVENT_HOVER_LEAVE) {
    return "LV_EVENT_HOVER_LEAVE";
  } else {
    //snprintf(event_name_other, sizeof(event_name_other), "Other: %d", code);
    //return event_name_other; 
    return "?";
  }
}

static void timer_button_event_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  // lv_obj_t * obj = lv_event_get_target(e);
  if (code > LV_EVENT_HOVER_LEAVE) {
    return;
  }

  Tab * tab = (Tab *) lv_event_get_user_data(e);
  TimerTabData * timer_tab_data = (TimerTabData *) tab->tab_data;
  
  printf("*** Button event: code=%s, tab->name='%s'\n", event_name(code), tab->name);
  
  if (code == LV_EVENT_SINGLE_CLICKED) {
    if ((timer_tab_data->tt).running) {
      tt_stop(&(timer_tab_data->tt));
    } else {
      TT_TYPE remaining = tt_remaining(&(timer_tab_data->tt));
      if (!remaining) {
        tt_reset(&(timer_tab_data->tt));
      }
      tt_start(&(timer_tab_data->tt));
    }
    tab->tick(tab);
  } else if (code == LV_EVENT_LONG_PRESSED) {
    if (!(timer_tab_data->tt).running) {
      tt_reset(&(timer_tab_data->tt));
    }
    tab->tick(tab);
  }
}

static void timer_bump_button_event_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  // lv_obj_t * obj = lv_event_get_target(e);
  if (code > LV_EVENT_HOVER_LEAVE) {
    return;
  }

  TimerBump * bump = (TimerBump *) lv_event_get_user_data(e);
  TimerTabData * timer_tab_data = bump->timer_tab_data;
  Tab * tab = timer_tab_data->tab;
  
  if (timer_tab_data->tt.running) return;
  // printf("Bump button event: %s, %s %d\n", event_name(code), timer->tab->name, bump->inc);
  
  int amount = bump->inc;
  
  if (code == LV_EVENT_LONG_PRESSED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
    bump->consecutive_long_presses++;
    if (bump->consecutive_long_presses > 5) {
      amount = amount * 6;
    }
    // printf("Consecutive = %d, amount = %d\n", bump->consecutive_long_presses, amount);  
  } else {
    bump->consecutive_long_presses = 0;
    // printf("Reset: Consecutive = %d, amount = %d\n", bump->consecutive_long_presses, amount);  
  }
  
  tt_bump(&timer_tab_data->tt, -(amount * 1000));
  tab->tick(tab);
}

void timer_tab_tick(Tab * tab) {
  TimerTabData * timer_tab_data = (TimerTabData *) (tab -> tab_data);
  TT_TYPE remaining_ms = tt_remaining(&timer_tab_data->tt);

  //xxxx("pretick");
  
  int rs = remaining_ms / 1000;
  if (remaining_ms > 0 && rs == 0) rs = 1;
  if (remaining_ms <= 0) rs = 0;
  
  int mm = rs / 60;
  int ss = rs % 60;
  
  if (timer_tab_data->tt.running) {
    printf("checking %d to see if I should beep... ", rs);
    bool should_beep = is_i_in_list(rs, timer_tab_data->beep_points);
    printf("%s\n", should_beep ? "yep" : "nope");
    if (should_beep) beep_on();
  }

  if (rs == 0) {
    tt_stop(&timer_tab_data->tt);
  }

  if (timer_tab_data->tt.running) {
    setBrightness(BRIGHT);
  } else {
    setBrightness(DIM);
  }

  char label[TIMER_LABEL_L];
  snprintf(label, sizeof(label), "%.2d:%.2d", mm, ss);
  // only update if necessary
  if (strcmp(label, timer_tab_data->last_mmss_text) != 0) {
    lv_label_set_text(timer_tab_data->label, label);
    memcpy(&timer_tab_data->last_mmss_text, label, TIMER_LABEL_L);
  }

  //xxxx("posttick");
}

void setup_timer_tab (lv_obj_t * tabview, Tab * tab, char * title, int start_seconds, const int * beep_points) {
  lv_obj_t * lv_tabview_tab = lv_tabview_add_tab(tabview, title);
  lv_obj_set_style_bg_opa(lv_tabview_tab, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(lv_tabview_tab, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
  
  tab->tick = timer_tab_tick;
  tab->name = title;
  
  tab->tab_data = malloc(sizeof(TimerTabData));
  TimerTabData * timer_tab_data = (TimerTabData *) tab->tab_data;

  timer_tab_data->tab = tab;
  timer_tab_data->last_mmss_text[0] = 0;

  // need to make a copy of beep_points because the (const int[]) from the caller
  // can go out of scope and the contents become invalid
  {
    int l_index = 0;
    do {
      // printf("Beep point %d\n", beep_points[l_index]);
    } while (beep_points[l_index++] != -1);
    // printf("Beep point count %d\n", l_index);
    
    timer_tab_data -> beep_points = (int *) malloc(l_index * sizeof(int));
    memcpy(timer_tab_data->beep_points, beep_points, l_index*sizeof(int));
    
    // fflush(stdout);
  }
  
#if 0
  {
    const int * bb = timer_tab_data->beep_points;
    int l_index = 0;
    do {
      printf("Beep point* %d\n", bb[l_index]);
    } while (bb[l_index++] != -1);
    fflush(stdout);
  }
#endif
  
  tt_init(&(timer_tab_data->tt), start_seconds * 1000);

  lv_obj_t * obj;
  lv_obj_t * label;

  obj = timer_tab_data->button = lv_btn_create(lv_tabview_tab);
  lv_obj_add_event_cb(obj, timer_button_event_cb, LV_EVENT_LONG_PRESSED, tab);
  lv_obj_add_event_cb(obj, timer_button_event_cb, LV_EVENT_SINGLE_CLICKED, tab);
  lv_obj_set_align(obj, LV_ALIGN_CENTER);
  lv_obj_set_size(obj, lv_pct(100), lv_pct(50));

  obj = timer_tab_data->label = lv_label_create(timer_tab_data->button);
  lv_obj_set_align(obj, LV_ALIGN_CENTER);
  lv_obj_set_style_text_font(obj, &IBMPlexMonoBold_96, 0);

  TimerBump * bump;
    
  bump = makeTimerBump(timer_tab_data, +10);
  obj = timer_tab_data->up_button = lv_btn_create(lv_tabview_tab);
  lv_obj_add_event_cb(obj, timer_bump_button_event_cb, LV_EVENT_CLICKED, bump);
  lv_obj_add_event_cb(obj, timer_bump_button_event_cb, LV_EVENT_LONG_PRESSED, bump);
  lv_obj_add_event_cb(obj, timer_bump_button_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, bump);
  lv_obj_set_align(obj, LV_ALIGN_TOP_MID);
  lv_obj_set_size(obj, lv_pct(100), lv_pct(20));
  
  label = lv_label_create(timer_tab_data->up_button);          /*Add a label to the button*/
  lv_obj_center(label);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
  lv_label_set_text(label, LV_SYMBOL_UP);                     /*Set the labels text*/

  bump = makeTimerBump(timer_tab_data, -10);
  obj = timer_tab_data->down_button = lv_btn_create(lv_tabview_tab);
  lv_obj_add_event_cb(obj, timer_bump_button_event_cb, LV_EVENT_CLICKED, bump);
  lv_obj_add_event_cb(obj, timer_bump_button_event_cb, LV_EVENT_LONG_PRESSED, bump);
  lv_obj_add_event_cb(obj, timer_bump_button_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, bump);
  lv_obj_set_align(obj, LV_ALIGN_BOTTOM_MID);
  lv_obj_set_size(obj, lv_pct(100), lv_pct(20));

  label = lv_label_create(timer_tab_data->down_button);          /*Add a label to the button*/
  lv_obj_center(label);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
  lv_label_set_text(label, LV_SYMBOL_DOWN);                     /*Set the labels text*/

}

typedef struct MiscTabData_s MiscTabData;

#define TOD_LABEL_L 40
#define BATTERY_LABEL_L 10
struct MiscTabData_s {
  lv_obj_t * time_label;
  lv_obj_t * battery_label;
  char last_tod_text[TOD_LABEL_L];
  char last_battery_text[BATTERY_LABEL_L];
};

void misc_tab_tick(Tab * tab) {
  MiscTabData * misc_tab_data = (MiscTabData *) (tab -> tab_data);
 
#ifdef ARDUINO
  if (rtc_ready) {
    RTC_DateTime datetime = rtc.getDateTime();
    char text[TOD_LABEL_L];
    snprintf(text, sizeof(text), " %02d:%02d:%02d \n%04d-%02d-%02d", datetime.getHour(), datetime.getMinute(), datetime.getSecond(), datetime.getYear(), datetime.getMonth(), datetime.getDay());
    // only update if necessary
    if (strcmp(text, misc_tab_data->last_tod_text) != 0) {
      lv_label_set_text(misc_tab_data->time_label, text);
      memcpy(&misc_tab_data->last_tod_text, text, TOD_LABEL_L);
    }
  }

  char batt_text[BATTERY_LABEL_L];
  if (power.isBatteryConnect()) {
    snprintf(batt_text, sizeof(batt_text), "%d%%", power.getBatteryPercent());
  } else {
    snprintf(batt_text, sizeof(batt_text), "missing");
  }
  // only update if necessary
  if (strcmp(batt_text, misc_tab_data->last_battery_text) != 0) {
    lv_label_set_text(misc_tab_data->battery_label, batt_text);
    memcpy(&misc_tab_data->last_battery_text, batt_text, BATTERY_LABEL_L);
  }

#else
  (void) misc_tab_data;
#endif

}

void setup_misc_tab (lv_obj_t * tabview, Tab * tab, lv_obj_t * top) {
  lv_obj_t * lv_tabview_tab = lv_tabview_add_tab(tabview, "Time");
  lv_obj_set_style_bg_color(lv_tabview_tab, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(lv_tabview_tab, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  
  tab->tick = misc_tab_tick;
  tab->name = "Time";

  tab->tab_data = malloc(sizeof(MiscTabData));
  MiscTabData * misc_tab_data = (MiscTabData *) tab->tab_data;

  lv_obj_t * obj;

  obj = misc_tab_data->time_label = lv_label_create(lv_tabview_tab);
  lv_obj_align(obj, LV_ALIGN_CENTER, 0, -50);
  lv_obj_set_style_text_color(obj, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(obj, &IBMPlexMonoBold_60, 0);
  lv_obj_set_style_text_line_space(obj, 30, 0); 
  lv_label_set_text(obj, " 00:00:00 \n0000-00-00");

  misc_tab_data->last_tod_text[0] = 0;
  misc_tab_data->last_battery_text[0] = 0;

  obj = misc_tab_data->battery_label = lv_label_create(top);
  lv_obj_align(obj, LV_ALIGN_CENTER, 80, 0);
  lv_obj_set_style_bg_color(obj, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_color(obj, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(obj, &lv_font_montserrat_48, 0);
  lv_obj_set_width(obj, 150);
  lv_label_set_text(obj, ".");
}

void my_timer_cb(lv_timer_t * timer) {
  (void) timer;

  Tab visibleTab = tabs[visible_tab_index];
  visibleTab.tick(&visibleTab);
  
  uint32_t inactive = lv_display_get_inactive_time(NULL);

  if (inactive < 5000) {
    setBrightness(BRIGHT);
  } else {
    setBrightness(DIM);
  }
}

void beep_timer_cb(lv_timer_t * timer) {
  (void) timer;
#if ARDUINO
  if (number_of_beeps > 0) {
    static int16_t buf[256 * 2];
    static float phase = 0;
    size_t written;
    
    written = 0;
    for (int n = 0; n < (SAMPLE_RATE * 0.25) / 256; n++) {
      for (int i = 0; i < 256; i++) {
        int16_t s = (int16_t)(sinf(phase) * 16000);   // was 8000
        phase += 2 * PI * 1760.0f / SAMPLE_RATE;      // was 440.0f
        if (phase > 2 * PI) phase -= 2 * PI;
        buf[2 * i] = s;
        buf[2 * i + 1] = s;
      }
      written += i2s.write((uint8_t *)buf, sizeof(buf));
    }
    // printf("wrote %u bytes\n", (unsigned)written);

    written = 0;
    for (int n = 0; n < (SAMPLE_RATE * 0.25) / 256; n++) {
      for (int i = 0; i < 256; i++) {
        int16_t s = (int16_t)(sinf(phase) * 16000);   // was 8000
        phase += 2 * PI * 880.0f / SAMPLE_RATE;      // was 440.0f
        if (phase > 2 * PI) phase -= 2 * PI;
        buf[2 * i] = s;
        buf[2 * i + 1] = s;
      }
      written += i2s.write((uint8_t *)buf, sizeof(buf));
    }
    // printf("wrote %u bytes\n", (unsigned)written);
    number_of_beeps--;
  } else {
    disableAmp();
  }
#endif
}

void app() {
  setBrightness(BRIGHT);

  lv_obj_t * screen = lv_screen_active();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);    // doesn't seem to help
 
  lv_obj_t * top = lv_obj_create(screen);
  lv_obj_set_align(top, LV_ALIGN_TOP_MID);
  lv_obj_set_size(top, lv_pct(100), lv_pct(15));
  lv_obj_set_style_bg_color(top, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(top, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(top, 0, LV_PART_MAIN);       // doesn't seem to help

  lv_obj_t * bottom = lv_obj_create(screen);
  lv_obj_set_align(bottom, LV_ALIGN_BOTTOM_MID);
  lv_obj_set_size(bottom, lv_pct(100), lv_pct(85));
  lv_obj_set_style_bg_color(bottom, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_all(bottom, 0, LV_PART_MAIN);    // doesn't seem to help

  /* 💡 Tap a tab button or swipe horizontally to switch tabs. */
  lv_obj_t * tabview = lv_tabview_create(bottom);
  lv_obj_add_event_cb(tabview, tabview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_set_size(tabview, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(bottom, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_DEFAULT);

  setup_misc_tab(tabview, &tabs[0], top);
  setup_timer_tab(tabview, &tabs[1], "1:00\nTimeout", 60, (const int[]) {20, 5, -1});
  setup_timer_tab(tabview, &tabs[2], "3:00\nBetween\nSets", 180, (const int[]) {65, 35, 5, -1});
  setup_timer_tab(tabview, &tabs[3], "4/4/2\nWarmup", 10 * 60, (const int[]) {20, 2*60 + 20, 6*60 + 20, -1});
  setup_timer_tab(tabview, &tabs[4], "4/4/4/4\nWarmup", 20 * 60, (const int[]) {20, 2*60 + 5, 4*60 + 20, 8*60 + 20, 12*60 + 20, 16*60 + 20, -1});
  setup_timer_tab(tabview, &tabs[5], "0:06\nTest", 6, (const int[]) {3, -1});

  tabs[0].tick(&tabs[0]);

  lv_timer_create(my_timer_cb, 500, NULL);
  lv_timer_create(beep_timer_cb, 500, NULL);
}
