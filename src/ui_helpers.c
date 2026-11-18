#include "ui_helpers.h"

#include <ctype.h>
#include <string.h>

static const uint8_t kMenuIcons[UiMenuIconCount][7] = {
    // Each byte row uses bits [6..0] left-to-right (0x40 = leftmost pixel)
    [UiMenuIconBrand] =
        {
            0x3E, // .#####.
            0x41, // #.....#
            0x49, // #..#..#
            0x49, // #..#..#
            0x41, // #.....#
            0x22, // .#...#.
            0x1C, // ..###..
        },
    [UiMenuIconSearch] =
        {
            0x38, // ..###..
            0x44, // .#...#.
            0x44, // .#...#.
            0x38, // ..###..
            0x14, // ...#.#.
            0x02, // .....#.
            0x01, // ......#
        },
    [UiMenuIconCar] =
        {
            0x00, // .......
            0x3C, // ..####.
            0x42, // .#....#
            0x7F, // #######
            0x5D, // #.###.#
            0x7F, // #######
            0x22, // .#...#.
        },
    [UiMenuIconGate] =
        {
            0x7F, // #######
            0x55, // #.#.#.#
            0x55, // #.#.#.#
            0x7F, // #######
            0x55, // #.#.#.#
            0x41, // #.....#
            0x00, // .......
        },
    [UiMenuIconFilter] =
        {
            0x10, // ..#....
            0x10, // ..#....
            0x7E, // ######.
            0x46, // #...##.
            0x56, // #.#.##.
            0x7E, // ######.
            0x00, // .......
        },
    [UiMenuIconAll] =
        {
            0x5E, // #.####.
            0x00, // .......
            0x5E, // #.####.
            0x00, // .......
            0x5E, // #.####.
            0x00, // .......
            0x5E, // #.####.
        },
    [UiMenuIconInfo] =
        {
            0x3E, // .#####.
            0x49, // #..#..#
            0x41, // #.....#
            0x49, // #..#..#
            0x49, // #..#..#
            0x49, // #..#..#
            0x3E, // .#####.
        },
};

void ui_draw_menu_icon(Canvas* canvas, UiMenuIcon icon, int32_t x, int32_t y) {
    if(icon >= UiMenuIconCount) return;
    for(int32_t row = 0; row < 7; row++) {
        uint8_t bits = kMenuIcons[icon][row];
        for(int32_t col = 0; col < 7; col++) {
            if(bits & (1u << (6 - col))) {
                canvas_draw_dot(canvas, x + col, y + row);
            }
        }
    }
}

static const uint8_t* hero_glyph_for_char(char ch) {
    static const uint8_t glyph_a[6] = {0x6, 0x9, 0x9, 0xF, 0x9, 0x9};
    static const uint8_t glyph_f[6] = {0xF, 0x8, 0xE, 0x8, 0x8, 0x8};
    static const uint8_t glyph_m[6] = {0x9, 0xF, 0xF, 0x9, 0x9, 0x9};
    static const uint8_t glyph_slash[6] = {0x1, 0x2, 0x2, 0x4, 0x4, 0x8};

    switch(ch) {
    case 'A':
        return glyph_a;
    case 'F':
        return glyph_f;
    case 'M':
        return glyph_m;
    case '/':
        return glyph_slash;
    default:
        return NULL;
    }
}

void ui_draw_hero_rf(
    Canvas* canvas,
    int32_t box_x,
    int32_t box_y,
    size_t box_w,
    size_t box_h,
    const char* rf_short) {
    size_t len = strlen(rf_short);
    bool all_bitmap = (len > 0 && len <= 3);
    for(size_t i = 0; i < len && all_bitmap; i++) {
        if(!hero_glyph_for_char(rf_short[i])) {
            all_bitmap = false;
        }
    }

    if(all_bitmap) {
        int32_t total_w = (int32_t)(len * 10 - 2);
        int32_t gx = box_x + ((int32_t)box_w - total_w) / 2;
        int32_t gy = box_y + ((int32_t)box_h - 12) / 2;
        for(size_t i = 0; i < len; i++) {
            const uint8_t* g = hero_glyph_for_char(rf_short[i]);
            for(int32_t row = 0; row < 6; row++) {
                uint8_t bits = g[row];
                for(int32_t col = 0; col < 4; col++) {
                    if(bits & (8u >> col)) {
                        canvas_draw_box(canvas, gx + col * 2, gy + row * 2, 2, 2);
                    }
                }
            }
            gx += 10;
        }
    } else {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(
            canvas,
            box_x + (int32_t)(box_w / 2),
            box_y + (int32_t)(box_h / 2) + 1,
            AlignCenter,
            AlignCenter,
            rf_short);
    }
}

