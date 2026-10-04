#ifndef DATETIME_INPUT_H
#define DATETIME_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct {
    uint16_t year;
    uint8_t  month;   /* 1..12 */
    uint8_t  day;     /* 1..days in month */
    uint8_t  hour;    /* 0..23 */
    uint8_t  minute;  /* 0..59 */
} dt_value_t;

/* Create the widget. Sends LV_EVENT_VALUE_CHANGED whenever a field changes. */
lv_obj_t * dt_input_create(lv_obj_t * parent);

/* Set / get the current value (set clamps invalid values). */
void dt_input_set_value(lv_obj_t * obj, const dt_value_t * v);
void dt_input_get_value(lv_obj_t * obj, dt_value_t * v);

/* Limit the selectable year range (default 2000..2099). */
void dt_input_set_year_range(lv_obj_t * obj, uint16_t year_min, uint16_t year_max);

#ifdef __cplusplus
}
#endif

#endif /* DATETIME_INPUT_H */