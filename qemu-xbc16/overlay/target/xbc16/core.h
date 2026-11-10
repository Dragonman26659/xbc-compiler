/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XBC16_CORE_H
#define XBC16_CORE_H
#include <stdint.h>
#include <stdbool.h>
#define XBC16_WORDS 65536u
#define XBC16_STACK_LO 59180u
#define XBC16_STACK_END 63276u
#define XBC16_IDT 63278u
#define XBC16_TIMER 65535u
#define XBC16_QUEUE 64u
#define XBC16_IRQ_REGS 14u
#define XBC16_IRQ_FRAME (1u + XBC16_IRQ_REGS)

typedef struct XBC16Event { uint16_t vector, a, b, c; } XBC16Event;
typedef struct XBC16State {
    uint32_t pc, r[16], acc;
    bool z, n, unordered, irq_enable, halted, fatal;
    uint64_t instructions, timer_ticks;
    uint32_t queue_head, queue_count;
    XBC16Event queue[XBC16_QUEUE];
    const char *error;
} XBC16State;
typedef struct XBC16Bus {
    void *opaque;
    uint16_t (*read)(void *opaque, uint32_t word_address);
    void (*write)(void *opaque, uint32_t word_address, uint16_t value);
} XBC16Bus;
void xbc16_reset(XBC16State *s);
bool xbc16_queue(XBC16State *s, unsigned vector, uint16_t a, uint16_t b, uint16_t c);
void xbc16_step(XBC16State *s, const XBC16Bus *bus);
double xbc16_decode_half(uint16_t bits);
uint16_t xbc16_encode_half(double value);
#endif
