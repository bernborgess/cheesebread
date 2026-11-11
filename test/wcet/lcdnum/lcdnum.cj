/* MDH WCET BENCHMARK SUITE. File version $Id: lcdnum.c 7801 2018-01-19 13:33:32Z lkg02 $ */

/* 2012/09/28, Jan Gustafsson <jan.gustafsson@mdh.se>
 * Changes:
 *  - The volatile variable n controls a loop, which is not correct. The loop will not terminate. Fixed.
 */

/***********************************************************************
 *  FILE: synthetic.c
 *
 *  PURPOSE: demonstrate effect of flow facts for straight loops
 *
 *  IDEA: reading from an in port mapped to a ten-item buffer,
 *        send first five characters to an LCD as numbers
 *
 ***********************************************************************/

func num_to_lcd(a: UInt8): UInt8 {
    /*   -0-            1            01
     *  1   2         2   4        02  04
     *   -3-    i.e.    8     i.e.   08
     *  4   5        16   32       10  20
     *   -6-           64            40
     *
     */
    // @range-cat: branch-bound - the caller masks the argument with 0x0F before this call
    sassert(a <= 15)
    return match (a) {
        case 0x00 => 0
        case 0x01 => 0x24
        case 0x02 => 1 + 4 + 8 + 16 + 64
        case 0x03 => 1 + 4 + 8 + 32 + 64
        case 0x04 => 2 + 4 + 8 + 32
        case 0x05 => 1 + 4 + 8 + 16 + 64
        case 0x06 => 1 + 2 + 8 + 16 + 32 + 64
        case 0x07 => 1 + 4 + 32
        case 0x08 => 0x7F /* light all */
        case 0x09 => 0x0F + 32 + 64
        case 0x0A => 0x0F + 16 + 32
        case 0x0B => 2 + 8 + 16 + 32 + 64
        case 0x0C => 1 + 2 + 16 + 64
        case 0x0D => 4 + 8 + 16 + 32 + 64
        case 0x0E => 1 + 2 + 8 + 16 + 64
        case 0x0F => 1 + 2 + 8 + 16
        case _ => 0
    }
}

var IN: UInt8 = 0
var OUT: UInt8 = 0

main(): Int64 {
    var i: Int64 = 0
    var a: UInt8 = 0
    /*volatile*/ var n: Int64 = 0 /* JG */

    n = 10
    i = 0
    // @range-cat: branch-bound - n is assigned the constant 10 just above
    sassert(n == 10)
    while (i < n) {
        // @range-cat: loop-bound - the guard i < n = 10 keeps i at most 9
        sassert(i <= 9)
        a = IN /* scan port */
        if (i < 5) {
            // @range-cat: branch-bound - the true branch of i < 5 caps i at 4
            sassert(i <= 4)
            a = a & 0x0F
            // @range-cat: arith-bound - masking with 0x0F caps the value at 15
            sassert(a <= 15)
            OUT = num_to_lcd(a)
        }
        i = i + 1
    }
    // @range-cat: loop-bound - the loop exits once i reaches n = 10
    sassert(i <= 10)
    return 0
}
