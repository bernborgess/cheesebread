/* MDH WCET BENCHMARK SUITE. File version $Id: bsort100.c 7800 2018-01-19 12:43:12Z lkg02 $ */

let FALSE: Int64 = 0
let TRUE: Int64 = 1
let NUMELEMS: Int64 = 100
let MAXDIM: Int64 = NUMELEMS + 1

var array: Array<Int64> = Array<Int64>(MAXDIM, repeat: 0)
var Seed: Int64 = 0
var factor: Int64 = 0

func Initialize(arr: Array<Int64>) {
    var Index: Int64 = 0
    var fact: Int64 = 0

    factor = -1

    fact = factor
    Index = 1
    while (Index <= NUMELEMS) {
        arr[Index] = Index * fact
        Index = Index + 1
    }
}

func BubbleSort(arr: Array<Int64>) {
    var Sorted: Int64 = FALSE
    var Temp: Int64 = 0
    var Index: Int64 = 0
    var i: Int64 = 0

    i = 1
    while (i <= NUMELEMS - 1) {
        Sorted = TRUE
        Index = 1
        while (Index <= NUMELEMS - 1) {
            if (Index > NUMELEMS - i) {
                break
            }
            if (arr[Index] > arr[Index + 1]) {
                Temp = arr[Index]
                arr[Index] = arr[Index + 1]
                arr[Index + 1] = Temp
                Sorted = FALSE
            }
            Index = Index + 1
        }

        if (Sorted != FALSE) {
            break
        }
        i = i + 1
    }
}

main() {
    Initialize(array)
    BubbleSort(array)
}
