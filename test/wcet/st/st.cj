/* MDH WCET BENCHMARK SUITE. File version $Id: st.c 7801 2018-01-19 13:33:32Z lkg02 $ */

/* 2012/09/28, Jan Gustafsson <jan.gustafsson@mdh.se>
 * Changes:
 *  - time is only enabled if the POUT flag is set
 *  - st.c:30:1:  main () warning: type specifier missing, defaults to 'int': fixed
 */

/* 2011/10/18, Benedikt Huber <benedikt@vmars.tuwien.ac.at>
 * Changes:
 *  - Measurement and Printing the Results is only enabled if the POUT flag is set
 *  - Added Prototypes for InitSeed and RandomInteger
 *  - Changed return type of InitSeed from 'missing (default int)' to 'void'
 */

/* Statistics Program:
 * This program computes for two arrays of numbers the sum, the
 * mean, the variance, and standard deviation.  It then determines the
 * correlation coefficient between the two arrays.
 */

let MAX: Int64 = 1000

var Seed: Int64 = 0
var ArrayA: Array<Float64> = Array<Float64>(MAX, repeat: 0.0)
var ArrayB: Array<Float64> = Array<Float64>(MAX, repeat: 0.0)
var SumA: Float64 = 0.0
var SumB: Float64 = 0.0
var Coef: Float64 = 0.0

func InitSeed() {
    /*
     * Initializes the seed used in the random number generator.
     */
    Seed = 0
}

func RandomInteger(): Int64 {
    /*
     * Generates random integers between 0 and 8095
     */
    Seed = ((Seed * 133) + 81) % 8095
    return Seed
}

func Square(x: Float64): Float64 {
    return x * x
}

func Initialize(Array: Array<Float64>) {
    /*
     * Intializes the given array with random integers.
     */
    var i: Int64 = 0

    i = 0
    while (i < MAX) {
        Array[i] = Float64(i) + Float64(RandomInteger()) / 8095.0
        i = i + 1
    }
}

func Calc_Sum_Mean(Array: Array<Float64>): (Float64, Float64) {
    var i: Int64 = 0
    var Sum: Float64 = 0.0
    var Mean: Float64 = 0.0

    Sum = 0.0
    i = 0
    while (i < MAX) {
        Sum = Sum + Array[i]
        i = i + 1
    }
    Mean = Sum / Float64(MAX)
    return (Sum, Mean)
}

func Calc_Var_Stddev(Array: Array<Float64>, Mean: Float64): (Float64, Float64) {
    var i: Int64 = 0
    var Var: Float64 = 0.0
    var Stddev: Float64 = 0.0
    var diffs: Float64 = 0.0

    diffs = 0.0
    i = 0
    while (i < MAX) {
        diffs = diffs + Square(Array[i] - Mean)
        i = i + 1
    }
    Var = diffs / Float64(MAX)
    Stddev = sqrt(Var)
    return (Var, Stddev)
}

func Calc_LinCorrCoef(ArrayA: Array<Float64>, ArrayB: Array<Float64>, MeanA: Float64, MeanB: Float64) {
    var i: Int64 = 0
    var numerator: Float64 = 0.0
    var Aterm: Float64 = 0.0
    var Bterm: Float64 = 0.0

    numerator = 0.0
    Aterm = 0.0
    Bterm = 0.0
    i = 0
    while (i < MAX) {
        numerator = numerator + (ArrayA[i] - MeanA) * (ArrayB[i] - MeanB)
        Aterm = Aterm + Square(ArrayA[i] - MeanA)
        Bterm = Bterm + Square(ArrayB[i] - MeanB)
        i = i + 1
    }

    /* Coef used globally */
    Coef = numerator / (sqrt(Aterm) * sqrt(Bterm))
}

func sqrt(x: Float64): Float64 {
    var y: Float64 = x
    var z: Float64 = 1.0
    var delta: Float64 = 0.0
    var i: Int64 = 0

    if (x <= 0.0) {
        return 0.0
    }
    i = 0
    while (i < 100) {
        delta = (y * y - x) / (2.0 * y)
        y = y - delta
        z = delta
        if (z < 0.0) {
            z = -z
        }
        if (z < 0.0000001) {
            break
        }
        i = i + 1
    }
    return y
}

main(): Int64 {
    var MeanA: Float64 = 0.0
    var MeanB: Float64 = 0.0
    var VarA: Float64 = 0.0
    var VarB: Float64 = 0.0
    var StddevA: Float64 = 0.0
    var StddevB: Float64 = 0.0

    InitSeed()

    Initialize(ArrayA)
    let (sumA, meanA) = Calc_Sum_Mean(ArrayA)
    SumA = sumA
    MeanA = meanA
    let (varA, stddevA) = Calc_Var_Stddev(ArrayA, MeanA)
    VarA = varA
    StddevA = stddevA

    Initialize(ArrayB)
    let (sumB, meanB) = Calc_Sum_Mean(ArrayB)
    SumB = sumB
    MeanB = meanB
    let (varB, stddevB) = Calc_Var_Stddev(ArrayB, MeanB)
    VarB = varB
    StddevB = stddevB

    /* Coef will have to be used globally in Calc_LinCorrCoef since it would
       be beyond the 6 registers used for passing parameters
    */
    Calc_LinCorrCoef(ArrayA, ArrayB, MeanA, MeanB)

    return 0
}
