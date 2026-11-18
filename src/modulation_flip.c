#include <furi.h>
#include <gui/gui.h>
#include <gui/canvas.h>
#include <gui/elements.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/text_input.h>
#include <gui/modules/widget.h>
#include <gui/modules/popup.h>
#include <input/input.h>

#include <stdio.h>
#include <string.h>

#include "protocols_data.h"
#include "ui_helpers.h"

#define SEARCH_BUF_LEN        36
#define MAX_LIST_ITEMS        192
#define DETAIL_TEXT_LEN       384
#define DETAIL_MAX_W          120
#define SPLASH_DURATION_TICKS 14

typedef enum {
    SceneStart = 0,
    SceneBrands,
    SceneFilter,
    SceneSearch,
    SceneNoMatch,
    SceneProtoList,
    SceneDetail,
    SceneInfo,
    SceneSplash,
    SceneCount,
} SceneId;

typedef enum {
    ViewIdMenu = 0,
    ViewIdProtoList,
    ViewIdDetail,
    ViewIdSplash,
    ViewIdTextInput,
    ViewIdWidget,
    ViewIdPopup,
} ViewId;

typedef enum {
    CustomEventMenuSelect = 100,
    CustomEventListSelect,
    CustomEventSearchDone,
    CustomEventPopupDone,
    CustomEventDetailPrev,
    CustomEventDetailNext,
    CustomEventSplashDone,
} CustomEvent;

typedef enum {
    MenuModeStart = 0,
    MenuModeBrands,
    MenuModeFilter,
} MenuMode;

typedef enum {
    StartItemBrands = 0,
    StartItemSearch,
    StartItemCars,
    StartItemGates,
    StartItemFilter,
    StartItemAll,
    StartItemInfo,
    StartItemCount,
} StartItem;

typedef enum {
    FilterAm = 0,
    FilterFm,
    FilterAmFm,
    Filter315,
    Filter433,
    Filter868,
    FilterOther,
    FilterCount,
} FilterOption;

typedef enum {
    ListModeAll = 0,
    ListModeCars,
    ListModeGates,
    ListModeBrand,
    ListModeFilter,
    ListModeSearch,
} ListMode;

typedef struct {
    MenuMode mode;
    size_t cursor;
    size_t count;
    uint16_t filter_counts[FilterCount];
    uint16_t tick;
    bool needs_scroll;
} MenuViewModel;

typedef struct {
    char title[56];
    uint8_t indices[MAX_LIST_ITEMS];
    size_t count;
    size_t cursor;
    uint16_t tick;
    bool needs_scroll;
} ProtoListModel;

typedef struct {
    const ProtocolEntry* entry;
    size_t list_pos;
    size_t list_total;
    char detail_text[DETAIL_TEXT_LEN];
    uint8_t scroll;
    uint8_t max_scroll;
    uint16_t tick;
    bool needs_scroll;
} DetailViewModel;

typedef struct {
    uint16_t tick;
    bool done;
} SplashViewModel;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    View* menu_view;
    View* list_view;
    View* detail_view;
    View* splash_view;
    TextInput* text_input;
    Widget* widget;
    Popup* popup;

    // Navigation & active query state
    size_t start_cursor;
    size_t brand_cursor;
    size_t filter_cursor;
    size_t selected_brand;
    FilterOption selected_filter;
    ListMode list_mode;
    size_t list_cursor;
    uint8_t list_indices[MAX_LIST_ITEMS];
    size_t list_count;

    char search_buf[SEARCH_BUF_LEN];
    char popup_msg[64];
    uint16_t filter_counts[FilterCount];
} ModulationFlipApp;

static const char* const kStartLabels[StartItemCount] = {
    [StartItemBrands] = "By Brand",
    [StartItemSearch] = "Search Keyword",
    [StartItemCars] = "Cars & Alarms",
    [StartItemGates] = "Gates & Home",
    [StartItemFilter] = "Filter RF / Band",
    [StartItemAll] = "All Protocols",
    [StartItemInfo] = "RF & Preset Info",
};

static const UiMenuIcon kStartIcons[StartItemCount] = {
    [StartItemBrands] = UiMenuIconBrand,
    [StartItemSearch] = UiMenuIconSearch,
    [StartItemCars] = UiMenuIconCar,
    [StartItemGates] = UiMenuIconGate,
    [StartItemFilter] = UiMenuIconFilter,
    [StartItemAll] = UiMenuIconAll,
    [StartItemInfo] = UiMenuIconInfo,
};

static const char* const kFilterLabels[FilterCount] = {
    [FilterAm] = "AM (OOK)",
    [FilterFm] = "FM (2-FSK)",
    [FilterAmFm] = "AM / FM Dual",
    [Filter315] = "315 MHz Band",
    [Filter433] = "433.92 MHz Band",
    [Filter868] = "868 MHz Band",
    [FilterOther] = "Other (300-390)",
};

static const char* const kFilterShortTitles[FilterCount] = {
    [FilterAm] = "RF: AM",
    [FilterFm] = "RF: FM",
    [FilterAmFm] = "RF: AM/FM",
    [Filter315] = "Band: 315",
    [Filter433] = "Band: 433.92",
    [Filter868] = "Band: 868",
    [FilterOther] = "Band: 300-390",
};

