/* MDH WCET BENCHMARK SUITE. File version $Id: fdct.c 7801 2018-01-19 13:33:32Z lkg02 $ */

let CONST_BITS: Int64 = 13
let PASS1_BITS: Int64 = 2

var out: Int64 = 0

var block: Array<Int64> = [
    99, 104, 109, 113, 115, 115, 55, 55,
    104, 111, 113, 116, 119, 56, 56, 56,
    110, 115, 120, 119, 118, 56, 56, 56,
    119, 121, 122, 120, 120, 59, 59, 59,
    119, 120, 121, 122, 122, 55, 55, 55,
    121, 121, 121, 121, 60, 57, 57, 57,
    122, 122, 61, 63, 62, 57, 57, 57,
    62, 62, 61, 61, 63, 58, 58, 58
]

func fdct(blk: Array<Int64>, lx: Int64) {
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
    var i: Int64 = 0
    var constant: Int64 = 0
    var row: Int64 = 0

    /* Pass 1: process rows. */
    i = 0
    while (i < 8) {
        row = i * lx
        tmp0 = blk[row + 0] + blk[row + 7]
        tmp7 = blk[row + 0] - blk[row + 7]
        tmp1 = blk[row + 1] + blk[row + 6]
        tmp6 = blk[row + 1] - blk[row + 6]
        tmp2 = blk[row + 2] + blk[row + 5]
        tmp5 = blk[row + 2] - blk[row + 5]
        tmp3 = blk[row + 3] + blk[row + 4]
        tmp4 = blk[row + 3] - blk[row + 4]

        tmp10 = tmp0 + tmp3
        tmp13 = tmp0 - tmp3
        tmp11 = tmp1 + tmp2
        tmp12 = tmp1 - tmp2

        blk[row + 0] = ((tmp10 + tmp11) << PASS1_BITS)
        blk[row + 4] = ((tmp10 - tmp11) << PASS1_BITS)

        constant = 4433
        z1 = (tmp12 + tmp13) * constant
        constant = 6270
        blk[row + 2] = (z1 + (tmp13 * constant)) >> (CONST_BITS - PASS1_BITS)
        constant = -15137
        blk[row + 6] = (z1 + (tmp12 * constant)) >> (CONST_BITS - PASS1_BITS)

        z1 = tmp4 + tmp7
        z2 = tmp5 + tmp6
        z3 = tmp4 + tmp6
        z4 = tmp5 + tmp7
        constant = 9633
        z5 = ((z3 + z4) * constant)

        constant = 2446
        tmp4 = (tmp4 * constant)
        constant = 16819
        tmp5 = (tmp5 * constant)
        constant = 25172
        tmp6 = (tmp6 * constant)
        constant = 12299
        tmp7 = (tmp7 * constant)
        constant = -7373
        z1 = (z1 * constant)
        constant = -20995
        z2 = (z2 * constant)
        constant = -16069
        z3 = (z3 * constant)
        constant = -3196
        z4 = (z4 * constant)

        z3 += z5
        z4 += z5

        blk[row + 7] = (tmp4 + z1 + z3) >> (CONST_BITS - PASS1_BITS)
        blk[row + 5] = (tmp5 + z2 + z4) >> (CONST_BITS - PASS1_BITS)
        blk[row + 3] = (tmp6 + z2 + z3) >> (CONST_BITS - PASS1_BITS)
        blk[row + 1] = (tmp7 + z1 + z4) >> (CONST_BITS - PASS1_BITS)

        i = i + 1
    }

    /* Pass 2: process columns. */
    i = 0
    while (i < 8) {
        var col: Int64 = i
        tmp0 = blk[col + 0 * lx] + blk[col + 7 * lx]
        tmp7 = blk[col + 0 * lx] - blk[col + 7 * lx]
        tmp1 = blk[col + 1 * lx] + blk[col + 6 * lx]
        tmp6 = blk[col + 1 * lx] - blk[col + 6 * lx]
        tmp2 = blk[col + 2 * lx] + blk[col + 5 * lx]
        tmp5 = blk[col + 2 * lx] - blk[col + 5 * lx]
        tmp3 = blk[col + 3 * lx] + blk[col + 4 * lx]
        tmp4 = blk[col + 3 * lx] - blk[col + 4 * lx]

        tmp10 = tmp0 + tmp3
        tmp13 = tmp0 - tmp3
        tmp11 = tmp1 + tmp2
        tmp12 = tmp1 - tmp2

        blk[col + 0 * lx] = (tmp10 + tmp11) >> (PASS1_BITS + 3)
        blk[col + 4 * lx] = (tmp10 - tmp11) >> (PASS1_BITS + 3)

        constant = 4433
        z1 = ((tmp12 + tmp13) * constant)
        constant = 6270
        blk[col + 2 * lx] = (z1 + (tmp13 * constant)) >> (CONST_BITS + PASS1_BITS + 3)
        constant = -15137
        blk[col + 6 * lx] = (z1 + (tmp12 * constant)) >> (CONST_BITS + PASS1_BITS + 3)

        z1 = tmp4 + tmp7
        z2 = tmp5 + tmp6
        z3 = tmp4 + tmp6
        z4 = tmp5 + tmp7
        constant = 9633
        z5 = ((z3 + z4) * constant)

        constant = 2446
        tmp4 = (tmp4 * constant)
        constant = 16819
        tmp5 = (tmp5 * constant)
        constant = 25172
        tmp6 = (tmp6 * constant)
        constant = 12299
        tmp7 = (tmp7 * constant)
        constant = -7373
        z1 = (z1 * constant)
        constant = -20995
        z2 = (z2 * constant)
        constant = -16069
        z3 = (z3 * constant)
        constant = -3196
        z4 = (z4 * constant)

        z3 += z5
        z4 += z5

        blk[col + 7 * lx] = (tmp4 + z1 + z3) >> (CONST_BITS + PASS1_BITS + 3)
        blk[col + 5 * lx] = (tmp5 + z2 + z4) >> (CONST_BITS + PASS1_BITS + 3)
        blk[col + 3 * lx] = (tmp6 + z2 + z3) >> (CONST_BITS + PASS1_BITS + 3)
        blk[col + 1 * lx] = (tmp7 + z1 + z4) >> (CONST_BITS + PASS1_BITS + 3)

        i = i + 1
    }
}

main(): Int64 {
    fdct(block, 8)
    return block[0]
}