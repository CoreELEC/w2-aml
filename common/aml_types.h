/*
 * Copyright (c) 2025 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 */

#ifndef AML_TYPES_H_
#define AML_TYPES_H_

#ifdef __linux__
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/bug.h>          /* for BUILD_BUG_ON_ZERO() */
#else
#include <stdint.h>
#endif

typedef uint32_t addr32_t;

#ifndef __has_attribute
#define __has_attribute(x)      0
#endif

/* earlier version of linux/compiler_attributes.h may not define the following */
#ifndef fallthrough
#if __has_attribute(__fallthrough__)
# define fallthrough            __attribute__((__fallthrough__))
#else
# define fallthrough            do {} while (0)  /* fallthrough */
#endif
#endif

#ifndef __maybe_unused
#define __maybe_unused          __attribute__((__unused__))
#endif

#ifndef __linux__   /* for firmware */
#ifndef __packed
#define __packed                __attribute__((__packed__))
#endif

#ifndef BUILD_BUG_ON_ZERO
#define BUILD_BUG_ON_ZERO(e)    ((int)(sizeof(struct { int:(-!!(e)); })))
#endif

#ifndef ALIGN
#define ALIGN(x,a)              (((x)+(a)-1)&~((a)-1))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d)      (((n) + (d) - 1) / (d))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)           (sizeof(a) / sizeof((a)[0]))
#endif
#endif

#endif /* AML_TYPES_H_ */