static const char* const kRfPresetInfoText = "\e#ModulationFlip v1.0\n"
                                             "Created by limbo\n"
                                             "138 RF protocols\n"
                                             "95 Car/Alarm + 43 Gate/Home\n"
                                             "\n"
                                             "\e#Flipper Hopper Lists\n"
                                             "Std/Momentum/Unleashed/FoxFW:\n"
                                             "315, 390, 430.5, 433.92,\n"
                                             "434.42, 868.35 MHz\n"
                                             "Flipper-ARF: + 434.25 MHz\n"
                                             "Doctor FW: 315, 390, 430.5,\n"
                                             "433.14, 433.47, 433.92,\n"
                                             "434.37, 434.42, 868.35 MHz\n"
                                             "\n"
                                             "\e#CC1101 Presets & Mod\n"
                                             "AM270 / AM650 -> OOK/AM\n"
                                             "(VAG route uses AM270)\n"
                                             "FM238 -> Honda1 (Honda Static)\n"
                                             "FM12K -> F4 (Ford, Honda V2/3)\n"
                                             "FM476 -> 2-FSK +/-47.61 kHz\n"
                                             "Doctor adds AM101, FM14K,\n"
                                             "FM15k, FM20K/FM20KR (+Si4463)\n"
                                             "KeeLoq FM: Cardin S449 &\n"
                                             "Sommer SOMloq (FSK12K/476)\n"
                                             "\n"
                                             "\e#Verified Repo Sources\n"
                                             "Flipper-ARF, doctor-firmware,\n"
                                             "Unleashed, Momentum, FoxFW,\n"
                                             "ProtoPirate\n";

// --- Filtering Helpers ---

static bool entry_matches_filter(const ProtocolEntry* e, FilterOption opt) {
    switch(opt) {
    case FilterAm:
        return (e->rf_mask & RF_FLAG_AM) != 0;
    case FilterFm:
        return (e->rf_mask & RF_FLAG_FM) != 0;
    case FilterAmFm:
        return (e->rf_mask & (RF_FLAG_AM | RF_FLAG_FM)) == (RF_FLAG_AM | RF_FLAG_FM);
    case Filter315:
        return (e->freq_mask & FREQ_FLAG_315) != 0;
    case Filter433:
        return (e->freq_mask & FREQ_FLAG_433) != 0;
    case Filter868:
        return (e->freq_mask & FREQ_FLAG_868) != 0;
    case FilterOther:
        return (e->freq_mask & FREQ_FLAG_OTHER) != 0;
    default:
        return false;
    }
}

static bool entry_matches_query(const ProtocolEntry* e, const char* query) {
    if(!query || query[0] == '\0') return true;

    const char* p = query;
    bool has_token = false;
    while(*p != '\0') {
        while(*p == ' ') {
            p++;
        }
        if(*p == '\0') break;

        has_token = true;
        char token[SEARCH_BUF_LEN];
        size_t tlen = 0;
        while(*p != '\0' && *p != ' ' && tlen < sizeof(token) - 1) {
            token[tlen++] = *p++;
        }
        token[tlen] = '\0';

        bool hit = ui_str_contains_ci(e->name, token) || ui_str_contains_ci(e->encoding, token) ||
                   ui_str_contains_ci(e->rf, token) || ui_str_contains_ci(e->code, token) ||
                   ui_str_contains_ci(e->freq, token) ||
                   (e->tx[0] != '\0' && ui_str_contains_ci(e->tx, token));
        if(!hit) return false;
    }
    return has_token;
}

static size_t
    compute_list_for_current_mode(ModulationFlipApp* app, char* out_title, size_t title_sz) {
    size_t count = 0;

    switch(app->list_mode) {
    case ListModeAll:
        for(size_t i = 0; i < kProtocolCount && count < MAX_LIST_ITEMS; i++) {
            app->list_indices[count++] = (uint8_t)i;
        }
        snprintf(out_title, title_sz, "All Protocols (%zu)", count);
        break;

    case ListModeCars:
        for(size_t i = 0; i < kProtocolCount && count < MAX_LIST_ITEMS; i++) {
            if(kProtocols[i].category == ProtoCategoryCar) {
                app->list_indices[count++] = (uint8_t)i;
            }
        }
        snprintf(out_title, title_sz, "Cars & Alarms (%zu)", count);
        break;

    case ListModeGates:
        for(size_t i = 0; i < kProtocolCount && count < MAX_LIST_ITEMS; i++) {
            if(kProtocols[i].category == ProtoCategoryGate) {
                app->list_indices[count++] = (uint8_t)i;
            }
        }
        snprintf(out_title, title_sz, "Gates & Home (%zu)", count);
        break;

    case ListModeBrand: {
        const BrandEntry* b = &kBrands[app->selected_brand];
        for(uint16_t i = 0; i < b->count && count < MAX_LIST_ITEMS; i++) {
            app->list_indices[count++] = kBrandProtocolIndices[b->offset + i];
        }
        snprintf(out_title, title_sz, "%s (%zu)", b->name, count);
        break;
    }

    case ListModeFilter:
        for(size_t i = 0; i < kProtocolCount && count < MAX_LIST_ITEMS; i++) {
            if(entry_matches_filter(&kProtocols[i], app->selected_filter)) {
                app->list_indices[count++] = (uint8_t)i;
            }
        }
        snprintf(out_title, title_sz, "%s (%zu)", kFilterShortTitles[app->selected_filter], count);
        break;

    case ListModeSearch:
        for(size_t i = 0; i < kProtocolCount && count < MAX_LIST_ITEMS; i++) {
            if(entry_matches_query(&kProtocols[i], app->search_buf)) {
                app->list_indices[count++] = (uint8_t)i;
            }
        }
        snprintf(out_title, title_sz, "Search (%zu): %s", count, app->search_buf);
        break;
    }

    app->list_count = count;
    if(count > 0 && app->list_cursor >= count) {
        app->list_cursor = count - 1;
    }
    return count;
}

// --- Step/Page Navigation Helper ---

static bool step_list_cursor(
    size_t* cursor,
    size_t count,
    InputEvent* event,
    size_t page_step,
    bool* changed) {
    *changed = false;
    if(count == 0) return false;
    bool is_press = (event->type == InputTypePress);
    bool is_repeat = (event->type == InputTypeRepeat);
    if(!is_press && !is_repeat) return false;

    size_t cur = *cursor;
    switch(event->key) {
    case InputKeyUp:
        if(cur > 0) {
            *cursor = cur - 1;
        } else if(is_press) {
            *cursor = count - 1;
        }
        *changed = (*cursor != cur);
        return true;

    case InputKeyDown:
        if(cur + 1 < count) {
            *cursor = cur + 1;
        } else if(is_press) {
            *cursor = 0;
        }
        *changed = (*cursor != cur);
        return true;

    case InputKeyLeft:
        *cursor = (cur > page_step) ? (cur - page_step) : 0;
        *changed = (*cursor != cur);
        return true;

    case InputKeyRight:
        *cursor = (cur + page_step < count) ? (cur + page_step) : (count - 1);
        *changed = (*cursor != cur);
        return true;

    default:
        return false;
    }
}

