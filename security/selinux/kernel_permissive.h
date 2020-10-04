/** License GPL-v2
    Copyright 2020 by Pal Zoltan Illes
*/

static bool kernel_permissive = false;
void set_kernel_permissive(bool on) {
        kernel_permissive = on;
}
EXPORT_SYMBOL(set_kernel_permissive);

// source class
#define KERNEL_SOURCE "u:r:kernel:s0"

// target class list
static char targets[13][255] = {
                "u:object_r:toolbox_exec:s0",
                "u:object_r:shell_exec:s0",
                "u:r:kernel:s0",
                "u:object_r:fuse:s0",
                "u:object_r:shell_data_file:s0",
                "u:object_r:property_data_file:s0",
                "u:object_r:property_socket:s0",
                "u:r:init:s0",
                "u:object_r:exported2_default_prop:s0",
                "u:object_r:vendor_radio_prop:s0",
                "u:object_r:default_prop:s0",
                "u:object_r:system_file:s0",
                "u:object_r:device:s0",
        };

