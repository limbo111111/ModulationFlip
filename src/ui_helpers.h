#pragma once

#include <gui/canvas.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    UiMenuIconBrand = 0,
    UiMenuIconSearch,
    UiMenuIconCar,
    UiMenuIconGate,
    UiMenuIconFilter,
    UiMenuIconAll,
    UiMenuIconInfo,
    UiMenuIconCount,
} UiMenuIcon;

/** Draw a 7x7 monochrome icon at (x, y) using the currently active canvas color. */
void ui_draw_menu_icon(Canvas* canvas, UiMenuIcon icon, int32_t x, int32_t y);

/** Draw 2x scaled crisp modulation glyphs ("AM", "FM", "A/F") inside a box centered around cx, cy.
 *  Falls back to FontPrimary centered for other short strings such as "A/F*".
 */
void ui_draw_hero_rf(
    Canvas* canvas,
    int32_t box_x,
    int32_t box_y,
    size_t box_w,
    size_t box_h,
    const char* rf_short);

/** Draw a single top-aligned text line within max_w pixels.
 *  If the text exceeds max_w:
 *   - when tick == 0, truncates smoothly with "..."
 *   - when tick > 0, pauses briefly at offset 0 then scrolls horizontally with "  -  " wrap
 */
bool ui_draw_marquee_top(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t max_w,
    const char* text,
    uint16_t tick);

/** Case-insensitive substring search (ASCII). */
bool ui_str_contains_ci(const char* haystack, const char* needle);