// --- Shared MenuView (Start, Brands, RF Filter) ---

static void menu_view_draw_callback(Canvas* canvas, void* _model) {
    MenuViewModel* m = _model;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    canvas_set_font(canvas, FontPrimary);
    if(m->mode == MenuModeStart) {
        char hdr[32];
        snprintf(hdr, sizeof(hdr), "ModulationFlip [%zu]", kProtocolCount);
        canvas_draw_str_aligned(canvas, 2, 0, AlignLeft, AlignTop, hdr);
    } else if(m->mode == MenuModeBrands) {
        char hdr[32];
        snprintf(hdr, sizeof(hdr), "Select Brand (%zu)", kBrandCount);
        canvas_draw_str_aligned(canvas, 2, 0, AlignLeft, AlignTop, hdr);
    } else {
        canvas_draw_str_aligned(canvas, 2, 0, AlignLeft, AlignTop, "Filter RF / Band");
    }
    canvas_draw_line(canvas, 0, 11, 127, 11);

    const size_t vis = 4;
    size_t start = 0;
    if(m->count > vis && m->cursor > 1) {
        start = m->cursor - 1;
        if(start > m->count - vis) {
            start = m->count - vis;
        }
    }

    bool need_scroll = false;
    canvas_set_font(canvas, FontSecondary);
    for(size_t r = 0; r < vis && (start + r) < m->count; r++) {
        size_t idx = start + r;
        int32_t y = 13 + (int32_t)(r * 12);
        bool sel = (idx == m->cursor);

        if(sel) {
            canvas_draw_rbox(canvas, 1, y, 122, 11, 2);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_set_color(canvas, ColorBlack);
        }

        const char* label = "";
        char count_str[12] = "";

        if(m->mode == MenuModeStart) {
            ui_draw_menu_icon(canvas, kStartIcons[idx], 4, y + 2);
            label = kStartLabels[idx];
            if(idx == StartItemBrands) {
                snprintf(count_str, sizeof(count_str), "(%zu)", kBrandCount);
            } else if(idx == StartItemCars) {
                snprintf(count_str, sizeof(count_str), "(%zu)", kCarProtocolCount);
            } else if(idx == StartItemGates) {
                snprintf(count_str, sizeof(count_str), "(%zu)", kGateProtocolCount);
            } else if(idx == StartItemAll) {
                snprintf(count_str, sizeof(count_str), "(%zu)", kProtocolCount);
            }
            size_t count_w = count_str[0] ? (size_t)canvas_string_width(canvas, count_str) + 4 : 0;
            size_t label_max_w = 105 - count_w;
            bool overflow =
                ui_draw_marquee_top(canvas, 14, y + 1, label_max_w, label, sel ? m->tick : 0);
            if(sel) need_scroll |= overflow;
            if(count_str[0]) {
                canvas_draw_str_aligned(canvas, 120, y + 1, AlignRight, AlignTop, count_str);
            }
        } else if(m->mode == MenuModeBrands) {
            label = kBrands[idx].name;
            snprintf(count_str, sizeof(count_str), "(%u)", kBrands[idx].count);
            size_t count_w = (size_t)canvas_string_width(canvas, count_str) + 4;
            bool overflow =
                ui_draw_marquee_top(canvas, 4, y + 1, 114 - count_w, label, sel ? m->tick : 0);
            if(sel) need_scroll |= overflow;
            canvas_draw_str_aligned(canvas, 120, y + 1, AlignRight, AlignTop, count_str);
        } else {
            label = kFilterLabels[idx];
            snprintf(count_str, sizeof(count_str), "[%u]", m->filter_counts[idx]);
            size_t count_w = (size_t)canvas_string_width(canvas, count_str) + 4;
            bool overflow =
                ui_draw_marquee_top(canvas, 4, y + 1, 114 - count_w, label, sel ? m->tick : 0);
            if(sel) need_scroll |= overflow;
            canvas_draw_str_aligned(canvas, 120, y + 1, AlignRight, AlignTop, count_str);
        }
    }
    m->needs_scroll = need_scroll;

    canvas_set_color(canvas, ColorBlack);
    if(m->count > vis) {
        elements_scrollbar_pos(canvas, 128, 13, 48, m->cursor, m->count);
    }
}

static bool menu_view_input_callback(InputEvent* event, void* context) {
    ModulationFlipApp* app = context;

    if(event->key == InputKeyOk && event->type == InputTypeShort) {
        size_t selected = 0;
        with_view_model(app->menu_view, MenuViewModel * m, { selected = m->cursor; }, false);

        if(scene_manager_get_current_scene(app->scene_manager) == SceneStart) {
            app->start_cursor = selected;
        } else if(scene_manager_get_current_scene(app->scene_manager) == SceneBrands) {
            app->brand_cursor = selected;
        } else {
            app->filter_cursor = selected;
        }

        view_dispatcher_send_custom_event(app->view_dispatcher, CustomEventMenuSelect);
        return true;
    }

    bool consumed = false;
    bool changed = false;
    with_view_model(
        app->menu_view,
        MenuViewModel * m,
        {
            size_t page_step = (m->mode == MenuModeBrands) ? 8 : 4;
            consumed = step_list_cursor(&m->cursor, m->count, event, page_step, &changed);
            if(changed) {
                m->tick = 0;
            }
        },
        changed);

    return consumed;
}

// --- ProtoListView (Kombi B 5-row List + [RF Freq] Badges) ---