bool ui_draw_marquee_top(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t max_w,
    const char* text,
    uint16_t tick) {
    if(!text || text[0] == '\0') return false;

    if(canvas_string_width(canvas, text) <= max_w) {
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, text);
        return false;
    }

    char buf[64];
    size_t len = strlen(text);
    const size_t pause_ticks = 6;
    const char* sep = "  -  ";
    const size_t sep_len = 5;

    if(tick == 0) {
        size_t copy_len = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
        memcpy(buf, text, copy_len);
        buf[copy_len] = '\0';

        while(copy_len > 3) {
            memcpy(buf + copy_len - 3, "...", 3);
            buf[copy_len] = '\0';
            if(canvas_string_width(canvas, buf) <= max_w) break;
            copy_len--;
        }
        canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, buf);
        return true;
    }

    size_t cycle = len + sep_len;
    size_t phase = tick % (cycle + pause_ticks);
    size_t start_pos = (phase < pause_ticks) ? 0 : (phase - pause_ticks);

    if(start_pos == 0) {
        size_t trim = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
        memcpy(buf, text, trim);
        buf[trim] = '\0';
        while(trim > 3) {
            memcpy(buf + trim - 3, "...", 3);
            buf[trim] = '\0';
            if(canvas_string_width(canvas, buf) <= max_w) break;
            trim--;
        }
    } else {
        size_t out_len = sizeof(buf) - 1;
        for(size_t i = 0; i < out_len; i++) {
            size_t pos = (start_pos + i) % cycle;
            buf[i] = (pos < len) ? text[pos] : sep[pos - len];
        }
        buf[out_len] = '\0';

        while(out_len > 1 && canvas_string_width(canvas, buf) > max_w) {
            out_len--;
            buf[out_len] = '\0';
        }
    }

    canvas_draw_str_aligned(canvas, x, y, AlignLeft, AlignTop, buf);
    return true;
}

