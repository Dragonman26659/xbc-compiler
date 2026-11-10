/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../overlay/target/xbc16/core.h"
#include <assert.h>
#include <stdio.h>
static uint16_t ram[XBC16_WORDS];
static uint16_t rd(void *p, uint32_t a) { assert(a<XBC16_WORDS); return ram[a]; }
static void wr(void *p, uint32_t a, uint16_t v) { assert(a<XBC16_WORDS); ram[a]=v; }
static XBC16Bus bus={NULL,rd,wr};
static void steps(XBC16State *s, unsigned n) {
    for (unsigned i=0;i<n;i++) { xbc16_step(s,&bus); assert(!s->fatal && !s->halted); }
}
int main(int argc, char **argv) {
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb"); assert(f);
    for(unsigned i=0;i<XBC16_WORDS;i++) {
        int lo=fgetc(f); if(lo==EOF)break;
        int hi=fgetc(f); assert(hi!=EOF);ram[i]=lo|(hi<<8);
    }
    fclose(f);
    XBC16State s; xbc16_reset(&s);steps(&s,100000);
    assert(s.irq_enable && ram[63534]==0x1f31 && ram[65533]==0x1720);
    assert(ram[63724]==0x1e30 && ram[63961]==0x1e30 && ram[64029]==0x1f30);
    assert(ram[63723]==0x1e20 && ram[63960]==0x1e20 && ram[64028]==0x1f20);
    for(unsigned i=0;i<100;i++) {
        assert(xbc16_queue(&s,0,30,0,0));steps(&s,1000);
        assert(s.r[15]==XBC16_STACK_END && s.irq_enable && !s.queue_count);
        assert(ram[63799]==0x1f61);
        unsigned v=(i+1)&15;assert(ram[63724]==(0x1e00|(v<10?'0'+v:'A'+v-10)));
        assert(xbc16_queue(&s,1,5,65533,4));steps(&s,1000);
        assert(s.r[15]==XBC16_STACK_END && s.irq_enable && !s.queue_count);
        assert(ram[64029]==0x1f35);
        assert(ram[63961]==(0x1e00|(v<10?'0'+v:'A'+v-10)));
    }
    /* Real QEMU Set-1 make codes: W=0x11 A=0x1e S=0x1f D=0x20. */
    const unsigned codes[] = {0x11,0x1e,0x1f,0x20};
    const char chars[] = "wasd";
    for(unsigned i=0;i<4;i++) {
        assert(xbc16_queue(&s,0,codes[i],0,0));steps(&s,3000);
        assert(ram[63799]==(0x1f00|chars[i]));
        assert(xbc16_queue(&s,0,0x8000|codes[i],0,0));steps(&s,3000);
        assert(ram[63799]==(0x1f00|chars[i]));
        assert(ram[63873]==0x1f38); /* Raw release begins with hex 8. */
        assert(s.r[15]==XBC16_STACK_END && s.irq_enable);
    }
    assert(xbc16_queue(&s,0,42,0,0));steps(&s,3000); /* Left Shift */
    assert(xbc16_queue(&s,0,0x11,0,0));steps(&s,3000);
    assert(ram[63799]==0x1f57); /* W */
    assert(xbc16_queue(&s,0,2,0,0));steps(&s,3000);
    assert(ram[63799]==0x1f21); /* Shift+1 = ! */
    assert(xbc16_queue(&s,0,54,0,0));steps(&s,3000); /* Both shifts held */
    assert(xbc16_queue(&s,0,0x802a,0,0));steps(&s,3000);
    assert(xbc16_queue(&s,0,30,0,0));steps(&s,3000);
    assert(ram[63799]==0x1f41); /* Right shift still active */
    assert(xbc16_queue(&s,0,0x8036,0,0));steps(&s,3000);
    assert(xbc16_queue(&s,0,58,0,0));steps(&s,3000); /* Caps Lock */
    assert(xbc16_queue(&s,0,0x803a,0,0));steps(&s,3000);
    assert(xbc16_queue(&s,0,30,0,0));steps(&s,3000);
    assert(ram[63799]==0x1f41);
    assert(xbc16_queue(&s,0,42,0,0));steps(&s,3000);
    assert(xbc16_queue(&s,0,30,0,0));steps(&s,3000);
    assert(ram[63799]==0x1f61); /* Caps + Shift produces lowercase */
    assert(xbc16_queue(&s,0,0xc8,0,0));steps(&s,3000); /* Up is nonprinting */
    assert(ram[63799]==0x1f61 && ram[63873]==0x1f30 && ram[63875]==0x1f43 && ram[63876]==0x1f38);
    assert(s.r[15]==XBC16_STACK_END && s.irq_enable && !s.queue_count);
    puts("Graphics demo IRQs, WASD, release, Shift, Caps Lock and nonprinting keys passed");
}
