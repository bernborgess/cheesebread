let SAMPLE_RATE: Int64 = 11025
let PI: Int64 = 3141
let SIZE: Int64 = 3
let IN_END: Int64 = 4

var tqmf: Array<Int64> = Array<Int64>(24, repeat: 0)
var h: Array<Int64> = [
    12, -44, -44, 212, 48, -624, 128, 1448,
    -840, -3220, 3804, 15504, 15504, 3804, -3220, -840,
    1448, 128, -624, 48, 212, -44, -44, 12
]

var xl: Int64 = 0
var xh: Int64 = 0

var accumc: Array<Int64> = Array<Int64>(11, repeat: 0)
var accumd: Array<Int64> = Array<Int64>(11, repeat: 0)

var xout1: Int64 = 0
var xout2: Int64 = 0

var xs: Int64 = 0
var xd: Int64 = 0

var il: Int64 = 0
var szl: Int64 = 0
var spl: Int64 = 0
var sl: Int64 = 0
var el: Int64 = 0

var qq4_code4_table: Array<Int64> = [
    0, -20456, -12896, -8968, -6288, -4240, -2584, -1200,
    20456, 12896, 8968, 6288, 4240, 2584, 1200, 0
]

var qq5_code5_table: Array<Int64> = [
    -280, -280, -23352, -17560, -14120, -11664, -9752, -8184,
    -6864, -5712, -4696, -3784, -2960, -2208, -1520, -880,
    23352, 17560, 14120, 11664, 9752, 8184, 6864, 5712,
    4696, 3784, 2960, 2208, 1520, 880, 280, -280
]

var qq6_code6_table: Array<Int64> = [
    -136, -136, -136, -136, -24808, -21904, -19008, -16704,
    -14984, -13512, -12280, -11192, -10232, -9360, -8576, -7856,
    -7192, -6576, -6000, -5456, -4944, -4464, -4008, -3576,
    -3168, -2776, -2400, -2032, -1688, -1360, -1040, -728,
    24808, 21904, 19008, 16704, 14984, 13512, 12280, 11192,
    10232, 9360, 8576, 7856, 7192, 6576, 6000, 5456,
    4944, 4464, 4008, 3576, 3168, 2776, 2400, 2032,
    1688, 1360, 1040, 728, 432, 136, -432, -136
]

var delay_bpl: Array<Int64> = Array<Int64>(6, repeat: 0)
var delay_dltx: Array<Int64> = Array<Int64>(6, repeat: 0)

var wl_code_table: Array<Int64> = [
    -60, 3042, 1198, 538, 334, 172, 58, -30,
    3042, 1198, 538, 334, 172, 58, -30, -60
]

var wl_table: Array<Int64> = [
    -60, -30, 58, 172, 334, 538, 1198, 3042
]

var ilb_table: Array<Int64> = [
    2048, 2093, 2139, 2186, 2233, 2282, 2332, 2383,
    2435, 2489, 2543, 2599, 2656, 2714, 2774, 2834,
    2896, 2960, 3025, 3091, 3158, 3228, 3298, 3371,
    3444, 3520, 3597, 3676, 3756, 3838, 3922, 4008
]

var nbl: Int64 = 0
var al1: Int64 = 0
var al2: Int64 = 0
var plt: Int64 = 0
var plt1: Int64 = 0
var plt2: Int64 = 0
var rs: Int64 = 0
var dlt: Int64 = 0
var rlt: Int64 = 0
var rlt1: Int64 = 0
var rlt2: Int64 = 0

var decis_levl: Array<Int64> = [
    280, 576, 880, 1200, 1520, 1864, 2208, 2584,
    2960, 3376, 3784, 4240, 4696, 5200, 5712, 6288,
    6864, 7520, 8184, 8968, 9752, 10712, 11664, 12896,
    14120, 15840, 17560, 20456, 23352, 32767
]

var detl: Int64 = 0

var quant26bt_pos: Array<Int64> = [
    61, 60, 59, 58, 57, 56, 55, 54,
    53, 52, 51, 50, 49, 48, 47, 46,
    45, 44, 43, 42, 41, 40, 39, 38,
    37, 36, 35, 34, 33, 32, 32
]

var quant26bt_neg: Array<Int64> = [
    63, 62, 31, 30, 29, 28, 27, 26,
    25, 24, 23, 22, 21, 20, 19, 18,
    17, 16, 15, 14, 13, 12, 11, 10,
    9, 8, 7, 6, 5, 4, 4
]

