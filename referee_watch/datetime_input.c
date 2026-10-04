#include "datetime_input.h"

/* Value font: use Montserrat 20 if enabled in lv_conf.h, otherwise the default font */
#if LV_FONT_MONTSERRAT_20
#define DT_VALUE_FONT (&lv_font_montserrat_20)
#else
#define DT_VALUE_FONT LV_FONT_DEFAULT
#endif

#define DT_BTN_HEIGHT 48   /* touch-friendly button height in px */
#define DT_BTN_WIDTH  72   /* button width in px */

typedef enum { F_YEAR, F_MONTH, F_DAY, F_HOUR, F_MINUTE, F_COUNT } field_t;

typedef struct dt_input_s dt_input_t;

typedef struct {
    dt_input_t * owner;
    field_t      field;
    int8_t       dir;   /* +1 or -1 */
} btn_ctx_t;

struct dt_input_s {
    lv_obj_t * cont;
    lv_obj_t * value_lbl[F_COUNT];
    btn_ctx_t  ctx[F_COUNT][2];
    dt_value_t v;
    uint16_t   year_min;
    uint16_t   year_max;
};

static const char * const captions[F_COUNT] = { "Year", "Month", "Day", "Hour", "Minute" };

/* ---------- date helpers ---------- */

static bool is_leap(uint16_t y)
{
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static uint8_t days_in_month(uint16_t y, uint8_t m)
{
    static const uint8_t dim[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if(m == 2 && is_leap(y)) return 29;
    return dim[(m - 1) % 12];
}

static int wrap(int n, int min, int max)
{
    if(n > max) return min;
    if(n < min) return max;
    return n;
}

static void clamp_day(dt_input_t * dt)
{
    uint8_t dim = days_in_month(dt->v.year, dt->v.month);
    if(dt->v.day > dim) dt->v.day = dim;
    if(dt->v.day < 1) dt->v.day = 1;
}

/* ---------- UI update ---------- */

static void refresh(dt_input_t * dt)
{
    lv_label_set_text_fmt(dt->value_lbl[F_YEAR],   "%04u", (unsigned)dt->v.year);
    lv_label_set_text_fmt(dt->value_lbl[F_MONTH],  "%02u", (unsigned)dt->v.month);
    lv_label_set_text_fmt(dt->value_lbl[F_DAY],    "%02u", (unsigned)dt->v.day);
    lv_label_set_text_fmt(dt->value_lbl[F_HOUR],   "%02u", (unsigned)dt->v.hour);
    lv_label_set_text_fmt(dt->value_lbl[F_MINUTE], "%02u", (unsigned)dt->v.minute);
}

static void step_field(dt_input_t * dt, field_t f, int dir)
{
    switch(f) {
        case F_YEAR: {
            int y = (int)dt->v.year + dir;
            if(y < dt->year_min) y = dt->year_min;
            if(y > dt->year_max) y = dt->year_max;
            dt->v.year = (uint16_t)y;
            break;
        }
        case F_MONTH:
            dt->v.month = (uint8_t)wrap(dt->v.month + dir, 1, 12);
            break;
        case F_DAY:
            dt->v.day = (uint8_t)wrap(dt->v.day + dir, 1, days_in_month(dt->v.year, dt->v.month));
            break;
        case F_HOUR:
            dt->v.hour = (uint8_t)wrap(dt->v.hour + dir, 0, 23);
            break;
        case F_MINUTE:
            dt->v.minute = (uint8_t)wrap(dt->v.minute + dir, 0, 59);
            break;
        default:
            break;
    }

    /* Changing month/year may invalidate the day (e.g. Jan 31 -> Feb) */
    clamp_day(dt);
    refresh(dt);
    lv_obj_send_event(dt->cont, LV_EVENT_VALUE_CHANGED, NULL);
}

/* ---------- events ---------- */

static void btn_event_cb(lv_event_t * e)
{
    btn_ctx_t * c = (btn_ctx_t *)lv_event_get_user_data(e);
    step_field(c->owner, c->field, c->dir);
}

static void delete_event_cb(lv_event_t * e)
{
    dt_input_t * dt = (dt_input_t *)lv_event_get_user_data(e);
    lv_free(dt);
}

/* ---------- construction ---------- */

static lv_obj_t * make_btn(lv_obj_t * parent, const char * symbol, btn_ctx_t * ctx)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_size(btn, DT_BTN_WIDTH, DT_BTN_HEIGHT);

    lv_obj_t * lbl = lv_label_create(btn);
    lv_label_set_text(lbl, symbol);
    lv_obj_center(lbl);

    /* Short click = one step; holding = auto-repeat */
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_SHORT_CLICKED, ctx);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, ctx);
    return btn;
}

