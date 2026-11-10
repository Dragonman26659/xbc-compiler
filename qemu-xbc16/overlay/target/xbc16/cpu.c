/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "qemu/osdep.h"
#include "cpu.h"
#include "qapi/error.h"
#include "qemu/qemu-print.h"
#include "exec/exec-all.h"
#include "gdbstub/helpers.h"
#include "hw/core/sysemu-cpu-ops.h"
#include "hw/core/tcg-cpu-ops.h"
#include "hw/qdev-properties.h"
#include "disas/dis-asm.h"
#include "migration/vmstate.h"

static void set_pc(CPUState *cs, vaddr value) { cpu_env(cs)->core.pc = value / 2; }
static vaddr get_pc(CPUState *cs) { return cpu_env(cs)->core.pc * 2; }
static bool has_work(CPUState *cs) { return false; } /* HLT is permanent until reset. */
static int mmu_index(CPUState *cs, bool ifetch) { return 0; }
static void sync_tb(CPUState *cs, const TranslationBlock *tb) { cpu_env(cs)->core.pc = tb->pc / 2; }
static void restore(CPUState *cs, const TranslationBlock *tb, const uint64_t *data) { cpu_env(cs)->core.pc = data[0]; }
static hwaddr phys_debug(CPUState *cs, vaddr address) { return address; }
static bool tlb_fill(CPUState *cs, vaddr addr, int size, MMUAccessType access,
                     int index, bool probe, uintptr_t retaddr) {
    hwaddr physical=addr & (XBC16_WORDS*2-1);
    if (probe && addr >= XBC16_WORDS*2) return false;
    /* Out-of-range fetches map an alias only for translation; the helper
     * detects the architectural address and dispatches SegFault before fetch. */
    tlb_set_page(cs,addr & TARGET_PAGE_MASK,physical & TARGET_PAGE_MASK,
                 PAGE_READ | PAGE_WRITE | PAGE_EXEC,index,TARGET_PAGE_SIZE);
    return true;
}
static void reset_hold(Object *obj, ResetType type) {
    XBC16CPUClass *klass = XBC16_CPU_GET_CLASS(obj);
    if (klass->parent_phases.hold) klass->parent_phases.hold(obj, type);
    XBC16CPU *cpu=XBC16_CPU(obj);
    xbc16_reset(&cpu->env.core);
    cpu->mouse_buttons=0; cpu->mouse_dx=cpu->mouse_dy=0; cpu->mouse_changed=false;
}
static void realize(DeviceState *dev, Error **errp) {
    CPUState *cs = CPU(dev); XBC16CPUClass *klass = XBC16_CPU_GET_CLASS(dev);
    Error *local_error=NULL;
    cpu_exec_realizefn(cs,&local_error);
    if (local_error) { error_propagate(errp,local_error); return; }
    qemu_init_vcpu(cs); cpu_reset(cs); klass->parent_realize(dev, errp);
}
static ObjectClass *class_by_name(const char *name) {
    return object_class_by_name(!strcmp(name, "base") ? XBC16_CPU_TYPE_NAME : name);
}
static void dump(CPUState *cs, FILE *f, int flags) {
    XBC16State *s = &cpu_env(cs)->core;
    qemu_fprintf(f, "XBC16 PC(word)=%u PC(byte)=0x%08x ACC=0x%04x Z=%d N=%d U=%d IRQ=%d HLT=%d instructions=%" PRIu64 "\n",
        s->pc, s->pc*2, s->acc, s->z, s->n, s->unordered, s->irq_enable, s->halted, s->instructions);
    for (unsigned i=0;i<16;++i) qemu_fprintf(f, "R%u=0x%04x%s", i+1, s->r[i], i%4==3 ? "\n" : " ");
}
static int gdb_read(CPUState *cs, GByteArray *buf, int reg) {
    XBC16State *s = &cpu_env(cs)->core;
    if (reg >= 0 && reg < 16) return gdb_get_reg16(buf, s->r[reg]);
    if (reg == 16) return gdb_get_reg16(buf, s->acc);
    if (reg == 17) return gdb_get_reg32(buf, s->pc*2);
    if (reg == 18) return gdb_get_reg16(buf, s->z | (s->n<<1) | (s->unordered<<2) | (s->irq_enable<<3));
    return 0;
}
static int gdb_write(CPUState *cs, uint8_t *buf, int reg) {
    XBC16State *s = &cpu_env(cs)->core;
    if (reg >= 0 && reg < 16) { s->r[reg]=lduw_le_p(buf); return 2; }
    if (reg == 16) { s->acc=lduw_le_p(buf); return 2; }
    if (reg == 17) { s->pc=ldl_le_p(buf)/2; return 4; }
    if (reg == 18) { uint16_t v=lduw_le_p(buf); s->z=v&1; s->n=v&2; s->unordered=v&4; s->irq_enable=v&8; return 2; }
    return 0;
}
static int print_insn(bfd_vma addr, disassemble_info *info) {
    uint8_t data[2];
    int status=info->read_memory_func(addr,data,2,info);
    if (status) { info->memory_error_func(status,addr,info); return -1; }
    info->fprintf_func(info->stream,".word 0x%04x",lduw_le_p(data)); return 2;
}
static void disas(CPUState *cs, disassemble_info *info) { info->print_insn=print_insn; }
static const SysemuCPUOps system_ops = {.get_phys_page_debug=phys_debug};
static bool exec_interrupt(CPUState *cs, int request) { return false; }
static const TCGCPUOps tcg_ops = {
    .initialize=xbc16_cpu_tcg_init, .synchronize_from_tb=sync_tb,
    .restore_state_to_opc=restore, .tlb_fill=tlb_fill, .cpu_exec_halt=has_work, .cpu_exec_interrupt=exec_interrupt,
};
static Property properties[] = {
    DEFINE_PROP_BOOL("exit-on-halt", XBC16CPU, exit_on_halt, false),
    DEFINE_PROP_END_OF_LIST(),
};
static void class_init(ObjectClass *oc, void *data) {
    DeviceClass *dc=DEVICE_CLASS(oc); CPUClass *cc=CPU_CLASS(oc);
    XBC16CPUClass *klass=XBC16_CPU_CLASS(oc);
    device_class_set_parent_realize(dc,realize,&klass->parent_realize);
    resettable_class_set_parent_phases(RESETTABLE_CLASS(oc),NULL,reset_hold,NULL,&klass->parent_phases);
    device_class_set_props(dc,properties);
    cc->class_by_name=class_by_name; cc->has_work=has_work; cc->mmu_index=mmu_index;
    cc->set_pc=set_pc; cc->get_pc=get_pc; cc->dump_state=dump;
    cc->sysemu_ops=&system_ops; cc->tcg_ops=&tcg_ops; cc->disas_set_info=disas;
    cc->gdb_read_register=gdb_read; cc->gdb_write_register=gdb_write;
    cc->gdb_num_core_regs=19;
    /* Migration is explicitly blocked by the board; no silent partial snapshots. */
}
static const TypeInfo types[] = {
    {.name=TYPE_XBC16_CPU,.parent=TYPE_CPU,.instance_size=sizeof(XBC16CPU),
     .instance_align=__alignof(XBC16CPU),.class_size=sizeof(XBC16CPUClass),
     .class_init=class_init,.abstract=true},
    {.name=XBC16_CPU_TYPE_NAME,.parent=TYPE_XBC16_CPU},
};
DEFINE_TYPES(types)
