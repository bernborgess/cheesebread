/* MDH WCET BENCHMARK SUITE. $Id: jfdctint.c 7800 2018-01-19 12:43:12Z lkg02 $ */

/*************************************************************************/
/*                                                                       */
/*   SNU-RT Benchmark Suite for Worst Case Timing Analysis               */
/*   =====================================================               */
/*                              Collected and Modified by S.-S. Lim      */
/*                                           sslim@archi.snu.ac.kr       */
/*                                         Real-Time Research Group      */
/*                                        Seoul National University      */
/*                                                                       */
/*                                                                       */
/*        < Features > - restrictions for our experimental environment   */
/*                                                                       */
/*          1. Completely structured.                                    */
/*               - There are no unconditional jumps.                     */
/*               - There are no exit from loop bodies.                   */
/*                 (There are no 'break' or 'return' in loop bodies)     */
/*          2. No 'switch' statements.                                   */
/*          3. No 'do..while' statements.                                */
/*          4. Expressions are restricted.                               */
/*               - There are no multiple expressions joined by 'or',     */
/*                'and' operations.                                      */
/*          5. No library calls.                                         */
/*               - All the functions needed are implemented in the       */
/*                 source file.                                          */
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  FILE: jfdctint.c                                                     */
/*  SOURCE : Thomas G. Lane,  Public domain JPEG source code.            */
/*           Modified by Steven Li at Princeton University.              */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     JPEG slow-but-accurate integer implementation of the forward      */
/*     DCT (Discrete Cosine Transform).                                  */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*************************************************************************/

/*
 * This module is specialized to the case DCTSIZE = 8.
 */

let DCTSIZE: Int64 = 8

let CONST_BITS: Int64 = 13
let PASS1_BITS: Int64 = 2

/* Some C compilers fail to reduce "FIX(constant)" at compile time, thus
 * causing a lot of useless floating-point operations at run time.
 * To get around this we use the following pre-calculated constants.
 * If you change CONST_BITS you may want to add appropriate values.
 * (With a reasonable C compiler, you can just rely on the FIX() macro...)
 */

let FIX_0_298631336: Int64 = 2446	/* FIX(0.298631336) */
let FIX_0_390180644: Int64 = 3196	/* FIX(0.390180644) */
let FIX_0_541196100: Int64 = 4433	/* FIX(0.541196100) */
let FIX_0_765366865: Int64 = 6270	/* FIX(0.765366865) */
let FIX_0_899976223: Int64 = 7373	/* FIX(0.899976223) */
let FIX_1_175875602: Int64 = 9633	/* FIX(1.175875602) */
let FIX_1_501321110: Int64 = 12299	/* FIX(1.501321110) */
let FIX_1_847759065: Int64 = 15137	/* FIX(1.847759065) */
let FIX_1_961570560: Int64 = 16069	/* FIX(1.961570560) */
let FIX_2_053119869: Int64 = 16819	/* FIX(2.053119869) */
let FIX_2_562915447: Int64 = 20995	/* FIX(2.562915447) */
let FIX_3_072711026: Int64 = 25172	/* FIX(3.072711026) */

/* DESCALE(x,n) == RIGHT_SHIFT((x) + (ONE << ((n)-1)), n) */
func descale(x: Int64, n: Int64): Int64 {
    return (x + (1 << (n - 1))) >> n
}

var data: Array<Int64> = Array<Int64>(64, repeat: 0)

/*
 * Perform the forward DCT on one block of samples.
 */

