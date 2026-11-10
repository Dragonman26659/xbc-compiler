/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../overlay/hw/xbc16/text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t pixels[XBC16_TEXT_HEIGHT][XBC16_TEXT_WIDTH + 4];
int main(void) {
    uint8_t font[256 * 16] = {0};
    font['A' * 16] = 0x81;
    memset(pixels, 0x5a, sizeof(pixels));
    xbc16_text_cell((uint8_t *)pixels, sizeof(pixels[0]), 0, 0x1e41, font);
    assert(pixels[0][0] == 0xffff55 && pixels[0][7] == 0xffff55);
    assert(pixels[0][1] == 0x0000aa && pixels[1][0] == 0x0000aa);
    assert(pixels[0][8] == 0x5a5a5a5a);
    xbc16_text_cell((uint8_t *)pixels, sizeof(pixels[0]), 1999, 0xf120, font);
    assert(pixels[399][639] == 0xffffff);
    assert(pixels[399][640] == 0x5a5a5a5a);
    assert(XBC16_TEXT_BASE + XBC16_TEXT_CELLS - 1 == 65533);
    puts("VGA text raster, palette, stride and bounds tests passed");
}
