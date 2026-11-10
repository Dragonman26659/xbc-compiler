/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef XBC16_CPU_QOM_H
#define XBC16_CPU_QOM_H
#include "hw/core/cpu.h"
#define TYPE_XBC16_CPU "xbc16-cpu"
#define XBC16_CPU_TYPE_NAME "base-xbc16-cpu"
OBJECT_DECLARE_CPU_TYPE(XBC16CPU, XBC16CPUClass, XBC16_CPU)
#endif