var deth: Int64 = 0
var sh: Int64 = 0
var eh: Int64 = 0

var qq2_code2_table: Array<Int64> = [-7408, -1616, 7408, 1616]
var wh_code_table: Array<Int64> = [798, -214, 798, -214]

var dh: Int64 = 0
var ih: Int64 = 0
var nbh: Int64 = 0
var szh: Int64 = 0
var sph: Int64 = 0
var ph: Int64 = 0
var yh: Int64 = 0
var rh: Int64 = 0

var delay_dhx: Array<Int64> = Array<Int64>(6, repeat: 0)
var delay_bph: Array<Int64> = Array<Int64>(6, repeat: 0)

var ah1: Int64 = 0
var ah2: Int64 = 0
var ph1: Int64 = 0
var ph2: Int64 = 0
var rh1: Int64 = 0
var rh2: Int64 = 0

var ilr: Int64 = 0
var yl: Int64 = 0
var rl: Int64 = 0
var dec_deth: Int64 = 0
var dec_detl: Int64 = 0
var dec_dlt: Int64 = 0

var dec_del_bpl: Array<Int64> = Array<Int64>(6, repeat: 0)
var dec_del_dltx: Array<Int64> = Array<Int64>(6, repeat: 0)

var dec_plt: Int64 = 0
var dec_plt1: Int64 = 0
var dec_plt2: Int64 = 0
var dec_szl: Int64 = 0
var dec_spl: Int64 = 0
var dec_sl: Int64 = 0
var dec_rlt1: Int64 = 0
var dec_rlt2: Int64 = 0
var dec_rlt: Int64 = 0
var dec_al1: Int64 = 0
var dec_al2: Int64 = 0
var dl: Int64 = 0
var dec_nbl: Int64 = 0
var dec_yh: Int64 = 0
var dec_dh: Int64 = 0
var dec_nbh: Int64 = 0

var dec_del_bph: Array<Int64> = Array<Int64>(6, repeat: 0)
var dec_del_dhx: Array<Int64> = Array<Int64>(6, repeat: 0)

var dec_szh: Int64 = 0
var dec_rh1: Int64 = 0
var dec_rh2: Int64 = 0
var dec_ah1: Int64 = 0
var dec_ah2: Int64 = 0
var dec_ph: Int64 = 0
var dec_sph: Int64 = 0

var dec_sh: Int64 = 0
var dec_rh: Int64 = 0

var dec_ph1: Int64 = 0
var dec_ph2: Int64 = 0

func my_abs(n: Int64): Int64 {
    var m: Int64 = 0
    if (n >= 0) {
        m = n
    } else {
        m = -n
    }
    return m
}

func my_fabs(n: Int64): Int64 {
    var f: Int64 = 0
    if (n >= 0) {
        f = n
    } else {
        f = -n
    }
    return f
}

// The original C code uses `int` (32 bit) throughout. Its Taylor-series style
// expansion in my_sin() deliberately lets intermediate products overflow 32
// bits and wrap, which is what bounds the number of loop iterations. Emulate
// that wraparound so the Cangjie version iterates the same number of times.
func w32(x: Int64): Int64 {
    let u: Int64 = x & 0xFFFFFFFF
    if (u >= 0x80000000) {
        u - 0x100000000
    } else {
        u
    }
}

func my_sin(rad: Int64): Int64 {
    var r: Int64 = rad
    var diff: Int64 = 0
    var app: Int64 = 0
    var inc: Int64 = 1

    while (r > 2 * PI) {
        r = r - 2 * PI
    }
    while (r < -2 * PI) {
        r = r + 2 * PI
    }
    diff = r
    app = diff
    diff = w32(diff * (-w32(r * r))) / w32(w32(2 * inc) * w32(2 * inc + 1))
    app = w32(app + diff)
    inc = inc + 1
    while (my_fabs(diff) >= 1) {
        diff = w32(diff * (-w32(r * r))) / w32(w32(2 * inc) * w32(2 * inc + 1))
        app = w32(app + diff)
        inc = inc + 1
    }
    return app
}

func my_cos(rad: Int64): Int64 {
    return my_sin(PI / 2 - rad)
}

