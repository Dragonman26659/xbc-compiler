/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifdef COMPILING_PER_TARGET
#include "qemu/osdep.h"
#endif
#include "core.h"
#include <math.h>
#include <string.h>

double xbc16_decode_half(uint16_t bits) {
    bool isNegative = (bits & 0x8000) != 0;
    unsigned exponent = (bits >> 10) & 0x1F;
    unsigned fraction = bits & 0x03FF;

    double value = 0;

    if (exponent == 0) {
        value = ldexp((fraction), -24);
    }
    else if (exponent == 31) {
        if (fraction == 0) {
            value = INFINITY;
        }
        else {
            value = NAN;
        }
    }
    else {
        value = ldexp((1024 + fraction),
            (int)exponent - 25);
    }

    if (isNegative) {
        return -value;
    }

    return value;
}


static unsigned round_even(double value) {
    double wholeValue = floor(value);
    unsigned roundedValue = (wholeValue);
    double fraction = value - wholeValue;

    if (fraction > 0.5 || (fraction == 0.5 && (roundedValue & 1u))) {
        ++roundedValue;
    }

    return roundedValue;
}


uint16_t xbc16_encode_half(double value) {
    uint16_t signBit = 0;

    if (signbit(value)) {
        signBit = 0x8000;
    }

    double magnitude = fabs(value);

    if (isnan(value)) {
        return 0x7E00;
    }

    if (isinf(value)) {
        return (signBit | 0x7C00);
    }

    if (magnitude == 0) {
        return signBit;
    }

    if (magnitude < ldexp(1.0, -14)) {
        unsigned fraction = round_even(ldexp(magnitude, 24));
        return (signBit | fraction);
    }

    int exponent = 0;
    frexp(magnitude, &exponent);
    --exponent;

    if (exponent > 15) {
        return (signBit | 0x7C00);
    }

    unsigned significand = round_even(ldexp(magnitude, 10 - exponent));

    if (significand == 2048) {
        significand = 1024;
        ++exponent;
    }

    if (exponent > 15) {
        return (signBit | 0x7C00);
    }

    return (uint16_t)(signBit | ((exponent + 15) << 10) |
        (significand - 1024));
}



