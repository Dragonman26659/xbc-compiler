/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XBC16_CPU_PARAM_H
#define XBC16_CPU_PARAM_H
/* QEMU's TCG address type is 32-bit; architectural values remain 16-bit. */
#define TARGET_LONG_BITS 32
#define TARGET_PAGE_BITS 10
#define TARGET_PHYS_ADDR_SPACE_BITS 32
#define TARGET_VIRT_ADDR_SPACE_BITS 32
#define TCG_GUEST_DEFAULT_MO 0
#endif