func encode(xin1: Int64, xin2: Int64): Int64 {
    var i: Int64 = 0
    var xa: Int64 = 0
    var xb: Int64 = 0
    var decisive: Int64 = 0

    var hi: Int64 = 0
    var ti: Int64 = 0
    xa = tqmf[ti] * h[hi]; ti = ti + 1; hi = hi + 1
    xb = tqmf[ti] * h[hi]; ti = ti + 1; hi = hi + 1
    i = 0
    while (i < 10) {
        xa = xa + tqmf[ti] * h[hi]; ti = ti + 1; hi = hi + 1
        xb = xb + tqmf[ti] * h[hi]; ti = ti + 1; hi = hi + 1
        i = i + 1
    }
    xa = xa + tqmf[ti] * h[hi]; ti = ti + 1; hi = hi + 1
    xb = xb + tqmf[ti] * h[hi]

    var tqmf_ptr: Int64 = 23
    var tqmf_ptr1: Int64 = tqmf_ptr - 2
    i = 0
    while (i < 22) {
        tqmf[tqmf_ptr] = tqmf[tqmf_ptr1]
        tqmf_ptr = tqmf_ptr - 1
        tqmf_ptr1 = tqmf_ptr1 - 1
        i = i + 1
    }
    tqmf[tqmf_ptr] = xin1
    tqmf_ptr = tqmf_ptr - 1
    tqmf[tqmf_ptr] = xin2

    xl = (xa + xb) >> 15
    xh = (xa - xb) >> 15

    szl = filtez(delay_bpl, delay_dltx)
    spl = filtep(rlt1, al1, rlt2, al2)
    sl = szl + spl
    el = xl - sl
    il = quantl(el, detl)
    dlt = (detl * qq4_code4_table[il >> 2]) >> 15
    nbl = logscl(il, nbl)
    detl = scalel(nbl, 8)
    plt = dlt + szl
    upzero(dlt, delay_dltx, delay_bpl)
    al2 = uppol2(al1, al2, plt, plt1, plt2)
    al1 = uppol1(al1, al2, plt, plt1)
    rlt = sl + dlt
    rlt2 = rlt1
    rlt1 = rlt
    plt2 = plt1
    plt1 = plt

    szh = filtez(delay_bph, delay_dhx)
    sph = filtep(rh1, ah1, rh2, ah2)
    sh = sph + szh
    eh = xh - sh
    if (eh >= 0) {
        ih = 3
    } else {
        ih = 1
    }
    decisive = (564 * deth) >> 12
    if (my_abs(eh) > decisive) {
        ih = ih - 1
    }
    dh = (deth * qq2_code2_table[ih]) >> 15
    nbh = logsch(ih, nbh)
    deth = scalel(nbh, 10)
    ph = dh + szh
    upzero(dh, delay_dhx, delay_bph)
    ah2 = uppol2(ah1, ah2, ph, ph1, ph2)
    ah1 = uppol1(ah1, ah2, ph, ph1)
    yh = sh + dh
    rh2 = rh1
    rh1 = yh
    ph2 = ph1
    ph1 = ph

    return (il | (ih << 6))
}

