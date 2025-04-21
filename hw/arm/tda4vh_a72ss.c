#include "hw/arm/tda4vh_a72ss.h"

static void tda4vh_a72ss_realize(DeviceState *dev, Error **errp)
{
    MachineState *ms = MACHINE(qdev_get_machine());
    A72SSState *s = A72SS(dev);
}

static Property xlnx_zynqmp_props[] = {
    DEFINE_PROP_STRING("boot-cpu", A72SSState, boot_cpu),
    DEFINE_PROP_BOOL("secure", A72SSState, secure, true),
    DEFINE_PROP_BOOL("virt", A72SSState, virt, true),
};

static void tda4vh_a72ss_instance_init(Object *obj)
{
    MachineState *ms = MACHINE(qdev_get_machine());
    A72SSState *s = A72SS(obj);
}

static void tda4vh_a72ss_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    device_class_set_props(dc, xlnx_zynqmp_props);
    dc->realize = tda4vh_a72ss_realize;
    /* Reason: Uses serial_hds in realize function, thus can't be used twice */
    dc->user_creatable = false;
}

static const TypeInfo tda4vh_a72ss_info = {
    .name = TYPE_TDA4VH_A72SS,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(A72SSState),
    .class_init = tda4vh_a72ss_class_init,
    .instance_init = tda4vh_a72ss_instance_init,
}

static void tda4vh_a72ss_register_types(void)
{
    type_register_static(&tda4vh_a72ss_info);
}

type_init(tda4vh_a72ss_register_types)