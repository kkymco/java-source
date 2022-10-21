/* $Id: skein.c 254 2011-06-07 19:38:58Z tp $ */
/*
 * Skein implementation.
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
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#include <stddef.h>
#include <string.h>

#include "sph_skein.h"

#ifdef __cplusplus
extern "C"{
#endif


#if SPH_SMALL_FOOTPRINT && !defined SPH_SMALL_FOOTPRINT_SKEIN
#define SPH_SMALL_FOOTPRINT_SKEIN   1
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4146)
#endif

#if SPH_64

#if 0
/* obsolete */
/*
 * M5_ ## s ## _ ## i  evaluates to s+i mod 5 (0 <= s <= 18, 0 <= i <= 3).
 */

#define M5_0_0    0
#define M5_0_1    1
#define M5_0_2    2
#define M5_0_3    3

#define M5_1_0    1
#define M5_1_1    2
#define M5_1_2    3
#define M5_1_3    4

#define M5_2_0    2
#define M5_2_1    3
#define M5_2_2    4
#define M5_2_3    0

#define M5_3_0    3
#define M5_3_1    4
#define M5_3_2    0
#define M5_3_3    1

#define M5_4_0    4
#define M5_4_1    0
#define M5_4_2    1
#define M5_4_3    2

#define M5_5_0    0
#define M5_5_1    1
#define M5_5_2    2
#define M5_5_3    3

#define M5_6_0    1
#define M5_6_1    2
#define M5_6_2    3
#define M5_6_3    4

#define M5_7_0    2
#define M5_7_1    3
#define M5_7_2    4
#define M5_7_3    0

#define M5_8_0    3
#define M5_8_1    4
#define M5_8_2    0
#define M5_8_3    1

#define M5_9_0    4
#define M5_9_1    0
#define M5_9_2    1
#define M5_9_3    2

#define M5_10_0   0
#define M5_10_1   1
#define M5_10_2   2
#define M5_10_3   3

#define M5_11_0   1
#define M5_11_1   2
#define M5_11_2   3
#define M5_11_3   4

#define M5_12_0   2
#define M5_12_1   3
#define M5_12_2   4
#define M5_12_3   0

#define M5_13_0   3
#define M5_13_1   4
#define M5_13_2   0
#define M5_13_3   1

#define M5_14_0   4
#define M5_14_1   0
#define M5_14_2   1
#define M5_14_3   2

#define M5_15_0   0
#define M5_15_1   1
#define M5_15_2   2
#define M5_15_3   3

#define M5_16_0   1
#define M5_16_1   2
#define M5_16_2   3
#define M5_16_3   4

#define M5_17_0   2
#define M5_17_1   3
#define M5_17_2   4
#define M5_17_3   0

#define M5_18_0   3
#define M5_18_1   4
#define M5_18_2   0
#define M5_18_3   1
#endif

/*
 * M9_ ## s ## _ ## i  evaluates to s+i mod 9 (0 <= s <= 18, 0 <= i <= 7).
 */

#define M9_0_0    0
#define M9_0_1    1
#define M9_0_2    2
#define M9_0_3    3
#define M9_0_4    4
#define M9_0_5    5
#define M9_0_6    6
#define M9_0_7    7

#define M9_1_0    1
#define M9_1_1    2
#define M9_1_2    3
#define M9_1_3    4
#define M9_1_4    5
#define M9_1_5    6
#define M9_1_6    7
#define M9_1_7    8

#define M9_2_0    2
#define M9_2_1    3
#define M9_2_2    4
#define M9_2_3    5
#define M9_2_4    6
#define M9_2_5    7
#define M9_2_6    8
#define M9_2_7    0

#define M9_3_0    3
#define M9_3_1    4
#define M9_3_2    5
#define M9_3_3    6
#define M9_3_4    7
#define M9_3_5    8
#define M9_3_6    0
#define M9_3_7    1

#define M9_4_0    4
#define M9_4_1    5
#define M9_4_2    6
#define M9_4_3    7
#define M9_4_4    8
#define M9_4_5    0
#define M9_4_6    1
#define M9_4_7    2

#define M9_5_0    5
#define M9_5_1    6
#define M9_5_2    7
#define M9_5_3    8
#define M9_5_4    0
#define M9_5_5    1
#define M9_5_6    2
#define M9_5_7    3

#define M9_6_0    6
#define M9_6_1    7
#define M9_6_2    8
#define M9_6_3    0
#define M9_6_4    1
#define M9_6_5    2
#define M9_6_6    3
#define M9_6_7    4

#define M9_7_0    7
#define M9_7_1    8
#define M9_7_2    0
#define M9_7_3    1
#define M9_7_4    2
#define M9_7_5    3
#define M9_7_6    4
#define M9_7_7    5

#define M9_8_0    8
#define M9_8_1    0
#define M9_8_2    1
#define M9_8_3    2
