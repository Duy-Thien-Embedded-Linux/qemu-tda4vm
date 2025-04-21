#include "hw/arm/tda4vh_a72ss.h"
#include "exec/address-spaces.h"
#include "qemu/error-report.h"
#include "qapi/error.h"

static void tda4vh_a72ss_realize(DeviceState *dev, Error **errp)
{
    MachineState *ms = MACHINE(qdev_get_machine());
    A72SSState *s = A72SS(dev);
    MemoryRegion *system_memory = get_system_memory();
    uint64_t ram_size = ms->ram_size;

    /* Initialize and add RAM to system memory */
    memory_region_init_ram(&s->RAM, OBJECT(dev), "tda4vh.ram", ram_size, errp);
    if (*errp) {
        return;
    }
    memory_region_add_subregion(system_memory, TDA4VH_RAM_ADDR, &s->RAM);

    /* Initialize and add Flash to system memory */
    memory_region_init_rom(&s->Flash, OBJECT(dev), "tda4vh.flash", TDA4VH_FLASH_SIZE, errp);
    if (*errp) {
        return;
    }
    memory_region_add_subregion(system_memory, TDA4VH_FLASH_ADDR, &s->Flash);

    /* Initialize CPU */
    object_initialize_child(OBJECT(dev), "cpu", &s->cpu, ARM_CPU_TYPE_NAME("cortex-a72"));

    /* Set CPU properties */
    qdev_prop_set_bit(DEVICE(&s->cpu), "reset-hivecs", true);
    qdev_prop_set_bit(DEVICE(&s->cpu), "has_el3", s->secure);
    qdev_prop_set_bit(DEVICE(&s->cpu), "has_el2", s->virt);

    /* Realize CPU */
    qdev_realize(DEVICE(&s->cpu), NULL, errp);
    if (*errp) {
        return;
    }

    /* Set boot CPU pointer after CPU is realized */
    s->boot_cpu_ptr = ARM_CPU(&s->cpu);
}


static void tda4vh_a72ss_instance_init(Object *obj)
{
    /* Nothing to initialize here yet */
}

/* We're using object_class_property_add_bool instead of property arrays */

/* Property getters and setters */
static bool tda4vh_a72ss_get_secure(Object *obj, Error **errp)
{
    A72SSState *s = A72SS(obj);
    return s->secure;
}

static void tda4vh_a72ss_set_secure(Object *obj, bool value, Error **errp)
{
    A72SSState *s = A72SS(obj);
    s->secure = value;
}

static bool tda4vh_a72ss_get_virt(Object *obj, Error **errp)
{
    A72SSState *s = A72SS(obj);
    return s->virt;
}

static void tda4vh_a72ss_set_virt(Object *obj, bool value, Error **errp)
{
    A72SSState *s = A72SS(obj);
    s->virt = value;
}

static void tda4vh_a72ss_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    ObjectClass *oc_a72ss = OBJECT_CLASS(oc);

    dc->realize = tda4vh_a72ss_realize;
    /* Reason: Uses serial_hds in realize function, thus can't be used twice */
    dc->user_creatable = false;

    /* Add properties */
    object_class_property_add_bool(oc_a72ss, "secure",
                                  tda4vh_a72ss_get_secure, tda4vh_a72ss_set_secure);
    object_class_property_add_bool(oc_a72ss, "virtualization",
                                  tda4vh_a72ss_get_virt, tda4vh_a72ss_set_virt);
}

static const TypeInfo tda4vh_a72ss_info = {
    .name = TYPE_TDA4VH_A72SS,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(A72SSState),
    .class_init = tda4vh_a72ss_class_init,
    .instance_init = tda4vh_a72ss_instance_init,
};

static void tda4vh_a72ss_register_types(void)
{
    type_register_static(&tda4vh_a72ss_info);
}

type_init(tda4vh_a72ss_register_types)