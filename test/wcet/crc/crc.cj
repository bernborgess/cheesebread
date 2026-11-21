/* MDH WCET BENCHMARK SUITE. File version $Id: crc.c 7800 2018-01-19 12:43:12Z lkg02 $ */

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
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  FILE: crc.c                                                          */
/*  SOURCE : Numerical Recipes in C - The Second Edition                 */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     A demonstration for CRC (Cyclic Redundancy Check) operation.      */
/*     The CRC is manipulated as two functions, icrc1 and icrc.          */
/*     icrc1 is for one character and icrc uses icrc1 for a string.      */
/*     The input string is stored in array lin[].                        */
/*     icrc is called two times, one for X-Modem string CRC and the      */
/*     other for X-Modem packet CRC.                                     */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
/* Changes:
 * JG 2005/12/12: Indented program.
 */

/* C's `typedef unsigned char uchar` maps to the Cangjie UInt8 type. */
func LOBYTE(x: UInt16): UInt8 {
    return UInt8(x & 0xFF)
}

func HIBYTE(x: UInt16): UInt8 {
    return UInt8(x >> 8)
}

var lin: Array<UInt8> = Array<UInt8>(256, repeat: 0)

func icrc1(crc: UInt16, onech: UInt8): UInt16 {
    var i: Int64 = 0
    var ans: UInt16 = crc ^ (UInt16(onech) << 8)

    while (i < 8) {
        if ((ans & 0x8000) != 0) {
            ans = ans << 1
            ans = ans ^ 0x1021
        } else {
            ans = ans << 1
        }
        i = i + 1
    }
    return ans
}

/* `static` locals of the C function become module level state. */
var icrctb: Array<UInt16> = Array<UInt16>(256, repeat: 0)
var rchr: Array<UInt8> = Array<UInt8>(256, repeat: 0)
var initFlag: Int64 = 0
let it: Array<UInt8> = [0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15]

func icrc(crc: UInt16, len: UInt64, jinit: Int16, jrev: Int64): UInt16 {
    var tmp1: UInt16 = 0
    var tmp2: UInt16 = 0
    var j: UInt16 = 0
    var cword: UInt16 = crc

    if (initFlag == 0) {
        initFlag = 1
        j = 0
        while (j <= 255) {
            icrctb[Int64(j)] = icrc1(j << 8, 0)
            rchr[Int64(j)] = (it[Int64(j & 0xF)] << 4) | it[Int64(j >> 4)]
            j = j + 1
        }
    }
    if (jinit >= 0) {
        cword = UInt16(jinit & 0xFF) | (UInt16(jinit & 0xFF) << 8)
    } else if (jrev < 0) {
        cword = UInt16(rchr[Int64(HIBYTE(cword))]) |
            (UInt16(rchr[Int64(LOBYTE(cword))]) << 8)
    }
    j = 1
    while (UInt64(j) <= len) {
        if (jrev < 0) {
            tmp1 = UInt16(rchr[Int64(lin[Int64(j)])] ^ HIBYTE(cword))
        } else {
            tmp1 = UInt16(lin[Int64(j)] ^ HIBYTE(cword))
        }
        cword = icrctb[Int64(tmp1)] ^ (UInt16(LOBYTE(cword)) << 8)
        j = j + 1
    }
    if (jrev >= 0) {
        tmp2 = cword
    } else {
        tmp2 = UInt16(rchr[Int64(HIBYTE(cword))]) |
            (UInt16(rchr[Int64(LOBYTE(cword))]) << 8)
    }
    return tmp2
}

main(): Int64 {
    var i1: UInt16 = 0
    var i2: UInt16 = 0
    var n: UInt64 = 0

    /* Initialise lin[] with the C string literal (implicit terminating NUL
     * and the remaining elements are zero filled). */
    let seed: Array<UInt8> = "asdffeagewaHAFEFaeDsFEawFdsFaefaeerdjgp".toArray()
    var k: Int64 = 0
    while (k < seed.size) {
        lin[k] = seed[k]
        k = k + 1
    }

    n = 40
    lin[Int64(n) + 1] = 0
    i1 = icrc(0, n, 0, 1)
    lin[Int64(n) + 1] = HIBYTE(i1)
    lin[Int64(n) + 2] = LOBYTE(i1)
    i2 = icrc(i1, n + 2, 0, 1)
    return Int64(i2)
}
