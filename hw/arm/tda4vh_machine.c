#include "hw/arm/tda4vh_a72ss.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "hw/arm/boot.h"
#include "qapi/error.h"

struct TDA4VHMachineState {
    MachineState parent_obj;

    A72SSState a72ss;
    bool secure;
    bool virt;
    struct arm_boot_info binfo;
};

#define TYPE_TDA4VH_MACHINE "tda4vh-machine"
OBJECT_DECLARE_SIMPLE_TYPE(TDA4VHMachineState, TDA4VH_MACHINE)


static bool tda4vh_get_secure(Object *obj, Error **errp)
{
    TDA4VHMachineState *s = TDA4VH_MACHINE(obj);

    return s->secure;
}

static void tda4vh_set_secure(Object *obj, bool value, Error **errp)
{
    TDA4VHMachineState *s = TDA4VH_MACHINE(obj);

    s->secure = value;
}

static bool tda4vh_get_virt(Object *obj, Error **errp)
{
    TDA4VHMachineState *s = TDA4VH_MACHINE(obj);

    return s->virt;
}

static void tda4vh_set_virt(Object *obj, bool value, Error **errp)
{
    TDA4VHMachineState *s = TDA4VH_MACHINE(obj);

    s->virt = value;
}

static void tda4vh_machine_init(MachineState *machine)
{
    TDA4VHMachineState *s = TDA4VH_MACHINE(machine);
    uint64_t ram_size = machine->ram_size;

    /* Create the memory region to pass to the SoC */
    if (ram_size > TDA4VH_MAX_RAM_SIZE) {
        error_report("ERROR: RAM size 0x%" PRIx64 " above max supported of "
                     "0x%" PRIx64, ram_size,
                     (uint64_t)TDA4VH_MAX_RAM_SIZE);
        exit(1);
    }

    if (ram_size < 0x08000000) {
        qemu_log("WARNING: RAM size 0x%" PRIx64 " is small for TDA4VH",
                 ram_size);
    }

    /* Initialize the A72 subsystem */
    object_initialize_child(OBJECT(machine), "a72ss", &s->a72ss, TYPE_TDA4VH_A72SS);

    /* Set properties */
    object_property_set_bool(OBJECT(&s->a72ss), "secure", s->secure,
                             &error_fatal);
    object_property_set_bool(OBJECT(&s->a72ss), "virtualization", s->virt,
                             &error_fatal);

    /* Realize the device */
    qdev_realize(DEVICE(&s->a72ss), NULL, &error_fatal);

    /* Set up boot info */
    s->binfo.ram_size = ram_size;
    s->binfo.loader_start = TDA4VH_RAM_ADDR;
    s->binfo.psci_conduit = 2; /* QEMU_PSCI_CONDUIT_HVC */

    /* Load kernel if specified */
    arm_load_kernel(s->a72ss.boot_cpu_ptr, machine, &s->binfo);
}

static void tda4vh_machine_instance_init(Object *obj)
{
    TDA4VHMachineState *s = TDA4VH_MACHINE(obj);
    /* Default to secure mode being disabled */
    s->secure = false;
    /* Default to virt (EL2) being disabled */
    s->virt = false;
}

static void tda4vh_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    ObjectClass *oc_machine = OBJECT_CLASS(oc);

    mc->desc = "TDA4VH Machine";
    mc->init = tda4vh_machine_init;
    mc->block_default_type = IF_IDE;
    mc->units_per_default_bus = 1;
    mc->ignore_memory_transaction_failures = true;
    mc->max_cpus = 8;
    mc->default_cpus = 1;
    mc->auto_create_sdcard = false;
    mc->default_ram_size = TDA4VH_RAM_SIZE;

    /* Add properties */
    object_class_property_add_bool(oc_machine, "secure",
                                  tda4vh_get_secure, tda4vh_set_secure);
    object_class_property_add_bool(oc_machine, "virtualization",
                                  tda4vh_get_virt, tda4vh_set_virt);
}

static const TypeInfo tda4vh_machine_info = {
    .name = TYPE_TDA4VH_MACHINE,
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(TDA4VHMachineState),
    .class_init = tda4vh_machine_class_init,
    .instance_init = tda4vh_machine_instance_init,
};

static void tda4vh_machine_register_types(void)
{
    type_register_static(&tda4vh_machine_info);
}

type_init(tda4vh_machine_register_types)