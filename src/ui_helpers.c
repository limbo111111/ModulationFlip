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
