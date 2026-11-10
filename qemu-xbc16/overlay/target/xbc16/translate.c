/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "qemu/osdep.h"
#include "cpu.h"
#include "tcg/tcg-op.h"
#include "exec/exec-all.h"
#include "exec/translator.h"
#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#define HELPER_H "helper.h"
#include "exec/helper-info.c.inc"
#undef HELPER_H

typedef struct DisasContext { DisasContextBase base; } DisasContext;
static void init_context(DisasContextBase *dc, CPUState *cs) { dc->max_insns = 1; }
static void tb_start(DisasContextBase *dc, CPUState *cs) { }
static void insn_start(DisasContextBase *dc, CPUState *cs) { tcg_gen_insn_start(dc->pc_next / 2); }
static void translate_insn(DisasContextBase *dc, CPUState *cs) {
    /* Register code pages with QEMU; the helper fetches the live instruction. */
    translator_lduw(cpu_env(cs), dc, dc->pc_next);
    gen_helper_step(tcg_env);
    dc->pc_next += 2;
    dc->is_jmp = DISAS_TOO_MANY;
}
static void tb_stop(DisasContextBase *dc, CPUState *cs) { tcg_gen_exit_tb(NULL, 0); }
static const TranslatorOps ops = {
    .init_disas_context = init_context, .tb_start = tb_start,
    .insn_start = insn_start, .translate_insn = translate_insn, .tb_stop = tb_stop,
};
void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int *max_insns,
                           vaddr pc, void *host_pc) {
    DisasContext dc = {0};
    translator_loop(cs, tb, max_insns, pc, host_pc, &ops, &dc.base);
}
void xbc16_cpu_tcg_init(void) { }
