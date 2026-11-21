/* MDH WCET BENCHMARK SUITE. File version $Id: edn.c 7927 2018-08-01 12:18:41Z lkg02 $ */

let N: Int64 = 100
let ORDER: Int64 = 50

func vec_mpy1(y: Array<Int64>, x: Array<Int64>, scaler: Int64): Unit {
    var i: Int64 = 0
    while (i < 150) {
        y[i] = toShort(y[i] + ((scaler * x[i]) >> 15))
        i = i + 1
    }
}

func mac(a: Array<Int64>, b: Array<Int64>, sqr: Int64, sum: Array<Int64>): Int64 {
    var i: Int64 = 0
    var dotp: Int64 = sum[0]
    var sqrVar: Int64 = sqr
    while (i < 150) {
        dotp = dotp + b[i] * a[i]
        sqrVar = sqrVar + b[i] * b[i]
        i = i + 1
    }
    sum[0] = dotp
    return sqrVar
}

func fir(array1: Array<Int64>, coeff: Array<Int64>, output: Array<Int64>): Unit {
    var i: Int64 = 0
    var j: Int64 = 0
    var sum: Int64 = 0
    while (i < N - ORDER) {
        sum = 0
        j = 0
        while (j < ORDER) {
            sum = sum + array1[i + j] * coeff[j]
            j = j + 1
        }
        output[i] = sum >> 15
        i = i + 1
    }
}

func fir_no_red_ld(x: Array<Int64>, h: Array<Int64>, y: Array<Int64>): Unit {
    var i: Int64 = 0
    var j: Int64 = 0
    var sum0: Int64 = 0
    var sum1: Int64 = 0
    var x0: Int64 = 0
    var x1: Int64 = 0
    var h0: Int64 = 0
    var h1: Int64 = 0
    j = 0
    while (j < 100) {
        sum0 = 0
        sum1 = 0
        x0 = x[j]
        i = 0
        while (i < 32) {
            x1 = x[j + i + 1]
            h0 = h[i]
            sum0 = sum0 + x0 * h0
            sum1 = sum1 + x1 * h0
            x0 = x[j + i + 2]
            h1 = h[i + 1]
            sum0 = sum0 + x1 * h1
            sum1 = sum1 + x0 * h1
            i = i + 2
        }
        y[j] = sum0 >> 15
        y[j + 1] = sum1 >> 15
        j = j + 2
    }
}

func latsynth(bArr: Array<Int64>, kArr: Array<Int64>, n: Int64, f: Int64): Int64 {
    var i: Int64 = 0
    var fVar: Int64 = f
    fVar = fVar - bArr[n - 1] * kArr[n - 1]
    i = n - 2
    while (i >= 0) {
        fVar = fVar - bArr[i] * kArr[i]
        bArr[i + 1] = toShort(bArr[i] + ((kArr[i] * (fVar >> 16)) >> 16))
        i = i - 1
    }
    bArr[0] = toShort(fVar >> 16)
    return fVar
}

func iir1(coefs: Array<Int64>, input: Array<Int64>, optr: Array<Int64>, optrIndex: Int64, state: Array<Int64>, stateIndex: Int64): Unit {
    var x: Int64 = input[0]
    var t: Int64 = 0
    var n: Int64 = 0
    var coefIndex: Int64 = 0
    var st: Int64 = stateIndex
    while (n < 50) {
        t = x + ((coefs[coefIndex + 2] * state[st] + coefs[coefIndex + 3] * state[st + 1]) >> 15)
        x = t + ((coefs[coefIndex] * state[st] + coefs[coefIndex + 1] * state[st + 1]) >> 15)
        state[st + 1] = state[st]
        state[st] = t
        coefIndex = coefIndex + 4
        st = st + 2
        n = n + 1
    }
    optr[optrIndex] = x
}

func codebook(mask: Int64, bitchanged: Int64, numbasis: Int64, codeword: Int64, g: Int64, d: Array<Int64>, ddim: Int64, theta: Int64): Int64 {
    var j: Int64 = 0
    var tmpMask: Int64 = mask << 1
    j = bitchanged + 1
    while (j <= numbasis) {
        /* removed code */
        j = j + 1
    }
    return g
}

func toShort(v: Int64): Int64 {
    var u: Int64 = v & 0xFFFF
    if (u >= 0x8000) {
        u = u - 0x10000
    }
    return u
}

func jpegdct(d: Array<Int64>, r: Array<Int64>): Unit {
    var t: Array<Int64> = Array<Int64>(12, repeat: 0)
    var i: Int64 = 0
    var j: Int64 = 0
    var k: Int64 = 1
    var m: Int64 = 0
    var n: Int64 = 13
    var p: Int64 = 8
    var di: Int64 = 0
    while (k <= 8) {
        i = 0
        while (i < 8) {
            j = 0
            while (j < 4) {
                t[j] = d[di + k * j] + d[di + k * (7 - j)]
                t[7 - j] = d[di + k * j] - d[di + k * (7 - j)]
                j = j + 1
            }
            t[8] = t[0] + t[3]
            t[9] = t[0] - t[3]
            t[10] = t[1] + t[2]
            t[11] = t[1] - t[2]
            d[di + 0] = toShort((t[8] + t[10]) >> m)
            d[di + 4 * k] = toShort((t[8] - t[10]) >> m)
            t[8] = toShort(t[11] + t[9]) * r[10]
            d[di + 2 * k] = toShort(t[8] + toShort((t[9] * r[9]) >> n))
            d[di + 6 * k] = toShort(t[8] + toShort((t[11] * r[11]) >> n))
            t[0] = toShort(t[4] + t[7]) * r[2]
            t[1] = toShort(t[5] + t[6]) * r[0]
            t[2] = t[4] + t[6]
            t[3] = t[5] + t[7]
            t[8] = toShort(t[2] + t[3]) * r[8]
            t[2] = toShort(t[2]) * r[1] + t[8]
            t[3] = toShort(t[3]) * r[3] + t[8]
            d[di + 7 * k] = toShort(toShort(t[4] * r[4] + t[0] + t[2]) >> n)
            d[di + 5 * k] = toShort(toShort(t[5] * r[6] + t[1] + t[3]) >> n)
            d[di + 3 * k] = toShort(toShort(t[6] * r[5] + t[1] + t[2]) >> n)
            d[di + 1 * k] = toShort(toShort(t[7] * r[7] + t[0] + t[3]) >> n)
            di = di + p
            i = i + 1
        }
        k = k + 7
        m = m + 3
        n = n + 3
        p = p - 7
        di = di - 64
    }
}

main(): Int64 {
    var a: Array<Int64> = [
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024,
    0, 2047, 3072, 2048, 512, -2048, -3328, 1024
    ]
    var b: Array<Int64> = [
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096,
    3168, 3136, 3104, 3072, -2560, -3072, -3584, -4096
    ]
    var c: Int64 = 0x3
    var output: Array<Int64> = Array<Int64>(200, repeat: 0)
    var d: Int64 = 0xAAAA
    var e: Array<Int64> = [0xEEEE]

    vec_mpy1(a, b, c)
    c = mac(a, b, c, output)
    fir(a, b, output)
    fir_no_red_ld(a, b, output)
    d = latsynth(a, b, N, d)
    iir1(a, b, output, 100, output, 0)
    e[0] = codebook(d, 1, 17, e[0], d, a, c, 1)
    jpegdct(a, b)

    return 0
}
