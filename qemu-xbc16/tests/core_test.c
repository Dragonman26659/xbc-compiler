/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../overlay/target/xbc16/core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint16_t ram[XBC16_WORDS];
static uint16_t read_word(void *opaque, uint32_t a) { assert(a<XBC16_WORDS); return ram[a]; }
static void write_word(void *opaque, uint32_t a, uint16_t v) { assert(a<XBC16_WORDS); ram[a]=v; }
static XBC16Bus bus={NULL,read_word,write_word};
static void start(XBC16State *s) { memset(ram,0,sizeof(ram)); xbc16_reset(s); }
int main(void) {
    XBC16State s;
    for (unsigned i=0;i<65536;++i) {
        if ((i&0x7c00)!=0x7c00 || !(i&1023)) assert(xbc16_encode_half(xbc16_decode_half(i))==i);
    }
    assert(xbc16_encode_half(1.00048828125)==0x3c00);
    start(&s); ram[0]=0;ram[1]=65535;ram[2]=1;ram[3]=2;ram[4]=0x1282;ram[5]=0xa1;ram[6]=65535;
    while (!s.halted && !s.fatal) xbc16_step(&s,&bus);
    assert(!s.fatal && s.pc==6 && s.r[0]==1 && s.acc==1 && !s.z && !s.n);
    start(&s);ram[0]=0xec;ram[1]=0xf5;ram[2]=65535;s.r[0]=42;
    xbc16_step(&s,&bus);assert(s.r[15]==63275 && ram[63275]==42);
    xbc16_step(&s,&bus);assert(s.r[1]==42 && s.r[15]==63276);
    start(&s);ram[0]=0xf0;s.acc=59180;s.r[0]=7;ram[XBC16_IDT+3]=100;
    xbc16_step(&s,&bus);assert(s.pc==100 && s.r[0]==3 && ram[63274]==7 && ram[63275]==0);
    start(&s);ram[0]=0xf401;s.r[0]=100;ram[XBC16_IDT+6]=100;
    xbc16_step(&s,&bus);assert(s.pc==100 && s.r[0]==6 && ram[63274]==100);
    start(&s);ram[0]=0xf4;ram[XBC16_IDT+5]=100;
    xbc16_step(&s,&bus);assert(s.pc==100 && s.r[0]==5);
    start(&s);ram[0]=0xec;s.r[15]=59180;
    xbc16_step(&s,&bus);assert(s.fatal && s.r[15]==59180);
    start(&s);ram[0]=0xe2;ram[XBC16_IDT]=100;
    for(unsigned i=0;i<15;i++)s.r[i]=10+i;
    assert(xbc16_queue(&s,0,65,0,0));
    xbc16_step(&s,&bus);assert(s.pc==100 && s.r[0]==65 && s.r[15]==63261);
    assert(ram[63275]==1 && ram[63274]==23 && s.r[14]==24 && !s.irq_enable);
    for(unsigned i=0;i<14;i++)assert(ram[63261+i]==10+i);
    start(&s);ram[0]=0xe2;ram[XBC16_IDT+1]=100;assert(xbc16_queue(&s,1,2,65533,4));
    xbc16_step(&s,&bus);assert(s.r[0]==2 && s.r[1]==65533 && s.r[2]==4);
    start(&s);ram[0]=0xe2;ram[65535]=1;ram[XBC16_IDT+2]=100;
    xbc16_step(&s,&bus);assert(s.pc==100 && s.r[15]==63261);
    start(&s);s.acc=123;s.z=true;ram[0]=0xf501;s.r[0]=3;
    xbc16_step(&s,&bus);assert(s.acc==123 && s.z && s.r[1]==0x4200);
    start(&s);ram[0]=0xf901;s.r[0]=0x3c00;s.r[1]=0x4000;
    xbc16_step(&s,&bus);assert(s.acc==0xbc00 && s.n);
    start(&s);for(unsigned i=0;i<XBC16_QUEUE;++i)assert(xbc16_queue(&s,0,i,0,0));
    assert(!xbc16_queue(&s,0,100,0,0));
    /* New frame fits in exactly 15 free words, for all three device vectors. */
    for(unsigned vector=0;vector<3;vector++) {
        start(&s);ram[0]=0xe2;ram[XBC16_IDT+vector]=100;
        s.r[15]=XBC16_STACK_LO+XBC16_IRQ_FRAME;
        for(unsigned i=0;i<15;i++)s.r[i]=0x100+i;
        assert(xbc16_queue(&s,vector,1,2,3));xbc16_step(&s,&bus);
        assert(!s.fatal && s.r[15]==XBC16_STACK_LO && s.r[14]==0x10e);
        for(unsigned i=0;i<14;i++)assert(ram[XBC16_STACK_LO+i]==0x100+i);
        assert(ram[XBC16_STACK_LO+14]==1);
        start(&s);ram[0]=0xe2;ram[XBC16_IDT+vector]=100;
        s.r[15]=XBC16_STACK_LO+XBC16_IRQ_FRAME-1;
        ram[XBC16_STACK_LO]=0xbeef;
        assert(xbc16_queue(&s,vector,1,2,3));xbc16_step(&s,&bus);
        assert(s.fatal && s.r[15]==XBC16_STACK_LO+14 && ram[XBC16_STACK_LO]==0xbeef);
    }
    puts("XBC16 core tests passed");
}
