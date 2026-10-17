/* MDH WCET BENCHMARK SUITE. File version $Id: qsort-exam.c 7801 2018-01-19 13:33:32Z lkg02 $ */

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
/*  FILE: qsort-exam.c                                                   */
/*  SOURCE : Numerical Recipes in C - The Second Edition                 */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     Non-recursive version of quick sort algorithm.                    */
/*     This example sorts 20 floating point numbers, arr[].              */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/
/* Changes:
 * JG 2005/12/14: sort(20) changed to sort(19) since sort(20) implied reading
 * outside array limits in sort. This error was discovered using the SWEET tool.
 */

/* The C macros SWAP, M and NSTACK map to Cangjie declarations below.
 * SWAP is inlined at each use site because it assigns the enclosing
 * function's local variable `temp`.
 */
let M: Int64 = 7
let NSTACK: Int64 = 50

var arr: Array<Float64> = [
    5.0, 4.0, 10.3, 1.1, 5.7, 100.0, 231.0, 111.0, 49.5, 99.0,
    10.0, 150.0, 222.22, 101.0, 77.0, 44.0, 35.0, 20.54, 99.99, 88.88
]

var istack: Array<Int64> = Array<Int64>(100, repeat: 0)

func sort(n: Int64) {
    var i: Int64 = 0
    var ir: Int64 = n
    var j: Int64 = 0
    var k: Int64 = 0
    var l: Int64 = 1
    var jstack: Int64 = 0
    var flag: Int64 = 0
    var a: Float64 = 0.0
    var temp: Float64 = 0.0

    flag = 0
    while (true) {
        if (ir - l < M) {
            j = l + 1
            while (j <= ir) {
                a = arr[j]
                i = j - 1
                while (i >= l) {
                    if (arr[i] <= a) {
                        break
                    }
                    arr[i + 1] = arr[i]
                    i = i - 1
                }
                arr[i + 1] = a
                j = j + 1
            }
            if (jstack == 0) {
                break
            }
            ir = istack[jstack]
            jstack = jstack - 1
            l = istack[jstack]
            jstack = jstack - 1
        } else {
            k = (l + ir) >> 1
            temp = arr[k]
            arr[k] = arr[l + 1]
            arr[l + 1] = temp
            if (arr[l] > arr[ir]) {
                temp = arr[l]
                arr[l] = arr[ir]
                arr[ir] = temp
            }
            if (arr[l + 1] > arr[ir]) {
                temp = arr[l + 1]
                arr[l + 1] = arr[ir]
                arr[ir] = temp
            }
            if (arr[l] > arr[l + 1]) {
                temp = arr[l]
                arr[l] = arr[l + 1]
                arr[l + 1] = temp
            }
            i = l + 1
            j = ir
            a = arr[l + 1]
            while (true) {
                i = i + 1
                while (arr[i] < a) {
                    i = i + 1
                }
                j = j - 1
                while (arr[j] > a) {
                    j = j - 1
                }
                if (j < i) {
                    break
                }
                temp = arr[i]
                arr[i] = arr[j]
                arr[j] = temp
            }
            arr[l + 1] = arr[j]
            arr[j] = a
            jstack = jstack + 2

            if (ir - i + 1 >= j - l) {
                istack[jstack] = ir
                istack[jstack - 1] = i
                ir = j - 1
            } else {
                istack[jstack] = j - 1
                istack[jstack - 1] = l
                l = i
            }
        }
    }
}

main(): Int64 {
    sort(19)
    return 0
}
