/**
 ****************************************************************************************
 *
 * @file ipc_compat.h
 *
 * Copyright (C) Amlogic 2011-2021
 *
 ****************************************************************************************
 */

#ifndef _IPC_H_
#define _IPC_H_

#define __INLINE static __attribute__((__always_inline__)) inline

#define __ALIGN4 __aligned(4)

#define ASSERT_ERR(condition)                                                           \
    do {                                                                                \
        if (unlikely(!(condition))) {                                                   \
            AML_ERR("ASSERT_ERR(" #condition ")\n");                                    \
        }                                                                               \
    } while(0)

#endif /* _IPC_H_ */
