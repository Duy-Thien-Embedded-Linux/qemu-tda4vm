/*
 * Texas Instruments J784S4 A72 Subsystem
 *
 * This file models the Cortex-A72 subsystem of the J784S4 SoC,
 * including the 8 Cortex-A72 cores arranged in two clusters,
 * their cache hierarchy, and related components.
 *
 * Copyright (c) 2025 QEMU Contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-core.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/sysbus.h"
#include "hw/arm/boot.h"
#include "exec/address-spaces.h"
#include "hw/core/sysbus-fdt.h"
#include "hw/irq.h"
#include "target/arm/cpu.h"
#include "target/arm/cpu-qom.h"
#include "hw/sysbus.h"
#include "hw/cpu/cluster.h"
#include "hw/intc/arm_gicv3_common.h"
#include "hw/intc/arm_gicv3_its_common.h"

#define A72SS_MAX_CPUS 8
#define A72SS_NUM_CLUSTERS 2
#define A72SS_CPUS_PER_CLUSTER 4
#define MY_PPI_IRQ(cpu, irq) (16 + irq + (cpu * 32))

typedef struct A72SSState A72SSState;
/* A72SS private instance structure */
struct A72SSState {
    /*< private >*/
    SysBusDevice parent_obj;
    
    /*< public >*/
    struct {
        ARMCPU *cpu[A72SS_MAX_CPUS];
    } cores;
    
    /* GIC */
    DeviceState *gic;
    
    /* Memory regions */
    MemoryRegion msmc_ram;    /* MSMC shared RAM */
    MemoryRegion l2_0;        /* L2 cache for cluster 0 */
    MemoryRegion l2_1;        /* L2 cache for cluster 1 */
    
    /* Properties */
    uint32_t num_cores;       /* Number of cores to instantiate */
    uint64_t msmc_size;       /* Size of MSMC RAM */
    uint32_t cluster_0_size;  /* Size of cluster 0 L2 cache */
    uint32_t cluster_1_size;  /* Size of cluster 1 L2 cache */
    
    /* PSCI configuration */
    uint32_t psci_conduit;
};


/* Define constants for addressing */
#define TI_J784S4_MSMC_ADDR   0x70000000
#define TI_J784S4_MSMC_SIZE   0x00400000  /* 4MB */
#define TI_J784S4_GIC_ADDR    0x01800000
#define TI_J784S4_GIC_SIZE    0x00200000  /* 2MB */

#define TYPE_A72SS "ti.j784s4.a72ss"
#define A72SS(obj) OBJECT_CHECK(A72SSState, (obj), TYPE_A72SS)

static Property a72ss_properties[] = {
    DEFINE_PROP_UINT32("num-cores", A72SSState, num_cores, A72SS_MAX_CPUS),
    DEFINE_PROP_UINT64("msmc-size", A72SSState, msmc_size, TI_J784S4_MSMC_SIZE),
    DEFINE_PROP_UINT32("cluster-0-l2-size", A72SSState, cluster_0_size, 0x200000), /* 2MB */
    DEFINE_PROP_UINT32("cluster-1-l2-size", A72SSState, cluster_1_size, 0x200000), /* 2MB */
    /* PSCI conduit: 0 for HVC, 1 for SMC */
    DEFINE_PROP_UINT32("psci-conduit", A72SSState, psci_conduit, 1), /* Default to SMC */
    // DEFINE_PROP_END_OF_LIST(),
};

static void a72ss_init_cpus(A72SSState *s, Error **errp)
{
    int i;
    Error *err = NULL;
    
    /* Create the CPUs according to J784S4 spec */
    for (i = 0; i < s->num_cores; i++) {
        char *name = g_strdup_printf("cpu%d", i);
        int cluster_id = i / A72SS_CPUS_PER_CLUSTER;
        int core_id_in_cluster = i % A72SS_CPUS_PER_CLUSTER;
        
        /* Create CPU object */
        Object *cpuobj = object_new(ARM_CPU_TYPE_NAME("cortex-a72"));
        
        /* Set CPU properties according to the DT */
        qdev_prop_set_uint32(DEVICE(cpuobj), "mp-affinity", 
                          (cluster_id << 8) | core_id_in_cluster);
        qdev_prop_set_uint8(DEVICE(cpuobj), "core-count", s->num_cores);
        
        /* CPU cache properties matching the J784S4 DT */
        object_property_set_int(cpuobj, "dcache-size", 0x8000, &err); /* 32KB */
        object_property_set_int(cpuobj, "icache-size", 0xc000, &err); /* 48KB */
        object_property_set_int(cpuobj, "dcache-line-size", 64, &err);
        object_property_set_int(cpuobj, "icache-line-size", 64, &err);
        object_property_set_int(cpuobj, "dcache-sets", 256, &err);
        object_property_set_int(cpuobj, "icache-sets", 256, &err);
        
        /* Set PSCI boot method */
        if (s->psci_conduit == 0) {
            qdev_prop_set_uint32(DEVICE(cpuobj), "psci-conduit", 2); /* HVC */
        } else {
            qdev_prop_set_uint32(DEVICE(cpuobj), "psci-conduit", 1); /* SMC */
        }
        
        /* Realize the CPU */
        object_property_set_bool(cpuobj, "realized", true, &err);
        if (err) {
            error_propagate(errp, err);
            object_unref(cpuobj);
            return;
        }
        
        /* Store the CPU in our object */
        s->cores.cpu[i] = ARM_CPU(cpuobj);
        
        g_free(name);
    }
}