func jpeg_fdct_islow(): Unit {
    var tmp0: Int64 = 0
    var tmp1: Int64 = 0
    var tmp2: Int64 = 0
    var tmp3: Int64 = 0
    var tmp4: Int64 = 0
    var tmp5: Int64 = 0
    var tmp6: Int64 = 0
    var tmp7: Int64 = 0
    var tmp10: Int64 = 0
    var tmp11: Int64 = 0
    var tmp12: Int64 = 0
    var tmp13: Int64 = 0
    var z1: Int64 = 0
    var z2: Int64 = 0
    var z3: Int64 = 0
    var z4: Int64 = 0
    var z5: Int64 = 0
    var base: Int64 = 0
    var ctr: Int64 = 0

    /* Pass 1: process rows. */
    /* Note results are scaled up by sqrt(8) compared to a true DCT; */
    /* furthermore, we scale the results by 2**PASS1_BITS. */

    base = 0
    ctr = DCTSIZE - 1
    while (ctr >= 0) {
        tmp0 = data[base + 0] + data[base + 7]
        tmp7 = data[base + 0] - data[base + 7]
        tmp1 = data[base + 1] + data[base + 6]
        tmp6 = data[base + 1] - data[base + 6]
        tmp2 = data[base + 2] + data[base + 5]
        tmp5 = data[base + 2] - data[base + 5]
        tmp3 = data[base + 3] + data[base + 4]
        tmp4 = data[base + 3] - data[base + 4]

        /* Even part per LL&M figure 1 --- note that published figure is faulty;
         * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
         */

        tmp10 = tmp0 + tmp3
        tmp13 = tmp0 - tmp3
        tmp11 = tmp1 + tmp2
        tmp12 = tmp1 - tmp2

        data[base + 0] = (tmp10 + tmp11) << PASS1_BITS
        data[base + 4] = (tmp10 - tmp11) << PASS1_BITS

        z1 = (tmp12 + tmp13) * FIX_0_541196100
        data[base + 2] = descale(z1 + (tmp13 * FIX_0_765366865),
                                 CONST_BITS - PASS1_BITS)
        data[base + 6] = descale(z1 + (tmp12 * (-FIX_1_847759065)),
                                 CONST_BITS - PASS1_BITS)

        /* Odd part per figure 8 --- note paper omits factor of sqrt(2).
         * cK represents cos(K*pi/16).
         * i0..i3 in the paper are tmp4..tmp7 here.
         */

        z1 = tmp4 + tmp7
        z2 = tmp5 + tmp6
        z3 = tmp4 + tmp6
        z4 = tmp5 + tmp7
        z5 = (z3 + z4) * FIX_1_175875602 /* sqrt(2) * c3 */

        tmp4 = tmp4 * FIX_0_298631336 /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp5 = tmp5 * FIX_2_053119869 /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp6 = tmp6 * FIX_3_072711026 /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp7 = tmp7 * FIX_1_501321110 /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = z1 * (-FIX_0_899976223) /* sqrt(2) * (c7-c3) */
        z2 = z2 * (-FIX_2_562915447) /* sqrt(2) * (-c1-c3) */
        z3 = z3 * (-FIX_1_961570560) /* sqrt(2) * (-c3-c5) */
        z4 = z4 * (-FIX_0_390180644) /* sqrt(2) * (c5-c3) */

        z3 += z5
        z4 += z5

        data[base + 7] = descale(tmp4 + z1 + z3, CONST_BITS - PASS1_BITS)
        data[base + 5] = descale(tmp5 + z2 + z4, CONST_BITS - PASS1_BITS)
        data[base + 3] = descale(tmp6 + z2 + z3, CONST_BITS - PASS1_BITS)
        data[base + 1] = descale(tmp7 + z1 + z4, CONST_BITS - PASS1_BITS)

        base += DCTSIZE		/* advance pointer to next row */
        ctr = ctr - 1
    }

    /* Pass 2: process columns.
     * We remove the PASS1_BITS scaling, but leave the results scaled up
     * by an overall factor of 8.
     */

    base = 0
    ctr = DCTSIZE - 1
    while (ctr >= 0) {
        tmp0 = data[base + DCTSIZE * 0] + data[base + DCTSIZE * 7]
        tmp7 = data[base + DCTSIZE * 0] - data[base + DCTSIZE * 7]
        tmp1 = data[base + DCTSIZE * 1] + data[base + DCTSIZE * 6]
        tmp6 = data[base + DCTSIZE * 1] - data[base + DCTSIZE * 6]
        tmp2 = data[base + DCTSIZE * 2] + data[base + DCTSIZE * 5]
        tmp5 = data[base + DCTSIZE * 2] - data[base + DCTSIZE * 5]
        tmp3 = data[base + DCTSIZE * 3] + data[base + DCTSIZE * 4]
        tmp4 = data[base + DCTSIZE * 3] - data[base + DCTSIZE * 4]

        /* Even part per LL&M figure 1 --- note that published figure is faulty;
         * rotator "sqrt(2)*c1" should be "sqrt(2)*c6".
         */

        tmp10 = tmp0 + tmp3
        tmp13 = tmp0 - tmp3
        tmp11 = tmp1 + tmp2
        tmp12 = tmp1 - tmp2

        data[base + DCTSIZE * 0] = descale(tmp10 + tmp11, PASS1_BITS)
        data[base + DCTSIZE * 4] = descale(tmp10 - tmp11, PASS1_BITS)

        z1 = (tmp12 + tmp13) * FIX_0_541196100
        data[base + DCTSIZE * 2] = descale(z1 + (tmp13 * FIX_0_765366865),
                                           CONST_BITS + PASS1_BITS)
        data[base + DCTSIZE * 6] = descale(z1 + (tmp12 * (-FIX_1_847759065)),
                                           CONST_BITS + PASS1_BITS)

        /* Odd part per figure 8 --- note paper omits factor of sqrt(2).
         * cK represents cos(K*pi/16).
         * i0..i3 in the paper are tmp4..tmp7 here.
         */

        z1 = tmp4 + tmp7
        z2 = tmp5 + tmp6
        z3 = tmp4 + tmp6
        z4 = tmp5 + tmp7
        z5 = (z3 + z4) * FIX_1_175875602 /* sqrt(2) * c3 */

        tmp4 = tmp4 * FIX_0_298631336 /* sqrt(2) * (-c1+c3+c5-c7) */
        tmp5 = tmp5 * FIX_2_053119869 /* sqrt(2) * ( c1+c3-c5+c7) */
        tmp6 = tmp6 * FIX_3_072711026 /* sqrt(2) * ( c1+c3+c5-c7) */
        tmp7 = tmp7 * FIX_1_501321110 /* sqrt(2) * ( c1+c3-c5-c7) */
        z1 = z1 * (-FIX_0_899976223) /* sqrt(2) * (c7-c3) */
        z2 = z2 * (-FIX_2_562915447) /* sqrt(2) * (-c1-c3) */
        z3 = z3 * (-FIX_1_961570560) /* sqrt(2) * (-c3-c5) */
        z4 = z4 * (-FIX_0_390180644) /* sqrt(2) * (c5-c3) */

        z3 += z5
        z4 += z5

        data[base + DCTSIZE * 7] = descale(tmp4 + z1 + z3,
                                           CONST_BITS + PASS1_BITS)
        data[base + DCTSIZE * 5] = descale(tmp5 + z2 + z4,
                                           CONST_BITS + PASS1_BITS)
        data[base + DCTSIZE * 3] = descale(tmp6 + z2 + z3,
                                           CONST_BITS + PASS1_BITS)
        data[base + DCTSIZE * 1] = descale(tmp7 + z1 + z4,
                                           CONST_BITS + PASS1_BITS)

        base += 1			/* advance pointer to next column */
        ctr = ctr - 1
    }
}

/* Main function
   Time to function execution time using logic analyzer,
   which measures the OFF time of a LED on board.

   The switching latency, including the function call/return time,
   is measured to be equal to 1.1us (22 clock cycles).
 */
main(): Int64 {
    var i: Int64 = 0
    var seed: Int64 = 0

    /* Worst case settings */
    /* Set array to random values */
    seed = 1
    i = 0
    while (i < 64) {
        seed = ((seed * 133) + 81) % 65535
        data[i] = seed
        i = i + 1
    }

    jpeg_fdct_islow()
    return 0
}
