/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#ifndef __AR_KCOMPAT_H__
#define __AR_KCOMPAT_H__

#include <linux/version.h>
#include <linux/types.h>

/*
 * Detect whether the kernel expects GPR callback with:
 *   - <= 6.19 : struct gpr_resp_pkt * (non-const)
 *   - >= 7.0 : const struct gpr_resp_pkt * (const)
 *
 * Allow a build-time override (vendors may backport).
 *
 * Use:
 *   -D HAVE_GPR_CB_CONST   -> force >=7.0 behavior (const)
 *   -D HAVE_GPR_CB_MUTABLE -> force <=6.19 behavior (non-const)
 *
 * If neither is defined, pick based on LINUX_VERSION_CODE.
 */
#if defined(HAVE_GPR_CB_CONST) && defined(HAVE_GPR_CB_MUTABLE)
# error "Define at most one of HAVE_GPR_CB_CONST or HAVE_GPR_CB_MUTABLE"
#endif

#if defined(HAVE_GPR_CB_CONST)
# define AR_HAVE_GPR_CB_CONST 1
#elif defined(HAVE_GPR_CB_MUTABLE)
# define AR_HAVE_GPR_CB_CONST 0
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(7, 0, 0)
# define AR_HAVE_GPR_CB_CONST 1
#else
# define AR_HAVE_GPR_CB_CONST 0
#endif

/*
 * Thunk generator for GPR callbacks:
 * You implement a version-agnostic core:
 *   int name_core(const struct gpr_resp_pkt *data, void *priv, int op);
 *
 * We expose the kernel-expected symbol 'name' with the correct signature,
 * and inside it we call the core. On old kernels (non-const), we just pass
 * the pointer as const (no write needed). No casts at call sites.
 *
 * Note: If old-kernel callers legitimately modified *data (rare), you must
 * copy into a local mutable buffer. In our use, we do not modify *data.
 */
#define AR_GPR_CB_WRAPPER(name)                                             \
	static int name##_core(const struct gpr_resp_pkt *data,                  \
							void *priv, int op);                              \
	/* Kernel-facing thunk with correct signature */                         \
	/* v7.0+: const pointer */                                              \
	/* v6.19-: non-const pointer */                                          \
	/* Keep type-correct; avoid function-pointer casts. */                   \
	/* NOLINTBEGIN */                                                        \
	/* (lint tools may not understand version splits) */                     \
	/* new kernels */                                                        \
	/* ---------------------------------------------------------------- */   \
	/* NOLINTEND */                                                          \
	/* Define the symbol 'name' with kernel-expected signature */            \
	/* so it can be assigned into the ops struct directly. */                \
	/* ---------------------------------------------------------------- */   \
	__AR_GPR_CB_DECL(name)                                                  \
	{                                                                       \
		/* On old kernels, 'data' is non-const; we promise not to modify. */\
		return name##_core((const struct gpr_resp_pkt *)data, priv, op);    \
	}

/* Helper that emits the correct function prototype for 'name' */
#if AR_HAVE_GPR_CB_CONST
# define __AR_GPR_CB_DECL(name) \
	static int name(const struct gpr_resp_pkt *data, void *priv, int op)
#else
# define __AR_GPR_CB_DECL(name) \
	static int name(struct gpr_resp_pkt *data, void *priv, int op)
#endif

/*
 * Optional: document the upstream change for maintainers.
 * Example:
 *   Linux v7.0: GPR callback typedef now takes 'const struct gpr_resp_pkt *'
 *   (If you know the exact commit hash in your kernel tree, add it here.)
 */

#endif /* __AR_KCOMPAT_H__ */