static bool address_ok(uint32_t addr) {
    return addr < XBC16_WORDS && !(addr >= XBC16_STACK_LO && addr < XBC16_STACK_END);
}
static bool sp_ok(const XBC16State *s) {
    return s->r[15] >= XBC16_STACK_LO && s->r[15] <= XBC16_STACK_END;
}
static void fail(XBC16State *s, const char *message) { s->fatal = true; s->error = message; }
static int32_t signed_word(uint32_t v) { return (v & 0x8000) ? (int32_t)(v & 65535)-65536 : (int32_t)(v & 65535); }
static void integer_result(XBC16State *s, int64_t v) {
    s->acc = (uint16_t)v; s->z = !s->acc; s->n = (s->acc & 0x8000) != 0; s->unordered = false;
}
static void float_result(XBC16State *s, double v) {
    s->acc = xbc16_encode_half(v); v = xbc16_decode_half(s->acc);
    s->z = v == 0; s->n = v < 0; s->unordered = isnan(v);
}
void xbc16_reset(XBC16State *s) { memset(s, 0, sizeof(*s)); s->r[15] = XBC16_STACK_END; }
bool xbc16_queue(XBC16State *s, unsigned vector, uint16_t a, uint16_t b, uint16_t c) {
    if (vector > 2 || s->queue_count == XBC16_QUEUE) return false;
    s->queue[(s->queue_head+s->queue_count++) % XBC16_QUEUE] = (XBC16Event){vector,a,b,c};
    return true;
}
static void push(XBC16State *s, const XBC16Bus *b, uint16_t value) { b->write(b->opaque,--s->r[15],value); }
static uint16_t pop(XBC16State *s, const XBC16Bus *b) { return b->read(b->opaque,s->r[15]++); }
static void dispatch(XBC16State *s, const XBC16Bus *b, unsigned vector, bool fault,
                     uint32_t return_pc, XBC16Event event) {
    unsigned frame = fault ? 2 : XBC16_IRQ_FRAME;
    uint16_t target = b->read(b->opaque,XBC16_IDT+vector);
    if (!sp_ok(s) || s->r[15]-XBC16_STACK_LO < frame) { fail(s,"interrupt frame cannot fit on stack"); return; }
    if (!address_ok(target)) { fail(s,"invalid IDT handler address"); return; }
    uint16_t old_eax = s->r[0];
    push(s,b,return_pc);
    if (fault) { push(s,b,old_eax); s->r[0] = vector; }
    else {
        /* Only R1-R14 are program registers. SP changes through PUSH/POP;
         * R15 is a volatile RAM-read latch, not part of the IRQ snapshot. */
        for (int i=XBC16_IRQ_REGS-1;i>=0;--i) push(s,b,s->r[i]);
        if (vector < 2) s->r[0] = event.a;
        if (vector == 1) { s->r[1] = event.b; s->r[2] = event.c; }
    }
    s->irq_enable = false; s->pc = target;
}
void xbc16_step(XBC16State *s, const XBC16Bus *b) {
    uint32_t old_pc = s->pc;
    unsigned fault = 0;
    if (s->halted || s->fatal) return;
    ++s->instructions;
    if (!address_ok(s->pc)) { fault=3; goto trap; }
    uint16_t insn = b->read(b->opaque,s->pc);
    if (insn == 65535) { s->halted=true; return; }
    ++s->pc;
    if ((insn & 0xf000) == 0xf000) {
        unsigned op=(insn>>8)&15, a=(insn>>4)&15, c=insn&15;
        uint16_t va=s->r[a], vb=s->r[c];
        double fa=xbc16_decode_half(va), fb=xbc16_decode_half(vb);
        switch (op) {
        case 8: integer_result(s,(uint64_t)va*vb); break;
        case 4: if (!vb) { fault=6; goto trap; } integer_result(s,va/vb); break;
        case 12: float_result(s,fa+fb); break;
        case 2: float_result(s,fa-fb); break;
        case 10: float_result(s,fa*fb); break;
        case 6: if (fb == 0) { fault=6; goto trap; } float_result(s,fa/fb); break;
        case 14: integer_result(s,(int64_t)signed_word(va)*signed_word(vb)); break;
        case 1: if (!vb) { fault=6; goto trap; } integer_result(s,(int64_t)signed_word(va)/signed_word(vb)); break;
        case 9: float_result(s,fa-fb); break;
        case 5: s->r[c]=xbc16_encode_half(signed_word(va)); break;
        case 13: s->r[c]=xbc16_encode_half(va); break;
        case 3: case 11: {
            double v=trunc(fa), lo=op==3 ? -32768 : 0, hi=op==3 ? 32767 : 65535;
            if (!isfinite(v) || v<lo || v>hi) { fail(s,"float-to-integer conversion out of range"); return; }
            s->r[c]=(uint16_t)(int64_t)v; break;
        }
        default: fail(s,"invalid extension opcode"); return;
        }
    } else {
        insn &= 255;
        if (insn < 16) {
            if (!address_ok(s->pc)) { fault=3; goto trap; }
            s->r[insn]=b->read(b->opaque,s->pc++);
        } else if ((insn&0xe0)==0xe0) {
            unsigned a=insn&3;
            switch (insn&0xfc) {
            case 0xf0:
                if (!address_ok(s->acc)) { fault=3; goto trap; }
                s->r[a]=s->r[14]=b->read(b->opaque,s->acc); break;
            case 0xe8:
                if (!address_ok(s->acc)) { fault=3; goto trap; }
                b->write(b->opaque,s->acc,s->r[a]); break;
            case 0xf8: integer_result(s,s->r[a]<<1); s->r[a]=s->acc; break;
            case 0xe4: integer_result(s,s->r[a]>>1); s->r[a]=s->acc; break;
            case 0xf4:
                if (!sp_ok(s)) { fault=3; goto trap; }
                if (s->r[15]==XBC16_STACK_END) { fault=5; goto trap; }
                s->r[a]=pop(s,b); break;
            case 0xec:
                if (!sp_ok(s)) { fault=3; goto trap; }
                if (s->r[15]==XBC16_STACK_LO) { fault=4; goto trap; }
                push(s,b,s->r[a]); break;
            case 0xe0:
                if (insn&1) { fail(s,"invalid SETIRR reserved bit"); return; }
                s->irq_enable=(insn&2)!=0; break;
            default: fail(s,"invalid memory opcode"); return;
            }
        } else {
            unsigned a=(insn>>3)&3, c=(insn>>1)&3, single=(insn>>1)&15;
            switch (insn&0xe1) {
            case 0x80: integer_result(s,s->r[a]+s->r[c]); break;
            case 0x40: integer_result(s,(int32_t)s->r[a]-(int32_t)s->r[c]); break;
            case 0xc0: integer_result(s,s->r[a]^s->r[c]); break;
            case 0x20: integer_result(s,s->r[a]|s->r[c]); break;
            case 0x60: integer_result(s,s->r[a]&s->r[c]); break;
            case 0xa0: integer_result(s,~s->r[single]); break;
            case 0x81: s->pc=s->r[single]; break;
            case 0x41: if (s->z) s->pc=s->r[single]; break;
            case 0xc1: if (s->n) s->pc=s->r[single]; break;
            case 0x21: if (!s->z && !s->n && !s->unordered) s->pc=s->r[single]; break;
            case 0xa1: s->r[single]=s->acc; break;
            case 0x61: integer_result(s,s->r[single]); break;
            default: fail(s,"invalid legacy opcode"); return;
            }
        }
    }
    uint16_t period=b->read(b->opaque,XBC16_TIMER);
    if (!period) s->timer_ticks=0;
    else if (++s->timer_ticks>=period) {
        s->timer_ticks=0;
        bool pending=false;
        for (unsigned i=0;i<s->queue_count;++i)
            pending |= s->queue[(s->queue_head+i)%XBC16_QUEUE].vector==2;
        if (!pending && !xbc16_queue(s,2,0,0,0)) { fail(s,"device event queue full"); return; }
    }
    if (s->irq_enable && s->queue_count) {
        XBC16Event event=s->queue[s->queue_head];
        dispatch(s,b,event.vector,false,s->pc,event);
        if (!s->fatal) { s->queue_head=(s->queue_head+1)%XBC16_QUEUE; --s->queue_count; }
    }
    return;
trap:
    dispatch(s,b,fault,true,old_pc,(XBC16Event){0});
}
