/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "exec/cpu_ldst.h"
#include "exec/address-spaces.h"
#include "exec/helper-proto.h"
#include "sysemu/runstate.h"
#include "qemu/error-report.h"
#include "qemu/main-loop.h"

static uint16_t read_word(void *opaque, uint32_t address) {
    return cpu_lduw_data(opaque, address * 2);
}
static void write_word(void *opaque, uint32_t address, uint16_t value) {
    /* Physical memory writes invalidate TBs, including self-modifying code. */
    address_space_stw_le(&address_space_memory, address * 2, value,
                        MEMTXATTRS_UNSPECIFIED, NULL);
}
void helper_step(CPUXBC16State *env) {
    CPUState *cs = env_cpu(env);
    XBC16CPU *cpu = XBC16_CPU(cs);
    XBC16Bus bus = {env, read_word, write_word};
    bql_lock();
    uint32_t queued=env->core.queue_count;
    xbc16_step(&env->core,&bus);
    if (cpu->keyboard_backend && env->core.queue_count<queued)
        qemu_chr_fe_accept_input(cpu->keyboard_backend);
    bql_unlock();
    if (env->core.fatal) {
        error_report("16BC fault at word PC %u: %s", env->core.pc, env->core.error);
        cpu_dump_state(cs, stderr, 0);
        cs->halted = true;
        qemu_system_shutdown_request_with_code(SHUTDOWN_CAUSE_GUEST_PANIC, 1);
        cs->exception_index = EXCP_HLT;
        cpu_loop_exit(cs);
    }
    if (env->core.halted) {
        cs->halted = true;
        if (cpu->exit_on_halt) {
            cpu_dump_state(cs, stdout, 0);
            qemu_system_shutdown_request(SHUTDOWN_CAUSE_GUEST_SHUTDOWN);
        }
        cs->exception_index = EXCP_HLT;
        cpu_loop_exit(cs);
    }
}
