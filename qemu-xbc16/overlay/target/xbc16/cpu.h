/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XBC16_CPU_H
#define XBC16_CPU_H
#include "cpu-qom.h"
#include "exec/cpu-defs.h"
#include "core.h"
#include "chardev/char-fe.h"
#define CPU_RESOLVING_TYPE TYPE_XBC16_CPU
#define MMU_MODES 1
struct CPUArchState { XBC16State core; };
typedef struct CPUArchState CPUXBC16State;
struct ArchCPU {
    CPUState parent_obj;
    CPUXBC16State env;
    bool exit_on_halt;
    CharBackend *keyboard_backend;
    uint16_t mouse_buttons;
    int32_t mouse_dx, mouse_dy;
    bool mouse_changed;
};
struct XBC16CPUClass {
    CPUClass parent_class;
    DeviceRealize parent_realize;
    ResettablePhases parent_phases;
};
static inline void cpu_get_tb_cpu_state(CPUXBC16State *env, vaddr *pc,
                                        uint64_t *cs_base, uint32_t *flags) {
    *pc = (vaddr)env->core.pc * 2; *cs_base = 0; *flags = 0;
}
void xbc16_cpu_tcg_init(void);
extern const VMStateDescription vmstate_xbc16_cpu;
#include "exec/cpu-all.h"
#endif
