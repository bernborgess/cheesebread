/*
 * WCET benchmark "select": Cangjie translation of select.c.
 *
 * Selects the k-th largest number out of the n numbers stored in arr[].
 * Translated from the MDH WCET Benchmark Suite (SNU-RT) version of the
 * Numerical Recipes in C routine.
 *
 * The C SWAP(a, b) macro is expanded inline through the local `temp'
 * variable, keeping the original statement order.
 */

var arr: Array<Float64> = [
    5.0, 4.0, 10.3, 1.1, 5.7, 100.0, 231.0, 111.0, 49.5, 99.0,
    10.0, 150.0, 222.22, 101.0, 77.0, 44.0, 35.0, 20.54, 99.99, 888.88
]

func select(k: Int64, n: Int64): Float64 {
    var i: Int64
    var ir: Int64
    var j: Int64
    var l: Int64
    var mid: Int64
    var a: Float64
    var temp: Float64
    var flag: Bool
    var flag2: Bool

    l = 0
    ir = n - 1
    flag = false
    while (!flag) {
        if (ir <= l + 1) {
            if (ir == l + 1) {
                if (arr[ir] < arr[l]) {
                    temp = arr[l]
                    arr[l] = arr[ir]
                    arr[ir] = temp
                }
            }
            flag = true
        } else if (!flag) {
            mid = (l + ir) >> 1
            temp = arr[mid]
            arr[mid] = arr[l + 1]
            arr[l + 1] = temp
            if (arr[l + 1] > arr[ir]) {
                temp = arr[l + 1]
                arr[l + 1] = arr[ir]
                arr[ir] = temp
            }
            if (arr[l] > arr[ir]) {
                temp = arr[l]
                arr[l] = arr[ir]
                arr[ir] = temp
            }
            if (arr[l + 1] > arr[l]) {
                temp = arr[l + 1]
                arr[l + 1] = arr[l]
                arr[l] = temp
            }
            i = l + 1
            j = ir
            a = arr[l]
            flag2 = false
            while (!flag2) {
                i++
                while (arr[i] < a) {
                    i++
                }
                j--
                while (arr[j] > a) {
                    j--
                }
                if (j < i) {
                    flag2 = true
                }
                if (!flag2) {
                    temp = arr[i]
                    arr[i] = arr[j]
                    arr[j] = temp
                }
            }
            arr[l] = arr[j]
            arr[j] = a
            if (j >= k) {
                ir = j - 1
            }
            if (j <= k) {
                l = i
            }
        }
    }
    return arr[k]
}

main(): Int64 {
    select(10, 20)

    return 0
}