static void a72ss_init_gic(A72SSState *s, Error **errp)
{
    Error *err = NULL;
    SysBusDevice *gicbusdev;
    int i;
    
    /* Create the GIC */
    s->gic = qdev_new(gicv3_class_name());
    qdev_prop_set_uint32(s->gic, "revision", 3); /* GICv3 */
    qdev_prop_set_uint32(s->gic, "num-cpu", s->num_cores);
    qdev_prop_set_uint32(s->gic, "num-irq", 288); /* Typical for J784S4 */
    
    /* Realize the GIC */
    object_property_set_bool(OBJECT(s->gic), "realized", true, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    
    /* Map the GIC */
    gicbusdev = SYS_BUS_DEVICE(s->gic);
    sysbus_mmio_map(gicbusdev, 0, TI_J784S4_GIC_ADDR);
    
    /* Connect CPUs to the GIC */
    for (i = 0; i < s->num_cores; i++) {
        qdev_connect_gpio_out_named(DEVICE(s->cores.cpu[i]), "pmu-interrupt", 0,
                                  qdev_get_gpio_in(s->gic, 
                                    MY_PPI_IRQ(i, 7))); /* PMU IRQ = PPI 7 */
        
        sysbus_connect_irq(gicbusdev, i, qdev_get_gpio_in(
                         DEVICE(s->cores.cpu[i]), ARM_CPU_IRQ));
        sysbus_connect_irq(gicbusdev, i + s->num_cores, qdev_get_gpio_in(
                         DEVICE(s->cores.cpu[i]), ARM_CPU_FIQ));
        sysbus_connect_irq(gicbusdev, i + 2 * s->num_cores, qdev_get_gpio_in(
                         DEVICE(s->cores.cpu[i]), ARM_CPU_VIRQ));
        sysbus_connect_irq(gicbusdev, i + 3 * s->num_cores, qdev_get_gpio_in(
                         DEVICE(s->cores.cpu[i]), ARM_CPU_VFIQ));
    }
}

static void a72ss_init_memory(A72SSState *s, Error **errp)
{
    /* Initialize MSMC RAM */
    memory_region_init_ram(&s->msmc_ram, OBJECT(s), "ti-j784s4.msmc_ram",
                          s->msmc_size, &error_fatal);
    memory_region_add_subregion(get_system_memory(), TI_J784S4_MSMC_ADDR, 
                              &s->msmc_ram);
    
    /* Initialize L2 caches as RAM regions for now */
    memory_region_init_ram(&s->l2_0, OBJECT(s), "ti-j784s4.l2_0",
                          s->cluster_0_size, &error_fatal);
    memory_region_init_ram(&s->l2_1, OBJECT(s), "ti-j784s4.l2_1",
                          s->cluster_1_size, &error_fatal);
    
    /* Note: In a real system, L2 caches wouldn't be directly mapped.
     * This is a simple approximation for simulation purposes.
     */
}

static void a72ss_realize(DeviceState *dev, Error **errp)
{
    A72SSState *s = A72SS(dev);
    Error *err = NULL;
    
    if (s->num_cores > A72SS_MAX_CPUS) {
        error_setg(errp, "%s: Number of cores (%d) exceeds maximum (%d)",
                 __func__, s->num_cores, A72SS_MAX_CPUS);
        return;
    }
    
    /* Initialize CPUs */
    a72ss_init_cpus(s, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    
    /* Initialize memory regions */
    a72ss_init_memory(s, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    
    /* Initialize GIC */
    a72ss_init_gic(s, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    
    qemu_log_mask(LOG_UNIMP, "J784S4 A72 Subsystem: Some features may not be implemented\n");
}

static void a72ss_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    
    dc->realize = a72ss_realize;
    dc->desc = "TI J784S4 A72 Subsystem";
    dc->props_ = a72ss_properties;
}

static const TypeInfo a72ss_info = {
    .name = TYPE_A72SS,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(A72SSState),
    .class_init = a72ss_class_init,
};

static void a72ss_register_types(void)
{
    type_register_static(&a72ss_info);
}

type_init(a72ss_register_types)