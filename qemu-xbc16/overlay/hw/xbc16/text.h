/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XBC16_TEXT_H
#define XBC16_TEXT_H
#include <stdint.h>
#define XBC16_TEXT_BASE 63534u
#define XBC16_TEXT_COLS 80u
#define XBC16_TEXT_ROWS 25u
#define XBC16_TEXT_CELLS (XBC16_TEXT_COLS * XBC16_TEXT_ROWS)
#define XBC16_TEXT_WIDTH (XBC16_TEXT_COLS * 8u)
#define XBC16_TEXT_HEIGHT (XBC16_TEXT_ROWS * 16u)
/* VGA-style attributes: low byte CP437 glyph, high byte BG:FG.
 * Attribute bit 7 selects bright background, rather than blinking. */
static inline void xbc16_text_cell(uint8_t *pixels, unsigned stride,
                                 unsigned cell, uint16_t word,
                                 const uint8_t *font)
{
    static const uint32_t palette[16] = {
        0x000000, 0x0000aa, 0x00aa00, 0x00aaaa,
        0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
        0x555555, 0x5555ff, 0x55ff55, 0x55ffff,
        0xff5555, 0xff55ff, 0xffff55, 0xffffff,
    };
    unsigned x = (cell % XBC16_TEXT_COLS) * 8;
    unsigned y = (cell / XBC16_TEXT_COLS) * 16;
    uint32_t fg = palette[(word >> 8) & 15];
    uint32_t bg = palette[(word >> 12) & 15];
    for (unsigned row = 0; row < 16; row++) {
        uint32_t *dest = (uint32_t *)(pixels + (y + row) * stride) + x;
        uint8_t bits = font[(word & 255) * 16 + row];
        for (unsigned col = 0; col < 8; col++) {
            dest[col] = (bits & (0x80 >> col)) ? fg : bg;
        }
    }
}
#endif
