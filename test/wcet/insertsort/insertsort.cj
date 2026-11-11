/* MDH WCET BENCHMARK SUITE. File version $Id: insertsort.c 7800 2018-01-19 12:43:12Z lkg02 $ */

/*************************************************************************/
/*                                                                       */
/*   SNU-RT Benchmark Suite for Worst Case Timing Analysis               */
/*   =====================================================               */
/*                              Collected and Modified by S.-S. Lim      */
/*                                           sslim@archi.snu.ac.kr       */
/*                                         Real-Time Research Group      */
/*                                        Seoul National University      */
/*                                                                       */
/*************************************************************************/
/*                                                                       */
/*  FILE: insertsort.c                                                   */
/*  SOURCE : Public Domain Code                                          */
/*                                                                       */
/*  DESCRIPTION :                                                        */
/*                                                                       */
/*     Insertion sort for 10 integer numbers.                            */
/*     The integer array a[] is initialized in main function.            */
/*                                                                       */
/*  REMARK :                                                             */
/*                                                                       */
/*  EXECUTION TIME :                                                     */
/*                                                                       */
/*                                                                       */
/*************************************************************************/



var a: Array<UInt32> = Array<UInt32>(11, repeat: 0)

main() {
    var i: Int64 = 0
    var j: Int64 = 0
    var temp: UInt32 = 0

    a[0] = 0   /* assume all data is positive */
    a[1] = 11; a[2] = 10; a[3] = 9; a[4] = 8; a[5] = 7; a[6] = 6; a[7] = 5
    a[8] = 4; a[9] = 3; a[10] = 2
    i = 2
    while (i <= 10) {
        // @range-cat: loop-bound - the outer index never exceeds the loop guard bound 10
        sassert(i <= 10)
        j = i
        while (a[j] < a[j - 1]) {
            // @range-cat: array-bound - j starts at the outer index i which is at most 10
            sassert(j <= 10)
            // @range-cat: array-bound - a[0] = 0 is the unique minimum so j never reaches 0
            sassert(j >= 1)
            // @range-cat: branch-bound - the shift guard only holds for j >= 2
            sassert(j >= 2)
            temp = a[j]
            // @range-cat: arith-bound - the array holds the literals 0..11, so temp <= 11
            sassert(temp <= 11)
            a[j] = a[j - 1]
            a[j - 1] = temp
            j = j - 1
        }
        i = i + 1
    }
    // @range-cat: loop-bound - after the loop i has advanced one past 10
    sassert(i <= 11)
    return 1
}