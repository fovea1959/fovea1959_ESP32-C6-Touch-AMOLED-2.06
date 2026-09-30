#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <time.h>

#define MMSS_L 20

#ifdef ARDUINO
#include <Arduino.h>
#include <lvgl.h>

#define TT_TYPE int64_t

TT_TYPE get_time_millis() {
  return millis();
}

void beep_on() {
}

void beep_off() {
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

#endif

typedef struct TT_s TT;

struct TT_s {
  TT_TYPE start_value;
  TT_TYPE accumulated;
  TT_TYPE started;
  bool running;
};

typedef struct Tab_s Tab;

typedef struct Timer_s Timer;

typedef struct TimerBump_s TimerBump;

typedef void (*TickFunction) (Tab*);

struct Tab_s {
  char * name;
  void * tab_data;
  TickFunction tick;
};
  

struct Timer_s {
  Tab * tab;

  TT tt;
  
  int * beep_points;
  
  lv_obj_t * button;
  lv_obj_t * label;
  lv_obj_t * up_button;
  lv_obj_t * down_button;
};

struct TimerBump_s {
  Timer * timer;
  int inc;
  int consecutive_long_presses;
};

typedef struct Beeper_s Beeper; 

struct Beeper_s {
  TT_TYPE beep_start;
};

Beeper beeper;

void beep() {
  if (beeper.beep_start == 0) beep_on();
  beeper.beep_start = get_time_millis();
}

void beeper_timer() {
  if (beeper.beep_start == 0) return;
  TT_TYPE now = get_time_millis();
  if (now - beeper.beep_start > 3000) {
    beep_off();
    beeper.beep_start = 0;
  }
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
#if __linux__
  printf ("TT dump: %s, running = %d, start_value = %ld, accumulated = %ld, started = %ld\n",
     s, tt->running, tt->start_value, tt->accumulated, tt->started);
  fflush(stdout);
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

Tab tabs[4];

void xxxx(char * c) {
  for (int i = 0; i < 4; i++) {
    printf("%s: tab %d name = '%s'\n", c, i, tabs[i].name);
  }
}

uint32_t visible_tab_index = 0;

static char last_label[MMSS_L] = {0};

void timer_tick(Tab * tab) {
  Timer * timer = (Timer *) (tab -> tab_data);
  TT_TYPE remaining_ms = tt_remaining(&timer->tt);

  //xxxx("pretick");
  
  int rs = remaining_ms / 1000;
  if (remaining_ms > 0 && rs == 0) rs = 1;
  if (remaining_ms <= 0) rs = 0;
  
  int mm = rs / 60;
  int ss = rs % 60;
  
  if (timer->tt.running) {
    bool should_beep = is_i_in_list(rs, timer->beep_points);
    if (should_beep) beep();
  }

  char label[MMSS_L];
  //snprintf(buffer, sizeof(buffer), "%.2d:%.2d\n%ld", mm, ss, remaining_ms);
  snprintf(label, sizeof(label), "%.2d:%.2d", mm, ss);

  // only update if necessary
  if (strcmp(label, last_label) != 0) {
    lv_label_set_text(timer->label, label);
    memcpy(&last_label, label, MMSS_L);
  }

  //xxxx("posttick");

}

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
  Timer * timer = (Timer *) tab->tab_data;
  
  printf("*** Button event: code=%s, tab->name='%s'\n", event_name(code), tab->name);
  
  if (code == LV_EVENT_SINGLE_CLICKED) {
    if ((timer->tt).running) {
      tt_stop(&(timer->tt));
    } else {
      tt_start(&(timer->tt));
    }
    tab->tick(tab);
  } else if (code == LV_EVENT_LONG_PRESSED) {
    if (!(timer->tt).running) {
      tt_reset(&(timer->tt));
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
  Timer * timer = bump->timer;
  Tab * tab = timer->tab;
  
  if (timer->tt.running) return;
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
  
  tt_bump(&timer->tt, -(amount * 1000));
  tab->tick(tab);
}

void my_timer_cb(lv_timer_t * timer) {
  (void) timer;

  Tab visibleTab = tabs[visible_tab_index];
  visibleTab.tick(&visibleTab);
  
  beeper_timer();
}

TimerBump * makeTimerBump(Timer * t, int inc) {
  TimerBump * rv = (TimerBump *) malloc(sizeof(TimerBump));
  rv->timer = t;
  rv->inc = inc;
  rv->consecutive_long_presses = 0;
  return rv;
}

void setup_timer_tab (lv_obj_t * tabview, Tab * tab, char * title, int start_seconds, const int * beep_points) {
  lv_obj_t * lv_tabview_tab = lv_tabview_add_tab(tabview, title);
  
  tab->tick = timer_tick;
  tab->name = title;
  
  tab->tab_data = malloc(sizeof(Timer));
  Timer * timer = (Timer *) tab->tab_data;

  timer->tab = tab;

  // need to make a copy of beep_points because the (const int[]) from the caller
  // can go out of scope and the contents become invalid
  {
    int l_index = 0;
    do {
      // printf("Beep point %d\n", beep_points[l_index]);
    } while (beep_points[l_index++] != -1);
    // printf("Beep point count %d\n", l_index);
    
    timer -> beep_points = (int *) malloc(l_index * sizeof(int));
    memcpy(timer->beep_points, beep_points, l_index*sizeof(int));
    
    // fflush(stdout);
  }
  
#if 0
  {
    const int * bb = timer->beep_points;
    int l_index = 0;
    do {
      printf("Beep point* %d\n", bb[l_index]);
    } while (bb[l_index++] != -1);
    fflush(stdout);
  }
#endif
  
  tt_init(&(timer->tt), start_seconds * 1000);

  timer->button = lv_btn_create(lv_tabview_tab);
  lv_obj_set_align(timer->button, LV_ALIGN_CENTER);
  lv_obj_set_size(timer->button, lv_pct(100), lv_pct(50));
  lv_obj_add_event_cb(timer->button, timer_button_event_cb, LV_EVENT_LONG_PRESSED, tab);
  lv_obj_add_event_cb(timer->button, timer_button_event_cb, LV_EVENT_SINGLE_CLICKED, tab);

  timer->label = lv_label_create(timer->button);
  lv_label_set_text(timer->label, title);
  lv_obj_set_align(timer->label, LV_ALIGN_CENTER);
  lv_obj_set_style_text_font(timer->label, &lv_font_montserrat_48, 0);

  lv_obj_t * label;

  TimerBump * bump;
    
  timer->up_button = lv_btn_create(lv_tabview_tab);
  lv_obj_set_align(timer->up_button, LV_ALIGN_TOP_MID);
  lv_obj_set_size(timer->up_button, lv_pct(100), lv_pct(20));
  bump = makeTimerBump(timer, +10);
  lv_obj_add_event_cb(timer->up_button, timer_bump_button_event_cb, LV_EVENT_CLICKED, bump);
  lv_obj_add_event_cb(timer->up_button, timer_bump_button_event_cb, LV_EVENT_LONG_PRESSED, bump);
  lv_obj_add_event_cb(timer->up_button, timer_bump_button_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, bump);
  label = lv_label_create(timer->up_button);          /*Add a label to the button*/
  lv_label_set_text(label, LV_SYMBOL_UP);                     /*Set the labels text*/
  lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
  lv_obj_center(label);

  timer->down_button = lv_btn_create(lv_tabview_tab);
  lv_obj_set_align(timer->down_button, LV_ALIGN_BOTTOM_MID);
  lv_obj_set_size(timer->down_button, lv_pct(100), lv_pct(20));
  bump = makeTimerBump(timer, -10);
  lv_obj_add_event_cb(timer->down_button, timer_bump_button_event_cb, LV_EVENT_CLICKED, bump);
  lv_obj_add_event_cb(timer->down_button, timer_bump_button_event_cb, LV_EVENT_LONG_PRESSED, bump);
  lv_obj_add_event_cb(timer->down_button, timer_bump_button_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, bump);
  label = lv_label_create(timer->down_button);          /*Add a label to the button*/
  lv_label_set_text(label, LV_SYMBOL_DOWN);                     /*Set the labels text*/
  lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
  lv_obj_center(label);

}

void app() {
  lv_obj_t * screen = lv_screen_active();

  /* 💡 Tap a tab button or swipe horizontally to switch tabs. */
  lv_obj_t * tabview = lv_tabview_create(screen);
  lv_obj_add_event_cb(tabview, tabview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_set_size(tabview, lv_pct(100), lv_pct(100));

  Tab * tab0 = &tabs[0];
  setup_timer_tab(tabview, &tabs[0], "1:00\nTimeout", 60, (const int[]) {55, 20, -1});
  setup_timer_tab(tabview, &tabs[1], "3:00\nBetween\nSets", 180, (const int[]) {65, 35, 5, -1});
  setup_timer_tab(tabview, &tabs[2], "4/4/2\nWarmup", 10 * 60, (const int[]) {20, 120 + 20, 360 + 20, -1});
  setup_timer_tab(tabview, &tabs[3], "4/4/4/4\nWarmup", 20 * 60, (const int[]) {20, 4*60 + 20, 8*60 + 20, 12*60 + 20, 16*60 + 20, -1});

  printf("tab0 running: %d\n", ((Timer *) tab0->tab_data)->tt.running);
  printf("foo!\n");
  fflush(stdout);

  xxxx("pre-tick");
  tabs[0].tick(&tabs[0]);
  xxxx("post-tick");

  lv_timer_create(my_timer_cb, 500, NULL);
}