func decode(input: Int64): Unit {
    var i: Int64 = 0
    var xa1: Int64 = 0
    var xa2: Int64 = 0

    ilr = input & 0x3f
    ih = input >> 6

    dec_szl = filtez(dec_del_bpl, dec_del_dltx)
    dec_spl = filtep(dec_rlt1, dec_al1, dec_rlt2, dec_al2)
    dec_sl = dec_spl + dec_szl
    dec_dlt = (dec_detl * qq4_code4_table[ilr >> 2]) >> 15
    dl = (dec_detl * qq6_code6_table[il]) >> 15
    rl = dl + dec_sl
    dec_nbl = logscl(ilr, dec_nbl)
    dec_detl = scalel(dec_nbl, 8)
    dec_plt = dec_dlt + dec_szl
    upzero(dec_dlt, dec_del_dltx, dec_del_bpl)
    dec_al2 = uppol2(dec_al1, dec_al2, dec_plt, dec_plt1, dec_plt2)
    dec_al1 = uppol1(dec_al1, dec_al2, dec_plt, dec_plt1)
    dec_rlt = dec_sl + dec_dlt
    dec_rlt2 = dec_rlt1
    dec_rlt1 = dec_rlt
    dec_plt2 = dec_plt1
    dec_plt1 = dec_plt

    dec_szh = filtez(dec_del_bph, dec_del_dhx)
    dec_sph = filtep(dec_rh1, dec_ah1, dec_rh2, dec_ah2)
    dec_sh = dec_sph + dec_szh
    dec_dh = (dec_deth * qq2_code2_table[ih]) >> 15
    dec_nbh = logsch(ih, dec_nbh)
    dec_deth = scalel(dec_nbh, 10)
    dec_ph = dec_dh + dec_szh
    upzero(dec_dh, dec_del_dhx, dec_del_bph)
    dec_ah2 = uppol2(dec_ah1, dec_ah2, dec_ph, dec_ph1, dec_ph2)
    dec_ah1 = uppol1(dec_ah1, dec_ah2, dec_ph, dec_ph1)
    rh = dec_sh + dec_dh
    dec_rh2 = dec_rh1
    dec_rh1 = rh
    dec_ph2 = dec_ph1
    dec_ph1 = dec_ph

    xd = rl - rh
    xs = rl + rh

    var hi: Int64 = 0
    xa1 = xd * h[hi]; hi = hi + 1
    xa2 = xs * h[hi]; hi = hi + 1
    var aci: Int64 = 0
    var adi: Int64 = 0
    i = 0
    while (i < 10) {
        xa1 = xa1 + accumc[aci] * h[hi]; aci = aci + 1; hi = hi + 1
        xa2 = xa2 + accumd[adi] * h[hi]; adi = adi + 1; hi = hi + 1
        i = i + 1
    }
    xa1 = xa1 + accumc[aci] * h[hi]; hi = hi + 1
    xa2 = xa2 + accumd[adi] * h[hi]; hi = hi + 1

    xout1 = xa1 >> 14
    xout2 = xa2 >> 14

    var aci_src: Int64 = aci - 1
    var adi_src: Int64 = adi - 1
    i = 0
    while (i < 10) {
        accumc[aci] = accumc[aci_src]
        accumd[adi] = accumd[adi_src]
        aci = aci - 1
        adi = adi - 1
        aci_src = aci_src - 1
        adi_src = adi_src - 1
        i = i + 1
    }
    accumc[aci] = xd
    accumd[adi] = xs
}

func reset(): Unit {
    detl = 32
    dec_detl = 32
    deth = 8
    dec_deth = 8
    nbl = 0
    al1 = 0
    al2 = 0
    plt1 = 0
    plt2 = 0
    rlt1 = 0
    rlt2 = 0
    nbh = 0
    ah1 = 0
    ah2 = 0
    ph1 = 0
    ph2 = 0
    rh1 = 0
    rh2 = 0
    dec_nbl = 0
    dec_al1 = 0
    dec_al2 = 0
    dec_plt1 = 0
    dec_plt2 = 0
    dec_rlt1 = 0
    dec_rlt2 = 0
    dec_nbh = 0
    dec_ah1 = 0
    dec_ah2 = 0
    dec_ph1 = 0
    dec_ph2 = 0
    dec_rh1 = 0
    dec_rh2 = 0

    var i: Int64 = 0
    while (i < 6) {
        delay_dltx[i] = 0
        delay_dhx[i] = 0
        dec_del_dltx[i] = 0
        dec_del_dhx[i] = 0
        delay_bpl[i] = 0
        delay_bph[i] = 0
        dec_del_bpl[i] = 0
        dec_del_bph[i] = 0
        i = i + 1
    }
    i = 0
    while (i < 23) {
        tqmf[i] = 0
        i = i + 1
    }
    tqmf[23] = 0
    i = 0
    while (i < 11) {
        accumc[i] = 0
        accumd[i] = 0
        i = i + 1
    }
}

func filtez(bpl: Array<Int64>, dlt: Array<Int64>): Int64 {
    var zl: Int64 = bpl[0] * dlt[0]
    var i: Int64 = 1
    while (i < 6) {
        zl = zl + bpl[i] * dlt[i]
        i = i + 1
    }
    return zl >> 14
}

func filtep(rlt1: Int64, al1: Int64, rlt2: Int64, al2: Int64): Int64 {
    var pl: Int64 = 0
    var pl2: Int64 = 0
    pl = 2 * rlt1
    pl = al1 * pl
    pl2 = 2 * rlt2
    pl = pl + al2 * pl2
    return pl >> 15
}