static void list_view_draw_callback(Canvas* canvas, void* _model) {
    ProtoListModel* m = _model;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    canvas_set_font(canvas, FontPrimary);
    ui_draw_marquee_top(canvas, 2, 0, 124, m->title, 0);
    canvas_draw_line(canvas, 0, 11, 127, 11);

    const size_t vis = 5;
    size_t start = 0;
    if(m->count > vis && m->cursor > 2) {
        start = m->cursor - 2;
        if(start > m->count - vis) {
            start = m->count - vis;
        }
    }

    bool need_scroll = false;
    canvas_set_font(canvas, FontSecondary);
    for(size_t r = 0; r < vis && (start + r) < m->count; r++) {
        size_t idx = start + r;
        const ProtocolEntry* e = &kProtocols[m->indices[idx]];
        int32_t y = 12 + (int32_t)(r * 10);
        bool sel = (idx == m->cursor);

        char badge[20];
        snprintf(badge, sizeof(badge), "%s %s", e->rf_short, e->freq_short);
        size_t badge_w = (size_t)canvas_string_width(canvas, badge) + 4;
        int32_t badge_x = 121 - (int32_t)badge_w;

        if(sel) {
            canvas_draw_rbox(canvas, 1, y, 122, 10, 1);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_set_color(canvas, ColorBlack);
        }

        size_t name_max_w = (badge_x > 6) ? (size_t)(badge_x - 6) : 16;
        bool overflow = ui_draw_marquee_top(canvas, 3, y, name_max_w, e->name, sel ? m->tick : 0);
        if(sel) need_scroll |= overflow;

        if(sel) {
            // White pill inside selected row with black text for sharp badge contrast
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_rbox(canvas, badge_x, y, badge_w, 9, 1);
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_str_aligned(canvas, badge_x + 2, y, AlignLeft, AlignTop, badge);
        } else {
            canvas_draw_rframe(canvas, badge_x, y, badge_w, 9, 1);
            canvas_draw_str_aligned(canvas, badge_x + 2, y, AlignLeft, AlignTop, badge);
        }
    }
    m->needs_scroll = need_scroll;

    canvas_set_color(canvas, ColorBlack);
    if(m->count > vis) {
        elements_scrollbar_pos(canvas, 128, 12, 50, m->cursor, m->count);
    }
}

static bool list_view_input_callback(InputEvent* event, void* context) {
    ModulationFlipApp* app = context;

    if(event->key == InputKeyOk && event->type == InputTypeShort) {
        size_t count = 0;
        with_view_model(
            app->list_view,
            ProtoListModel * m,
            {
                app->list_cursor = m->cursor;
                count = m->count;
            },
            false);
        if(count > 0) {
            view_dispatcher_send_custom_event(app->view_dispatcher, CustomEventListSelect);
        }
        return true;
    }

    bool consumed = false;
    bool changed = false;
    with_view_model(
        app->list_view,
        ProtoListModel * m,
        {
            consumed = step_list_cursor(&m->cursor, m->count, event, 5, &changed);
            if(changed) {
                m->tick = 0;
            }
        },
        changed);

    return consumed;
}

// --- DetailView (Kombi B Hero 2x RF + Enc/Cat Cells + Wrapped Scrollable Info) ---

static size_t wrap_and_maybe_draw(
    Canvas* canvas,
    const char* text,
    size_t max_w,
    size_t scroll_off,
    bool draw) {
    if(!text || text[0] == '\0') return 0;

    char cur[64];
    char test[64];
    cur[0] = '\0';
    size_t cur_len = 0;
    size_t line_idx = 0;

    const char* p = text;
    while(*p != '\0') {
        while(*p == ' ') {
            p++;
        }
        if(*p == '\0') break;

        const char* w_start = p;
        while(*p != '\0' && *p != ' ') {
            p++;
        }
        size_t w_len = (size_t)(p - w_start);

        if(cur_len > 0) {
            if(cur_len + 1 + w_len < sizeof(test)) {
                memcpy(test, cur, cur_len);
                test[cur_len] = ' ';
                memcpy(test + cur_len + 1, w_start, w_len);
                test[cur_len + 1 + w_len] = '\0';
                if(canvas_string_width(canvas, test) <= max_w) {
                    memcpy(cur, test, cur_len + 1 + w_len + 1);
                    cur_len += 1 + w_len;
                    continue;
                }
            }
            // Emit completed line
            if(draw && line_idx >= scroll_off && line_idx < scroll_off + 2) {
                int32_t ly = 45 + (int32_t)((line_idx - scroll_off) * 9);
                canvas_draw_str_aligned(canvas, 2, ly, AlignLeft, AlignTop, cur);
            }
            line_idx++;
            cur[0] = '\0';
            cur_len = 0;
        }

        // Fit word cleanly into empty cur, splitting mid-token only if wider than max_w
        size_t rem = w_len;
        const char* wp = w_start;
        while(rem > 0) {
            size_t take = (rem < sizeof(cur) - 1) ? rem : (sizeof(cur) - 1);
            memcpy(cur, wp, take);
            cur[take] = '\0';
            while(take > 1 && canvas_string_width(canvas, cur) > max_w) {
                take--;
                cur[take] = '\0';
            }
            if(take < rem) {
                if(draw && line_idx >= scroll_off && line_idx < scroll_off + 2) {
                    int32_t ly = 45 + (int32_t)((line_idx - scroll_off) * 9);
                    canvas_draw_str_aligned(canvas, 2, ly, AlignLeft, AlignTop, cur);
                }
                line_idx++;
                cur[0] = '\0';
                cur_len = 0;
            } else {
                cur_len = take;
            }
            wp += take;
            rem -= take;
        }
    }

    if(cur_len > 0) {
        if(draw && line_idx >= scroll_off && line_idx < scroll_off + 2) {
            int32_t ly = 45 + (int32_t)((line_idx - scroll_off) * 9);
            canvas_draw_str_aligned(canvas, 2, ly, AlignLeft, AlignTop, cur);
        }
        line_idx++;
    }

    return line_idx;
}

