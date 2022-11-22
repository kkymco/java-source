/* $Id: sph_echo.h 216 2010-06-08 09:46:57Z tp $ */
/**
 * ECHO interface. ECHO is a family of functions which differ by
 * their output size; this implementation defines ECHO for output
 * sizes 224, 256, 384 and 512 bits.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @file     sph_echo.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_ECHO_H__
#define SPH_ECHO_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include "sph_types.h"

/**
 * Output size (in bits) for ECHO-224.
 */
#define SPH_SIZE_echo224   224

/**
 * Output size (in bits) for ECHO-256.
 */
#define SPH_SIZE_echo256   256

/**
 * Output size (in bits) for ECHO-384.
 */
#define SPH_SIZE_echo384   384

/**
 * Output size (in bits) for ECHO-512.
 */
#define SPH_SIZE_echo512   512

/**
 * This structure is a context for ECHO computations: it contains the
 * intermediate values and some data from the last entered block. Once
 * an ECHO computation has been performed, the context can be reused for
 * another computation. This specific structure is used for ECHO-224
 * and ECHO-256.
 *
 * The contents of this structure are private. A running ECHO computation
 * can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[192];    /* first field, for alignment */
	size_t ptr;
	union {
		sph_u32 Vs[4][4];
#if SPH_64
		sph_u64 Vb[4]