void ui_draw_splash(Canvas* canvas, uint16_t tick) {
    // 20x24 LSB-first XBM car keyfob with emitter dome, TX LED window, 3 tactile buttons & keyring loop
    static const uint8_t kSplashKeyfobXbm[] = {
        0x00, 0x0F, 0x00, 0x80, 0x1F, 0x00, 0xC0, 0x30, 0x00, 0xE0, 0x70, 0x00, 0x30, 0xC0, 0x00,
        0x10, 0x80, 0x00, 0xD0, 0xBF, 0x00, 0x50, 0xA6, 0x00, 0xD0, 0xBF, 0x00, 0x10, 0x80, 0x00,
        0xD0, 0xBF, 0x00, 0x50, 0xAF, 0x00, 0xD0, 0xBF, 0x00, 0x10, 0x80, 0x00, 0xD0, 0xBF, 0x00,
        0x50, 0xA6, 0x00, 0xD0, 0xBF, 0x00, 0x10, 0x80, 0x00, 0x30, 0xC0, 0x00, 0xE0, 0x79, 0x00,
        0x80, 0x19, 0x00, 0x40, 0x20, 0x00, 0x40, 0x20, 0x00, 0x80, 0x1F, 0x00,
    };

    // Crisp right-facing RF emission arc lookup tables (centered vertically at y = 27)
    static const int8_t kArc1Dx[9] = {-2, -1, 0, 0, 0, 0, 0, -1, -2};
    static const int8_t kArc2Dx[13] = {-3, -2, -1, -1, 0, 0, 0, 0, 0, -1, -1, -2, -3};
    static const int8_t kArc3Dx[17] = {
        -4, -3, -2, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, -2, -3, -4};

    // Digital OOK/2-FSK carrier packet bits scrolled along bottom scope strip
    static const uint8_t kScopeBits[32] = {
        1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1,
    };

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    // 1. Hardware bezel & corner fiducials (strictly within 128x64)
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 3);
    canvas_draw_box(canvas, 4, 4, 2, 2);
    canvas_draw_box(canvas, 122, 4, 2, 2);
    canvas_draw_box(canvas, 4, 58, 2, 2);
    canvas_draw_box(canvas, 122, 58, 2, 2);

    // 2. Top title header + flanking RF chevrons
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 4, AlignCenter, AlignTop, "ModulationFlip");
    for(int32_t i = 0; i < 5; i++) {
        int32_t d = (i <= 2) ? (2 - i) : (i - 2);
        canvas_draw_dot(canvas, 16 - d, 6 + i);
        canvas_draw_dot(canvas, 12 - d, 6 + i);
        canvas_draw_dot(canvas, 111 + d, 6 + i);
        canvas_draw_dot(canvas, 115 + d, 6 + i);
    }
    canvas_draw_line(canvas, 5, 16, 122, 16);

    // 3. Left centerpiece: XBM car keyfob + pulsing TX LED + animated RF carrier arcs
    const int32_t fob_x = 8;
    const int32_t fob_y = 19;
    canvas_draw_xbm(canvas, fob_x, fob_y, 20, 24, kSplashKeyfobXbm);

    uint16_t phase = tick % 4u;
    if(phase <= 1u) {
        canvas_draw_box(canvas, fob_x + 9, fob_y + 2, 2, 2);
    }

    // Arc 1 (inner, x <= 31), Arc 2 (mid, x <= 35), Arc 3 (outer, x <= 39)
    for(int32_t i = 0; i < 9; i++) {
        int32_t ax = 31 + kArc1Dx[i];
        int32_t ay = 23 + i;
        if(phase == 0u || phase == 1u || phase == 2u) {
            canvas_draw_dot(canvas, ax, ay);
        }
    }
    if(phase >= 1u) {
        for(int32_t i = 0; i < 13; i++) {
            canvas_draw_dot(canvas, 35 + kArc2Dx[i], 21 + i);
        }
    }
    if(phase >= 2u) {
        for(int32_t i = 0; i < 17; i++) {
            canvas_draw_dot(canvas, 39 + kArc3Dx[i], 19 + i);
        }
    }

    // 4. Right centerpiece: Subtitle + prominent dynamically-sized inverted "by limbo" badge
    const int32_t info_cx = 84;
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, info_cx, 20, AlignCenter, AlignTop, "Sub-GHz RF Ref");

    const char* credit = "by limbo";
    int32_t pill_w = (int32_t)canvas_string_width(canvas, credit) + 12;
    if(pill_w > 76) pill_w = 76;
    int32_t pill_x = info_cx - (pill_w / 2);
    const int32_t pill_y = 31;
    canvas_draw_rbox(canvas, pill_x, pill_y, (size_t)pill_w, 12, 2);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_str_aligned(canvas, info_cx, pill_y + 2, AlignCenter, AlignTop, credit);

    // 5. Bottom oscilloscope pulse train + subtle [OK] skip prompt
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_line(canvas, 5, 46, 122, 46);

    const int32_t scope_y_hi = 49;
    const int32_t scope_y_lo = 56;
    uint8_t prev = kScopeBits[tick % 32u];
    for(int32_t seg = 0; seg < 22; seg++) {
        int32_t sx = 7 + seg * 4;
        uint8_t bit = kScopeBits[(tick + (uint16_t)seg) % 32u];
        int32_t sy = bit ? scope_y_hi : scope_y_lo;
        if(seg > 0 && bit != prev) {
            canvas_draw_line(canvas, sx, scope_y_hi, sx, scope_y_lo);
        }
        canvas_draw_line(canvas, sx, sy, sx + 3, sy);
        prev = bit;
    }

    canvas_draw_str_aligned(canvas, 122, 49, AlignRight, AlignTop, "[OK]");
}

bool ui_str_contains_ci(const char* haystack, const char* needle) {
    if(!needle || needle[0] == '\0') return true;
    if(!haystack || haystack[0] == '\0') return false;

    size_t n_len = strlen(needle);
    for(size_t i = 0; haystack[i] != '\0'; i++) {
        size_t j = 0;
        while(j < n_len && haystack[i + j] != '\0') {
            char a = (char)tolower((unsigned char)haystack[i + j]);
            char b = (char)tolower((unsigned char)needle[j]);
            if(a != b) break;
            j++;
        }
        if(j == n_len) return true;
    }
    return false;
}
