/* MDH WCET BENCHMARK SUITE. File version $Id: cnt.c 7800 2018-01-19 12:43:12Z lkg02 $ */

/* #define WORSTCASE 1 */
/* #define MAXSIZE 100 Changed JG/Ebbe */
let MAXSIZE: Int64 = 10

/* Globals */
var Seed: Int64 = 0
var array: Array<Array<Int64>> = Array<Array<Int64>>(MAXSIZE, repeat: Array<Int64>(MAXSIZE, repeat: 0))
var Postotal: Int64 = 0
var Negtotal: Int64 = 0
var Poscnt: Int64 = 0
var Negcnt: Int64 = 0

/* The main function */
main() {
    InitSeed()
    Test(array)
}

func Test(array: Array<Array<Int64>>): Int64 {
    var StartTime: Int64 = 0
    var StopTime: Int64 = 0
    var TotalTime: Float64 = 0.0

    Initialize(array)
    StartTime = 1000
    Sum(array)
    StopTime = 1500

    TotalTime = Float64(StopTime - StartTime) / 1000.0

    return 0
}

/* Intializes the given array with random integers. */
func Initialize(array: Array<Array<Int64>>): Int64 {
    for (OuterIndex in 0..MAXSIZE) {
        for (InnerIndex in 0..MAXSIZE) {
            array[OuterIndex][InnerIndex] = RandomInteger()
        }
    }

    return 0
}

/* Initializes the seed used in the random number generator. */
func InitSeed(): Int64 {
    Seed = 0
    return 0
}

func Sum(array: Array<Array<Int64>>) {
    var Ptotal: Int64 = 0
    var Ntotal: Int64 = 0
    var Pcnt: Int64 = 0
    var Ncnt: Int64 = 0

    for (Outer in 0..MAXSIZE) {
        for (Inner in 0..MAXSIZE) {
            if (array[Outer][Inner] < 0) {
                Ptotal += array[Outer][Inner]
                Pcnt += 1
            } else {
                Ntotal += array[Outer][Inner]
                Ncnt += 1
            }
        }
    }

    Postotal = Ptotal
    Poscnt = Pcnt
    Negtotal = Ntotal
    Negcnt = Ncnt
}

/* Generates random integers between 0 and 8095 */
func RandomInteger(): Int64 {
    Seed = ((Seed * 133) + 81) % 8095
    return Seed
}