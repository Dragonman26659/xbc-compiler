/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "../overlay/target/xbc16/core.h"
#include <stdio.h>
#include <stdlib.h>
static uint16_t ram[XBC16_WORDS];
static uint16_t read_word(void *opaque,uint32_t a) { return ram[a]; }
static void write_word(void *opaque,uint32_t a,uint16_t v) { ram[a]=v; }
int main(int argc,char **argv) {
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;
    unsigned i=0;int lo,hi;
    while((lo=fgetc(f))!=EOF) { hi=fgetc(f);if(hi==EOF||i>=XBC16_WORDS)return 2;ram[i++]=lo|(hi<<8); }
    fclose(f);if(!i)return 2;
    XBC16State s;xbc16_reset(&s);XBC16Bus b={NULL,read_word,write_word};
    while(!s.halted && !s.fatal && s.instructions<1000000)xbc16_step(&s,&b);
    if(s.fatal || !s.halted){fprintf(stderr,"%s\n",s.error?s.error:"instruction limit");return 1;}
    for(unsigned r=0;r<16;++r)printf("%u ",s.r[r]);
    printf("%u %u %d %d %d\n",s.acc,s.pc,s.z,s.n,s.unordered);
    return 0;
}
