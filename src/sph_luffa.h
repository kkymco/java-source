/* $Id: sph_luffa.h 154 2010-04-26 17:00:24Z tp $ */
/**
 * Luffa interface. Luffa is a family of functions which differ by
 * their output size; this implementation defines Luffa for output
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
 * @file     sph_luffa.h
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#ifndef SPH_LUFFA_H__
#define SPH_LUFFA_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include "sph_types.h"

/**
 * Output size (in bits) for Luffa-224.
 */
#define SPH_SIZE_luffa224   224

/**
 * Output size (in bits) for Luffa-256.
 */
#define SPH_SIZE_luffa256   256

/**
 * Output size (in bits) for Luffa-384.
 */
#define SPH_SIZE_luffa384   384

/**
 * Output size (in bits) for Luffa-512.
 */
#define SPH_SIZE_luffa512   512

/**
 * This structure is a context for Luffa-224 computations: it contains
 * the intermediate values and some data from the last entered block.
 * Once a Luffa computation has been performed, the context can be
 * reused for another computation.
 *
 * The contents of this structure are private. A running Luffa
 * computation can be cloned by copying the context (e.g. with a simple
 * <code>memcpy()</code>).
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[32];    /* first field, for alignment */
	size_t ptr;
	sph_u32 V[3][8];
#endif
} sph_luffa224_context;

/**
 * This structure is a context for Luffa-256 computations. It is
 * identical to <code>sph_luffa224_context</code>.
 */
typedef sph_luffa224_context sph_luffa256_context;

/**
 * This structure is a context for Luffa-384 computations.
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[32];    /* first field, for alignment */
	size_t ptr;
	sph_u32 V[4][8];
#endif
} sph_luffa384_context;

/**
 * This structure is a context for Luffa-512 computations.
 */
typedef struct {
#ifndef DOXYGEN_IGNORE
	unsigned char buf[32];    /* first field, for alignment */
	size_t ptr;
	sph_u32 V[5][8];
#endif
} sph_luffa512_context;

/**
 * Initialize a Luffa-224 context. This process performs no memory allocation.
 *
 * @param cc   the Luffa-224 context (pointer to a
 *             <code>sph_luffa224_context</code>)
 */
void sph_luffa224_init(void *cc);

/**
 * Process some data bytes. It is acceptable that <code>len</code> is zero
 * (in which case this function does nothing).
 *
 * @param cc     the Luffa-224 context
 * @param data   the input data
 * @param len    the input 