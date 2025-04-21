#ifndef HW_ARM_TDA4VH_A72SS_H
#define HW_ARM_TDA4VH_A72SS_H

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "hw/boards.h"
#include "system/system.h"
#include "target/arm/cpu-qom.h"
#include "hw/misc/unimp.h"
#include "hw/char/cadence_uart.h"
#include "qom/object.h"


#define TYPE_TDA4VH_A72SS "tda4vh-a72ss"
OBJECT_DECLARE_SIMPLE_TYPE(A72SSState, A72SS)


typedef struct A72SSState {
    /*< private >*/
    DeviceState parent_obj;
    
    /*< public >*/
    ARMCPU *cpu;
    MemoryRegion RAM;
    MemoryRegion Flash;
    char *boot_cpu;
    ARMCPU *boot_cpu_ptr;

    /* Has the ARM Security extensions?  */
    bool secure;
    /* Has the ARM Virtualization extensions?  */
    bool virt;
        
}A72SSState;

#endif /* HW_ARM_TDA4VH_A72SS_H */