static void detail_view_draw_callback(Canvas* canvas, void* _model) {
    DetailViewModel* m = _model;
    const ProtocolEntry* e = m->entry;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    if(!e) return;

    bool need_scroll = false;

    // 1. Header title
    canvas_set_font(canvas, FontPrimary);
    need_scroll |= ui_draw_marquee_top(canvas, 2, 0, 124, e->name, m->tick);

    // 2. Hero RF + Frequency Card (y=11..31)
    canvas_draw_rframe(canvas, 2, 11, 124, 21, 1);
    canvas_draw_box(canvas, 3, 12, 34, 19);
    canvas_set_color(canvas, ColorWhite);
    ui_draw_hero_rf(canvas, 3, 12, 34, 19, e->rf_short);

    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 41, 13, AlignLeft, AlignTop, "FREQ:");

    char pos_str[16];
    snprintf(pos_str, sizeof(pos_str), "%zu/%zu", m->list_pos + 1, m->list_total);
    canvas_draw_str_aligned(canvas, 122, 13, AlignRight, AlignTop, pos_str);

    need_scroll |= ui_draw_marquee_top(canvas, 41, 21, 81, e->freq, m->tick);

    // 3. Compact Enc + Category Pills (y=33..43)
    canvas_draw_rframe(canvas, 2, 33, 78, 11, 1);
    char enc_str[40];
    snprintf(enc_str, sizeof(enc_str), "Enc: %s", e->encoding);
    need_scroll |= ui_draw_marquee_top(canvas, 5, 34, 72, enc_str, m->tick);
    m->needs_scroll = need_scroll;

    canvas_draw_rframe(canvas, 82, 33, 44, 11, 1);
    canvas_draw_str_aligned(
        canvas,
        104,
        34,
        AlignCenter,
        AlignTop,
        (e->category == ProtoCategoryCar) ? "Car/Key" : "Gate/RF");

    // 4. Wrapped scrollable detail body (y=45..63)
    size_t total_lines = wrap_and_maybe_draw(canvas, m->detail_text, DETAIL_MAX_W, 0, false);
    uint8_t max_scroll = (total_lines > 2) ? (uint8_t)(total_lines - 2) : 0;
    m->max_scroll = max_scroll;
    if(m->scroll > max_scroll) {
        m->scroll = max_scroll;
    }

    wrap_and_maybe_draw(canvas, m->detail_text, DETAIL_MAX_W, m->scroll, true);

    if(max_scroll > 0) {
        elements_scrollbar_pos(canvas, 128, 45, 19, m->scroll, (size_t)max_scroll + 1);
    }
}

static bool detail_view_input_callback(InputEvent* event, void* context) {
    ModulationFlipApp* app = context;
    bool is_press = (event->type == InputTypePress);
    bool is_repeat = (event->type == InputTypeRepeat);
    if(!is_press && !is_repeat) return false;

    if(event->key == InputKeyLeft) {
        view_dispatcher_send_custom_event(app->view_dispatcher, CustomEventDetailPrev);
        return true;
    }
    if(event->key == InputKeyRight) {
        view_dispatcher_send_custom_event(app->view_dispatcher, CustomEventDetailNext);
        return true;
    }

    if(event->key == InputKeyUp || event->key == InputKeyDown) {
        bool changed = false;
        with_view_model(
            app->detail_view,
            DetailViewModel * m,
            {
                if(event->key == InputKeyUp && m->scroll > 0) {
                    m->scroll--;
                    changed = true;
                } else if(event->key == InputKeyDown && m->scroll < m->max_scroll) {
                    m->scroll++;
                    changed = true;
                }
            },
            changed);
        return true;
    }

    return false;
}

static size_t
    append_str_bounded(char* dst, size_t dst_sz, size_t pos, const char* prefix, const char* val) {
    if(pos >= dst_sz - 1) return dst_sz - 1;
    size_t rem = dst_sz - pos;
    int w = snprintf(dst + pos, rem, "%s%s", prefix, val);
    if(w <= 0) return pos;
    if((size_t)w >= rem) return dst_sz - 1;
    return pos + (size_t)w;
}

static void populate_detail_model(ModulationFlipApp* app) {
    if(app->list_count == 0) return;
    if(app->list_cursor >= app->list_count) {
        app->list_cursor = app->list_count - 1;
    }
    const ProtocolEntry* e = &kProtocols[app->list_indices[app->list_cursor]];

    with_view_model(
        app->detail_view,
        DetailViewModel * m,
        {
            m->entry = e;
            m->list_pos = app->list_cursor;
            m->list_total = app->list_count;
            m->scroll = 0;
            m->max_scroll = 0;
            m->tick = 0;
            m->needs_scroll = false;

            m->detail_text[0] = '\0';
            size_t pos = 0;
            bool need_rf_detail =
                (strcmp(e->rf, "AM") != 0 && strcmp(e->rf, "FM") != 0 && strlen(e->rf) > 5);

            if(need_rf_detail) {
                pos = append_str_bounded(m->detail_text, DETAIL_TEXT_LEN, pos, "RF: ", e->rf);
                pos = append_str_bounded(m->detail_text, DETAIL_TEXT_LEN, pos, " | ", "");
            }

            pos = append_str_bounded(m->detail_text, DETAIL_TEXT_LEN, pos, "", e->code);

            if(e->tx[0] != '\0') {
                pos = append_str_bounded(m->detail_text, DETAIL_TEXT_LEN, pos, " | TX: ", e->tx);
            }

            if(strcmp(e->rf, "AM/FM") == 0) {
                pos = append_str_bounded(m->detail_text, DETAIL_TEXT_LEN, pos, " | RF: ", "AM/FM");
            }

            if(strlen(e->encoding) > 10) {
                pos = append_str_bounded(
                    m->detail_text, DETAIL_TEXT_LEN, pos, " | Enc: ", e->encoding);
            }
        },
        true);
}

