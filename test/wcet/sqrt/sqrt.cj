/* MDH WCET BENCHMARK SUITE. File version $Id: sqrt.c 7801 2018-01-19 13:33:32Z lkg02 $ */

/* 2012/09/28, Jan Gustafsson <jan.gustafsson@mdh.se>
 * Changes:
 *  - This program redefines the C standard function sqrt. Therefore, this function has been renamed to sqrtfcn.
 *  - qrt.c:79:15: warning: explicitly assigning a variable of type 'float' to itself: fixed
 */

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
/*  FILE: sqrt.c                                                         */
/*  SOURCE : Public Domain Code                                          */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     Square root function implemented by Taylor series.                */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/


func fabs(x: Float64): Float64 {
    if (x < 0.0) {
        return -x
    } else {
        return x
    }
}

func sqrtfcn(val: Float64): Float64 {
    var x: Float64 = val / 10.0

    var dx: Float64 = 0.0

    var diff: Float64 = 0.0
    let min_tol: Float64 = 0.00001

    var i: Int64 = 0
    var flag: Int64 = 0

    flag = 0
    if (val == 0.0) {
        x = 0.0
    } else {
        i = 1
        while (i < 20) {
            if (flag == 0) {
                dx = (val - (x * x)) / (2.0 * x)
                x = x + dx
                diff = val - (x * x)
                if (fabs(diff) <= min_tol) {
                    flag = 1
                }
            } else {
                /* JG */
            }
            i = i + 1
        }
    }
    return x
}

main(): Int64 {
    var r: Float64 = fabs(sqrtfcn(2.0))
    return Int64(r)
}
