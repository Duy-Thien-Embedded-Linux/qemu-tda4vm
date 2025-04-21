#ifndef HW_ARM_TDA4VH_A72SS_H
#define HW_ARM_TDA4VH_A72SS_H

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "hw/boards.h"
#include "system/system.h"
#include "target/arm/cpu.h"
#include "target/arm/cpu-qom.h"
#include "hw/misc/unimp.h"
#include "hw/char/cadence_uart.h"
#include "qom/object.h"

/* Memory map definitions */
#define TDA4VH_RAM_ADDR       0x80000000
#define TDA4VH_RAM_SIZE       0x40000000 /* 1GB */
#define TDA4VH_FLASH_ADDR     0x00000000
#define TDA4VH_FLASH_SIZE     0x04000000 /* 64MB */

/* Maximum RAM size */
#define TDA4VH_MAX_RAM_SIZE   0x80000000 /* 2GB */


#define TYPE_TDA4VH_A72SS "tda4vh-a72ss"
#define TYPE_A72SS TYPE_TDA4VH_A72SS
OBJECT_DECLARE_SIMPLE_TYPE(A72SSState, A72SS)


typedef struct A72SSState {
    /*< private >*/
    DeviceState parent_obj;

    /*< public >*/
    ARMCPU cpu;
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