lv_obj_t * dt_input_create(lv_obj_t * parent)
{
    dt_input_t * dt = (dt_input_t *)lv_zalloc(sizeof(dt_input_t));
    LV_ASSERT_MALLOC(dt);
    if(dt == NULL) return NULL;

    dt->year_min = 2000;
    dt->year_max = 2099;
    dt->v = (dt_value_t){ 2026, 1, 1, 0, 0 };

    lv_obj_t * cont = lv_obj_create(parent);
    dt->cont = cont;
    lv_obj_set_user_data(cont, dt);
    lv_obj_set_width(cont, LV_PCT(100));
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 6, 0);
    lv_obj_set_style_pad_row(cont, 6, 0);
    lv_obj_add_event_cb(cont, delete_event_cb, LV_EVENT_DELETE, dt);

    for(int f = 0; f < F_COUNT; f++) {
        /* One row per field: [-]  caption/value  [+] */
        lv_obj_t * row = lv_obj_create(cont);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_column(row, 6, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        dt->ctx[f][0] = (btn_ctx_t){ dt, (field_t)f, +1 };
        dt->ctx[f][1] = (btn_ctx_t){ dt, (field_t)f, -1 };

        make_btn(row, LV_SYMBOL_MINUS, &dt->ctx[f][1]);   /* decrement on the left */

        /* Middle: small caption above the value, takes the remaining width */
        lv_obj_t * mid = lv_obj_create(row);
        lv_obj_set_height(mid, LV_SIZE_CONTENT);
        lv_obj_set_flex_grow(mid, 1);
        lv_obj_remove_flag(mid, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(mid, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(mid, 0, 0);
        lv_obj_set_style_pad_all(mid, 0, 0);
        lv_obj_set_style_pad_row(mid, 2, 0);
        lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(mid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t * cap = lv_label_create(mid);
        lv_label_set_text(cap, captions[f]);

        lv_obj_t * val = lv_label_create(mid);
        lv_obj_set_style_text_font(val, DT_VALUE_FONT, 0);
        dt->value_lbl[f] = val;

        make_btn(row, LV_SYMBOL_PLUS, &dt->ctx[f][0]);    /* increment on the right */
    }

    refresh(dt);
    return cont;
}

/* ---------- public API ---------- */

void dt_input_set_value(lv_obj_t * obj, const dt_value_t * v)
{
    dt_input_t * dt = (dt_input_t *)lv_obj_get_user_data(obj);
    if(dt == NULL || v == NULL) return;

    dt->v = *v;
    if(dt->v.year < dt->year_min) dt->v.year = dt->year_min;
    if(dt->v.year > dt->year_max) dt->v.year = dt->year_max;
    if(dt->v.month < 1)  dt->v.month = 1;
    if(dt->v.month > 12) dt->v.month = 12;
    if(dt->v.hour > 23)   dt->v.hour = 23;
    if(dt->v.minute > 59) dt->v.minute = 59;
    clamp_day(dt);
    refresh(dt);
}

void dt_input_get_value(lv_obj_t * obj, dt_value_t * v)
{
    dt_input_t * dt = (dt_input_t *)lv_obj_get_user_data(obj);
    if(dt == NULL || v == NULL) return;
    *v = dt->v;
}

void dt_input_set_year_range(lv_obj_t * obj, uint16_t year_min, uint16_t year_max)
{
    dt_input_t * dt = (dt_input_t *)lv_obj_get_user_data(obj);
    if(dt == NULL || year_min > year_max) return;

    dt->year_min = year_min;
    dt->year_max = year_max;
    dt_input_set_value(obj, &dt->v);   /* re-clamp current value */
}