// --- Native Module Callbacks ---

static void search_text_input_callback(void* context) {
    ModulationFlipApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, CustomEventSearchDone);
}

static void no_match_popup_callback(void* context) {
    ModulationFlipApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, CustomEventPopupDone);
}

// --- Scenes ---

static void scene_start_on_enter(void* context) {
    ModulationFlipApp* app = context;
    with_view_model(
        app->menu_view,
        MenuViewModel * m,
        {
            m->mode = MenuModeStart;
            m->count = StartItemCount;
            m->cursor = (app->start_cursor < StartItemCount) ? app->start_cursor : 0;
            m->tick = 0;
            m->needs_scroll = false;
        },
        true);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIdMenu);
}

static bool scene_start_on_event(void* context, SceneManagerEvent event) {
    ModulationFlipApp* app = context;
    if(event.type != SceneManagerEventTypeCustom || event.event != CustomEventMenuSelect) {
        return false;
    }

    switch(app->start_cursor) {
    case StartItemBrands:
        scene_manager_next_scene(app->scene_manager, SceneBrands);
        return true;

    case StartItemSearch:
        app->search_buf[0] = '\0';
        scene_manager_next_scene(app->scene_manager, SceneSearch);
        return true;

    case StartItemCars:
        app->list_mode = ListModeCars;
        app->list_cursor = 0;
        scene_manager_next_scene(app->scene_manager, SceneProtoList);
        return true;

    case StartItemGates:
        app->list_mode = ListModeGates;
        app->list_cursor = 0;
        scene_manager_next_scene(app->scene_manager, SceneProtoList);
        return true;

    case StartItemFilter:
        scene_manager_next_scene(app->scene_manager, SceneFilter);
        return true;

    case StartItemAll:
        app->list_mode = ListModeAll;
        app->list_cursor = 0;
        scene_manager_next_scene(app->scene_manager, SceneProtoList);
        return true;

    case StartItemInfo:
        scene_manager_next_scene(app->scene_manager, SceneInfo);
        return true;

    default:
        return false;
    }
}

static void scene_start_on_exit(void* context) {
    UNUSED(context);
}

static void scene_brands_on_enter(void* context) {
    ModulationFlipApp* app = context;
    with_view_model(
        app->menu_view,
        MenuViewModel * m,
        {
            m->mode = MenuModeBrands;
            m->count = kBrandCount;
            m->cursor = (app->brand_cursor < kBrandCount) ? app->brand_cursor : 0;
            m->tick = 0;
            m->needs_scroll = false;
        },
        true);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIdMenu);
}

static bool scene_brands_on_event(void* context, SceneManagerEvent event) {
    ModulationFlipApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == CustomEventMenuSelect) {
        app->selected_brand = (app->brand_cursor < kBrandCount) ? app->brand_cursor : 0;
        app->list_mode = ListModeBrand;
        app->list_cursor = 0;
        scene_manager_next_scene(app->scene_manager, SceneProtoList);
        return true;
    }
    return false;
}

static void scene_brands_on_exit(void* context) {
    UNUSED(context);
}

static void scene_filter_on_enter(void* context) {
    ModulationFlipApp* app = context;
    with_view_model(
        app->menu_view,
        MenuViewModel * m,
        {
            m->mode = MenuModeFilter;
            m->count = FilterCount;
            m->cursor = (app->filter_cursor < FilterCount) ? app->filter_cursor : 0;
            memcpy(m->filter_counts, app->filter_counts, sizeof(m->filter_counts));
            m->tick = 0;
            m->needs_scroll = false;
        },
        true);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIdMenu);
}

static bool scene_filter_on_event(void* context, SceneManagerEvent event) {
    ModulationFlipApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == CustomEventMenuSelect) {
        app->selected_filter =
            (app->filter_cursor < FilterCount) ? (FilterOption)app->filter_cursor : FilterAm;
        app->list_mode = ListModeFilter;
        app->list_cursor = 0;
        scene_manager_next_scene(app->scene_manager, SceneProtoList);
        return true;
    }
    return false;
}

static void scene_filter_on_exit(void* context) {
    UNUSED(context);
}

static void scene_search_on_enter(void* context) {
    ModulationFlipApp* app = context;
    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Search brand, RF, MHz");
    text_input_set_minimum_length(app->text_input, 1);
    text_input_set_result_callback(
        app->text_input,
        search_text_input_callback,
        app,
        app->search_buf,
        sizeof(app->search_buf),
        false);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIdTextInput);
}

static bool scene_search_on_event(void* context, SceneManagerEvent event) {
    ModulationFlipApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == CustomEventSearchDone) {
        char tmp_title[56];
        app->list_mode = ListModeSearch;
        app->list_cursor = 0;
        size_t matches = compute_list_for_current_mode(app, tmp_title, sizeof(tmp_title));
        if(matches == 0) {
            snprintf(app->popup_msg, sizeof(app->popup_msg), "Query: %s", app->search_buf);
            scene_manager_next_scene(app->scene_manager, SceneNoMatch);
        } else {
            scene_manager_next_scene(app->scene_manager, SceneProtoList);
        }
        return true;
    }
    return false;
}

static void scene_search_on_exit(void* context) {
    ModulationFlipApp* app = context;
    text_input_reset(app->text_input);
}

static void scene_no_match_on_enter(void* context) {
    ModulationFlipApp* app = context;
    popup_reset(app->popup);
    popup_set_header(app->popup, "No Protocols Found", 64, 22, AlignCenter, AlignCenter);
    popup_set_text(app->popup, app->popup_msg, 64, 40, AlignCenter, AlignTop);
    popup_set_context(app->popup, app);
    popup_set_callback(app->popup, no_match_popup_callback);
    popup_set_timeout(app->popup, 1800);
    popup_enable_timeout(app->popup);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIdPopup);
}

