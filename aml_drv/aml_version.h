#ifndef _AML_VERSION_H_
#define _AML_VERSION_H_

#include "aml_version_gen.h"

static inline void aml_print_version(void)
{
    printk(AML_VERS_BANNER"\n");
    //printk(AML_DRIVER_COMPILE_INFO"\n");
    //printk(FIRMWARE_INFO"\n");
    //printk(COMMON_INFO"\n");
}

#endif /* _AML_VERSION_H_ */
