/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/units.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/qdev-properties.h"
#include "exec/address-spaces.h"
#include "chardev/char-fe.h"
#include "sysemu/sysemu.h"
#include "migration/blocker.h"
#include "cpu.h"
#include "ui/input.h"
#include "ui/console.h"
#include "ui/vgafont.h"
#include "text.h"

#define TYPE_XBC16_MACHINE MACHINE_TYPE_NAME("xbc16")
OBJECT_DECLARE_SIMPLE_TYPE(XBC16MachineState, XBC16_MACHINE)
struct XBC16MachineState {
    MachineState parent_obj;
    MemoryRegion ram;
    XBC16CPU *cpu;
    CharBackend keyboard;
    QemuConsole *console;
    uint16_t text_cache[XBC16_TEXT_CELLS];
    bool redraw;
    bool exit_on_halt;
    Error *migration_blocker;
};
static void text_invalidate(void *opaque)
{
    XBC16MachineState *s = opaque;
    s->redraw = true;
}
static void text_update(void *opaque)
{
    XBC16MachineState *s = opaque;
    DisplaySurface *surface = qemu_console_surface(s->console);
    const uint8_t *ram = memory_region_get_ram_ptr(&s->ram);
    bool changed = false;
    for (unsigned cell = 0; cell < XBC16_TEXT_CELLS; cell++) {
        uint16_t word = lduw_le_p(ram + (XBC16_TEXT_BASE + cell) * 2);
        if (s->redraw || s->text_cache[cell] != word) {
            s->text_cache[cell] = word;
            xbc16_text_cell(surface_data(surface), surface_stride(surface),
                            cell, word, vgafont16);
            changed = true;
        }
    }
    s->redraw = false;
    if (changed) {
        dpy_gfx_update(s->console, 0, 0, XBC16_TEXT_WIDTH, XBC16_TEXT_HEIGHT);
    }
}
static const GraphicHwOps text_ops = {
    .invalidate = text_invalidate,
    .gfx_update = text_update,
};
static int can_read(void *opaque) {
    XBC16MachineState *s=opaque;
    return XBC16_QUEUE-s->cpu->env.core.queue_count;
}
static void receive(void *opaque, const uint8_t *buf, int size) {
    XBC16MachineState *s=opaque;
    for (int i=0;i<size;++i) {
        if (!xbc16_queue(&s->cpu->env.core,0,buf[i],0,0)) {
            error_report("16BC keyboard event queue full"); break;
        }
    }
}
static void input_event(DeviceState *dev, QemuConsole *src, InputEvent *event) {
    XBC16CPU *cpu=XBC16_CPU(dev);
    switch (event->type) {
    case INPUT_EVENT_KIND_KEY: {
        InputKeyEvent *key=event->u.key.data;
        unsigned code=qemu_input_key_value_to_number(key->key);
        if (!key->down) code |= 0x8000;
        if (!xbc16_queue(&cpu->env.core,0,code,0,0)) error_report("16BC input queue full");
        break;
    }
    case INPUT_EVENT_KIND_BTN: {
        InputBtnEvent *button=event->u.btn.data;
        unsigned mask=0;
        if (button->button==INPUT_BUTTON_LEFT) mask=1;
        if (button->button==INPUT_BUTTON_RIGHT) mask=2;
        if (button->button==INPUT_BUTTON_MIDDLE) mask=4;
        if (button->down) cpu->mouse_buttons |= mask; else cpu->mouse_buttons &= ~mask;
        cpu->mouse_changed |= mask != 0;
        break;
    }
    case INPUT_EVENT_KIND_REL: {
        InputMoveEvent *move=event->u.rel.data;
        int32_t *delta=move->axis==INPUT_AXIS_X ? &cpu->mouse_dx : &cpu->mouse_dy;
        int64_t movement=CLAMP(move->value,-32768,32767);
        *delta=CLAMP((int64_t)*delta+movement,-32768,32767);
        cpu->mouse_changed=true;
        break;
    }
    default: break;
    }
}
static void input_sync(DeviceState *dev) {
    XBC16CPU *cpu=XBC16_CPU(dev);
    if (cpu->mouse_changed) {
        if (!xbc16_queue(&cpu->env.core,1,cpu->mouse_buttons,cpu->mouse_dx,cpu->mouse_dy))
            error_report("16BC mouse event queue full");
        cpu->mouse_dx=cpu->mouse_dy=0; cpu->mouse_changed=false;
    }
}
static const QemuInputHandler input_handler = {
    .name="16BC keyboard/mouse", .mask=INPUT_EVENT_MASK_KEY | INPUT_EVENT_MASK_BTN | INPUT_EVENT_MASK_REL,
    .event=input_event, .sync=input_sync,
};
static void machine_init(MachineState *machine) {
    XBC16MachineState *s=XBC16_MACHINE(machine);
    g_autofree char *image=NULL;
    gsize size=0;
    GError *error=NULL;
    if (!machine->kernel_filename) {
        error_report("16BC requires -kernel program.bin (raw little-endian words)"); exit(1);
    }
    if (!g_file_get_contents(machine->kernel_filename,&image,&size,&error)) {
        error_report("Cannot read 16BC binary: %s",error->message); g_error_free(error); exit(1);
    }
    if (!size || (size&1) || size>XBC16_WORDS*2) {
        error_report("16BC binary must contain 1..65536 complete little-endian words"); exit(1);
    }
    memory_region_init_ram(&s->ram,NULL,"xbc16.ram",XBC16_WORDS*2,&error_fatal);
    memory_region_add_subregion(get_system_memory(),0,&s->ram);
    rom_add_blob_fixed("xbc16.boot",image,size,0);
    s->cpu=XBC16_CPU(object_new(machine->cpu_type));
    object_property_set_bool(OBJECT(s->cpu),"exit-on-halt",s->exit_on_halt,&error_fatal);
    qdev_realize(DEVICE(s->cpu),NULL,&error_fatal);
    if (serial_hd(0)) {
        qemu_chr_fe_init(&s->keyboard,serial_hd(0),&error_fatal);
        s->cpu->keyboard_backend=&s->keyboard;
        qemu_chr_fe_set_handlers(&s->keyboard,can_read,receive,NULL,NULL,s,NULL,true);
    }
    s->console = graphic_console_init(DEVICE(s->cpu), 0, &text_ops, s);
    qemu_console_resize(s->console, XBC16_TEXT_WIDTH, XBC16_TEXT_HEIGHT);
    s->redraw = true;
    qemu_input_handler_register(DEVICE(s->cpu),&input_handler);
    error_setg(&s->migration_blocker,"16BC CPU state migration is not implemented");
    migrate_add_blocker(&s->migration_blocker,&error_fatal);
}
static bool get_exit(Object *obj, Error **errp) { return XBC16_MACHINE(obj)->exit_on_halt; }
static void set_exit(Object *obj, bool value, Error **errp) { XBC16_MACHINE(obj)->exit_on_halt=value; }
static void instance_init(Object *obj) {
    object_property_add_bool(obj,"exit-on-halt",get_exit,set_exit);
}
static void machine_class_init(ObjectClass *oc, void *data) {
    MachineClass *mc=MACHINE_CLASS(oc);
    mc->desc="16BC word-addressed computer (raw binary boot)";
    mc->init=machine_init; mc->default_cpu_type=XBC16_CPU_TYPE_NAME;
    mc->default_cpus=mc->min_cpus=mc->max_cpus=1;
    mc->default_ram_size=XBC16_WORDS*2;
    mc->no_floppy=mc->no_cdrom=mc->no_parallel=true;
    mc->is_default=true;
}
static const TypeInfo type = {
    .name=TYPE_XBC16_MACHINE,.parent=TYPE_MACHINE,
    .instance_size=sizeof(XBC16MachineState),.instance_init=instance_init,
    .class_init=machine_class_init,
};
static void register_types(void) { type_register_static(&type); }
type_init(register_types)