static bool scene_no_match_on_event(void* context, SceneManagerEvent event) {
    ModulationFlipApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == CustomEventPopupDone) {
        scene_manager_previous_scene(app->scene_manager);
        return true;
    }
    return false;
}

static void scene_no_match_on_exit(void* context) {
    ModulationFlipApp* app = context;
    popup_disable_timeout(app->popup);
    popup_reset(app->popup);
}

static void scene_proto_list_on_enter(void* context) {
    ModulationFlipApp* app = context;
    char title[56];
    size_t count = compute_list_for_current_mode(app, title, sizeof(title));

    with_view_model(
        app->list_view,
        ProtoListModel * m,
        {
            snprintf(m->title, sizeof(m->title), "%s", title);
            m->count = count;
            if(count > 0) {
                memcpy(m->indices, app->list_indices, count);
            }
            m->cursor = (app->list_cursor < count) ? app->list_cursor : 0;
            m->tick = 0;
            m->needs_scroll = false;
        },
        true);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIdProtoList);
}

static bool scene_proto_list_on_event(void* context, SceneManagerEvent event) {
    ModulationFlipApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == CustomEventListSelect) {
        scene_manager_next_scene(app->scene_manager, SceneDetail);
        return true;
    }
    return false;
}

static void scene_proto_list_on_exit(void* context) {
    UNUSED(context);
}

static void scene_detail_on_enter(void* context) {
    ModulationFlipApp* app = context;
    populate_detail_model(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIdDetail);
}

static bool scene_detail_on_event(void* context, SceneManagerEvent event) {
    ModulationFlipApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(app->list_count == 0) return false;

    if(event.event == CustomEventDetailPrev) {
        if(app->list_cursor > 0) {
            app->list_cursor--;
        } else {
            app->list_cursor = app->list_count - 1;
        }
        populate_detail_model(app);
        return true;
    }
    if(event.event == CustomEventDetailNext) {
        if(app->list_cursor + 1 < app->list_count) {
            app->list_cursor++;
        } else {
            app->list_cursor = 0;
        }
        populate_detail_model(app);
        return true;
    }
    return false;
}

static void scene_detail_on_exit(void* context) {
    UNUSED(context);
}

static void scene_info_on_enter(void* context) {
    ModulationFlipApp* app = context;
    widget_reset(app->widget);
    widget_add_text_scroll_element(app->widget, 0, 0, 128, 64, kRfPresetInfoText);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIdWidget);
}

static bool scene_info_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

static void scene_info_on_exit(void* context) {
    ModulationFlipApp* app = context;
    widget_reset(app->widget);
}

static void splash_view_draw_callback(Canvas* canvas, void* _model) {
    SplashViewModel* m = _model;
    ui_draw_splash(canvas, m->tick);
}

static bool splash_view_input_callback(InputEvent* event, void* context) {
    ModulationFlipApp* app = context;
    if(event->key == InputKeyBack) return false;
    if(event->type == InputTypePress) return true;
    if(event->type != InputTypeShort && event->type != InputTypeLong) return false;

    bool emit_done = false;
    with_view_model(
        app->splash_view,
        SplashViewModel * m,
        {
            if(!m->done) {
                m->done = true;
                emit_done = true;
            }
        },
        false);
    if(emit_done) {
        view_dispatcher_send_custom_event(app->view_dispatcher, CustomEventSplashDone);
    }
    return true;
}

static void scene_splash_on_enter(void* context) {
    ModulationFlipApp* app = context;
    with_view_model(
        app->splash_view,
        SplashViewModel * m,
        {
            m->tick = 0;
            m->done = false;
        },
        true);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIdSplash);
}

static bool scene_splash_on_event(void* context, SceneManagerEvent event) {
    ModulationFlipApp* app = context;
    if(event.type == SceneManagerEventTypeCustom && event.event == CustomEventSplashDone) {
        scene_manager_previous_scene(app->scene_manager);
        return true;
    }
    if(event.type == SceneManagerEventTypeBack) {
        scene_manager_stop(app->scene_manager);
        view_dispatcher_stop(app->view_dispatcher);
        return true;
    }
    return false;
}

static void scene_splash_on_exit(void* context) {
    UNUSED(context);
}

static const AppSceneOnEnterCallback kSceneOnEnterHandlers[] = {
    [SceneStart] = scene_start_on_enter,
    [SceneBrands] = scene_brands_on_enter,
    [SceneFilter] = scene_filter_on_enter,
    [SceneSearch] = scene_search_on_enter,
    [SceneNoMatch] = scene_no_match_on_enter,
    [SceneProtoList] = scene_proto_list_on_enter,
    [SceneDetail] = scene_detail_on_enter,
    [SceneInfo] = scene_info_on_enter,
    [SceneSplash] = scene_splash_on_enter,
};

static const AppSceneOnEventCallback kSceneOnEventHandlers[] = {
    [SceneStart] = scene_start_on_event,
    [SceneBrands] = scene_brands_on_event,
    [SceneFilter] = scene_filter_on_event,
    [SceneSearch] = scene_search_on_event,
    [SceneNoMatch] = scene_no_match_on_event,
    [SceneProtoList] = scene_proto_list_on_event,
    [SceneDetail] = scene_detail_on_event,
    [SceneInfo] = scene_info_on_event,
    [SceneSplash] = scene_splash_on_event,
};

static const AppSceneOnExitCallback kSceneOnExitHandlers[] = {
    [SceneStart] = scene_start_on_exit,
    [SceneBrands] = scene_brands_on_exit,
    [SceneFilter] = scene_filter_on_exit,
    [SceneSearch] = scene_search_on_exit,
    [SceneNoMatch] = scene_no_match_on_exit,
    [SceneProtoList] = scene_proto_list_on_exit,
    [SceneDetail] = scene_detail_on_exit,
    [SceneInfo] = scene_info_on_exit,
    [SceneSplash] = scene_splash_on_exit,
};

