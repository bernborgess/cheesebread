/* MDH WCET BENCHMARK SUITE. File version $Id: matmult.c 7800 2018-01-19 12:43:12Z lkg02 $ */

/* matmult.c */
/* was mm.c! */


/*----------------------------------------------------------------------*
 * To make this program compile under our assumed embedded environment,
 * we had to make several changes:
 * - Declare all functions in ANSI style, not K&R.
 *   this includes adding return types in all cases!
 * - Declare function prototypes
 * - Disable all output
 * - Disable all UNIX-style includes
 *
 * This is a program that was developed from mm.c to matmult.c by
 * Thomas Lundqvist at Chalmers.
 *----------------------------------------------------------------------*/

/*
 * MATRIX MULTIPLICATION BENCHMARK PROGRAM:
 * This program multiplies 2 square matrices resulting in a 3rd
 * matrix. It tests a compiler's speed in handling multidimensional
 * arrays and simple arithmetic.
 */

/* #define UPPERLIMIT 20 */
let UPPERLIMIT: Int64 = 20

/* typedef int matrix [UPPERLIMIT][UPPERLIMIT]; */

/* Globals */
var Seed: Int64 = 0
var ArrayA: Array<Array<Int64>> = Array<Array<Int64>>(UPPERLIMIT, {_ => Array<Int64>(UPPERLIMIT, repeat: 0)})
var ArrayB: Array<Array<Int64>> = Array<Array<Int64>>(UPPERLIMIT, {_ => Array<Int64>(UPPERLIMIT, repeat: 0)})
var ResultArray: Array<Array<Int64>> = Array<Array<Int64>>(UPPERLIMIT, {_ => Array<Int64>(UPPERLIMIT, repeat: 0)})

main(): Int64 {
    InitSeed()
    /* ***UPPSALA WCET***:
       no printing please! */
    Test(ArrayA, ArrayB, ResultArray)
    return 0
}

/*
 * Initializes the seed used in the random number generator.
 */
func InitSeed(): Unit {
    /* ***UPPSALA WCET***:
       changed Thomas Ls code to something simpler.
     Seed = KNOWN_VALUE - 1; */
    Seed = 0
}

/*
 * Runs a multiplication test on an array.  Calculates and prints the
 * time it takes to multiply the matrices.
 */
func Test(A: Array<Array<Int64>>, B: Array<Array<Int64>>, Res: Array<Array<Int64>>): Unit {
    Initialize(A)
    Initialize(B)

    /* ***UPPSALA WCET***: don't print or time */

    Multiply(A, B, Res)
}

/*
 * Intializes the given array with random integers.
 */
func Initialize(arr: Array<Array<Int64>>): Unit {
    for (OuterIndex in 0..UPPERLIMIT) {
        for (InnerIndex in 0..UPPERLIMIT) {
            arr[OuterIndex][InnerIndex] = RandomInteger()
        }
    }
}

/*
 * Generates random integers between 0 and 8095
 */
func RandomInteger(): Int64 {
    Seed = ((Seed * 133) + 81) % 8095
    return Seed
}

/*
 * Multiplies arrays A and B and stores the result in ResultArray.
 */
func Multiply(A: Array<Array<Int64>>, B: Array<Array<Int64>>, Res: Array<Array<Int64>>): Unit {
    for (Outer in 0..UPPERLIMIT) {
        for (Inner in 0..UPPERLIMIT) {
            Res[Outer][Inner] = 0
            for (Index in 0..UPPERLIMIT) {
                Res[Outer][Inner] += A[Outer][Index] * B[Index][Inner]
            }
        }
    }
}