func quantl(el: Int64, detl: Int64): Int64 {
    var wd: Int64 = my_abs(el)
    var mil: Int64 = 0
    var decis: Int64 = 0
    while (mil < 30) {
        decis = (decis_levl[mil] * detl) >> 15
        if (wd <= decis) {
            break
        }
        mil = mil + 1
    }
    if (el >= 0) {
        return quant26bt_pos[mil]
    } else {
        return quant26bt_neg[mil]
    }
}

func logscl(il: Int64, nbl: Int64): Int64 {
    var wd: Int64 = 0
    var n: Int64 = 0
    wd = (nbl * 127) >> 7
    n = wd + wl_code_table[il >> 2]
    if (n < 0) {
        n = 0
    }
    if (n > 18432) {
        n = 18432
    }
    return n
}

func scalel(nbl: Int64, shift_constant: Int64): Int64 {
    var wd1: Int64 = (nbl >> 6) & 31
    var wd2: Int64 = nbl >> 11
    var wd3: Int64 = ilb_table[wd1] >> (shift_constant + 1 - wd2)
    return wd3 << 3
}

func upzero(dlt: Int64, dlti: Array<Int64>, bli: Array<Int64>): Unit {
    var i: Int64 = 0
    var wd2: Int64 = 0
    var wd3: Int64 = 0
    if (dlt == 0) {
        i = 0
        while (i < 6) {
            bli[i] = (255 * bli[i]) >> 8
            i = i + 1
        }
    } else {
        i = 0
        while (i < 6) {
            if (dlt * dlti[i] >= 0) {
                wd2 = 128
            } else {
                wd2 = -128
            }
            wd3 = (255 * bli[i]) >> 8
            bli[i] = wd2 + wd3
            i = i + 1
        }
    }
    dlti[5] = dlti[4]
    dlti[4] = dlti[3]
    dlti[3] = dlti[2]
    dlti[1] = dlti[0]
    dlti[0] = dlt
}

func uppol2(al1: Int64, al2: Int64, plt: Int64, plt1: Int64, plt2: Int64): Int64 {
    var wd2: Int64 = 0
    var wd4: Int64 = 0
    var apl2: Int64 = 0
    wd2 = 4 * al1
    if (plt * plt1 >= 0) {
        wd2 = -wd2
    }
    wd2 = wd2 >> 7
    if (plt * plt2 >= 0) {
        wd4 = wd2 + 128
    } else {
        wd4 = wd2 - 128
    }
    apl2 = wd4 + ((127 * al2) >> 7)
    if (apl2 > 12288) {
        apl2 = 12288
    }
    if (apl2 < -12288) {
        apl2 = -12288
    }
    return apl2
}

func uppol1(al1: Int64, apl2: Int64, plt: Int64, plt1: Int64): Int64 {
    var wd2: Int64 = 0
    var wd3: Int64 = 0
    var apl1: Int64 = 0
    wd2 = (al1 * 255) >> 8
    if (plt * plt1 >= 0) {
        apl1 = wd2 + 192
    } else {
        apl1 = wd2 - 192
    }
    wd3 = 15360 - apl2
    if (apl1 > wd3) {
        apl1 = wd3
    }
    if (apl1 < -wd3) {
        apl1 = -wd3
    }
    return apl1
}

func logsch(ih: Int64, nbh: Int64): Int64 {
    var wd: Int64 = 0
    var n: Int64 = 0
    wd = (nbh * 127) >> 7
    n = wd + wh_code_table[ih]
    if (n < 0) {
        n = 0
    }
    if (n > 22528) {
        n = 22528
    }
    return n
}

main(): Int64 {
    var i: Int64 = 0
    var j: Int64 = 0
    var f: Int64 = 0
    var test_data: Array<Int64> = Array<Int64>(SIZE * 2, repeat: 0)
    var compressed: Array<Int64> = Array<Int64>(SIZE, repeat: 0)
    var result: Array<Int64> = Array<Int64>(SIZE * 2, repeat: 0)

    reset()

    j = 10
    f = 2000

    i = 0
    while (i < SIZE) {
        test_data[i] = w32(j * my_cos(f * PI * i))
        i = i + 1
    }

    i = 0
    while (i < IN_END) {
        compressed[i / 2] = encode(test_data[i], test_data[i + 1])
        i = i + 2
    }
    i = 0
    while (i < IN_END) {
        decode(compressed[i / 2])
        result[i] = xout1
        result[i + 1] = xout2
        i = i + 2
    }

    return result[i] + result[i + 1]
}