static const SceneManagerHandlers kSceneHandlers = {
    .on_enter_handlers = kSceneOnEnterHandlers,
    .on_event_handlers = kSceneOnEventHandlers,
    .on_exit_handlers = kSceneOnExitHandlers,
    .scene_num = SceneCount,
};

// --- ViewDispatcher Callbacks ---

static bool navigation_event_callback(void* context) {
    ModulationFlipApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static bool custom_event_callback(void* context, uint32_t event) {
    ModulationFlipApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static void tick_event_callback(void* context) {
    ModulationFlipApp* app = context;
    uint32_t scene = scene_manager_get_current_scene(app->scene_manager);

    if(scene == SceneSplash) {
        bool emit_done = false;
        with_view_model(
            app->splash_view,
            SplashViewModel * m,
            {
                if(!m->done) {
                    m->tick++;
                    if(m->tick >= SPLASH_DURATION_TICKS) {
                        m->done = true;
                        emit_done = true;
                    }
                }
            },
            true);
        if(emit_done) {
            view_dispatcher_send_custom_event(app->view_dispatcher, CustomEventSplashDone);
        }
    } else if(scene == SceneStart || scene == SceneBrands || scene == SceneFilter) {
        bool do_update = false;
        with_view_model(
            app->menu_view,
            MenuViewModel * m,
            {
                if(m->needs_scroll) {
                    m->tick++;
                    do_update = true;
                }
            },
            do_update);
    } else if(scene == SceneProtoList) {
        bool do_update = false;
        with_view_model(
            app->list_view,
            ProtoListModel * m,
            {
                if(m->needs_scroll) {
                    m->tick++;
                    do_update = true;
                }
            },
            do_update);
    } else if(scene == SceneDetail) {
        bool do_update = false;
        with_view_model(
            app->detail_view,
            DetailViewModel * m,
            {
                if(m->needs_scroll) {
                    m->tick++;
                    do_update = true;
                }
            },
            do_update);
    }
}

// --- App Alloc / Free ---

static ModulationFlipApp* modulation_flip_app_alloc(void) {
    ModulationFlipApp* app = malloc(sizeof(ModulationFlipApp));
    furi_assert(app);
    memset(app, 0, sizeof(ModulationFlipApp));

    for(size_t i = 0; i < kProtocolCount; i++) {
        for(size_t f = 0; f < FilterCount; f++) {
            if(entry_matches_filter(&kProtocols[i], (FilterOption)f)) {
                app->filter_counts[f]++;
            }
        }
    }

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&kSceneHandlers, app);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, navigation_event_callback);
    view_dispatcher_set_tick_event_callback(app->view_dispatcher, tick_event_callback, 150);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Custom views
    app->menu_view = view_alloc();
    view_allocate_model(app->menu_view, ViewModelTypeLocking, sizeof(MenuViewModel));
    view_set_context(app->menu_view, app);
    view_set_draw_callback(app->menu_view, menu_view_draw_callback);
    view_set_input_callback(app->menu_view, menu_view_input_callback);
    view_dispatcher_add_view(app->view_dispatcher, ViewIdMenu, app->menu_view);

    app->list_view = view_alloc();
    view_allocate_model(app->list_view, ViewModelTypeLocking, sizeof(ProtoListModel));
    view_set_context(app->list_view, app);
    view_set_draw_callback(app->list_view, list_view_draw_callback);
    view_set_input_callback(app->list_view, list_view_input_callback);
    view_dispatcher_add_view(app->view_dispatcher, ViewIdProtoList, app->list_view);

    app->detail_view = view_alloc();
    view_allocate_model(app->detail_view, ViewModelTypeLocking, sizeof(DetailViewModel));
    view_set_context(app->detail_view, app);
    view_set_draw_callback(app->detail_view, detail_view_draw_callback);
    view_set_input_callback(app->detail_view, detail_view_input_callback);
    view_dispatcher_add_view(app->view_dispatcher, ViewIdDetail, app->detail_view);

    app->splash_view = view_alloc();
    view_allocate_model(app->splash_view, ViewModelTypeLocking, sizeof(SplashViewModel));
    view_set_context(app->splash_view, app);
    view_set_draw_callback(app->splash_view, splash_view_draw_callback);
    view_set_input_callback(app->splash_view, splash_view_input_callback);
    view_dispatcher_add_view(app->view_dispatcher, ViewIdSplash, app->splash_view);

    // SDK modules
    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, ViewIdTextInput, text_input_get_view(app->text_input));

    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, ViewIdWidget, widget_get_view(app->widget));

    app->popup = popup_alloc();
    view_dispatcher_add_view(app->view_dispatcher, ViewIdPopup, popup_get_view(app->popup));

    scene_manager_next_scene(app->scene_manager, SceneStart);
    scene_manager_next_scene(app->scene_manager, SceneSplash);
    return app;
}

static void modulation_flip_app_free(ModulationFlipApp* app) {
    furi_assert(app);

    view_dispatcher_remove_view(app->view_dispatcher, ViewIdPopup);
    popup_free(app->popup);

    view_dispatcher_remove_view(app->view_dispatcher, ViewIdWidget);
    widget_free(app->widget);

    view_dispatcher_remove_view(app->view_dispatcher, ViewIdTextInput);
    text_input_free(app->text_input);

    view_dispatcher_remove_view(app->view_dispatcher, ViewIdSplash);
    view_free(app->splash_view);

    view_dispatcher_remove_view(app->view_dispatcher, ViewIdDetail);
    view_free(app->detail_view);

    view_dispatcher_remove_view(app->view_dispatcher, ViewIdProtoList);
    view_free(app->list_view);

    view_dispatcher_remove_view(app->view_dispatcher, ViewIdMenu);
    view_free(app->menu_view);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);

    free(app);
}

int32_t modulation_flip_app(void* p) {
    UNUSED(p);
    ModulationFlipApp* app = modulation_flip_app_alloc();
    view_dispatcher_run(app->view_dispatcher);
    modulation_flip_app_free(app);
    return 0;
}
