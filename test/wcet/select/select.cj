/* MDH WCET BENCHMARK SUITE. Translated to Cangjie. */

let arr: [Float64; 20] = [
    5.0, 4.0, 10.3, 1.1, 5.7, 100.0, 231.0, 111.0, 49.5, 99.0,
    10.0, 150.0, 222.22, 101.0, 77.0, 44.0, 35.0, 20.54, 99.99, 888.88
]

func select(k: UInt64, n: UInt64): Float64 {
    var i: UInt64 = 0
    var ir: UInt64 = 0
    var j: UInt64 = 0
    var l: UInt64 = 0
    var mid: UInt64 = 0
    var a: Float64 = 0.0
    var temp: Float64 = 0.0
    var flag: Bool = false
    var flag2: Bool = false

    l = 0
    ir = n - 1
    flag = false

    while (!flag) {
        if (ir <= l + 1) {
            if (ir == l + 1) {
                if (arr[Int64(ir).toInt()] < arr[Int64(l).toInt()]) {
                    temp = arr[Int64(l).toInt()]
                    arr[Int64(l).toInt()] = arr[Int64(ir).toInt()]
                    arr[Int64(ir).toInt()] = temp
                }
            }
            flag = true
        } else {
            mid = (l + ir) >> 1
            // SWAP(arr[mid], arr[l + 1])
            temp = arr[Int64(mid).toInt()]
            arr[Int64(mid).toInt()] = arr[Int64(l + 1).toInt()]
            arr[Int64(l + 1).toInt()] = temp

            if (arr[Int64(l + 1).toInt()] > arr[Int64(ir).toInt()]) {
                // SWAP(arr[l + 1], arr[ir])
                temp = arr[Int64(l + 1).toInt()]
                arr[Int64(l + 1).toInt()] = arr[Int64(ir).toInt()]
                arr[Int64(ir).toInt()] = temp
            }

            if (arr[Int64(l).toInt()] > arr[Int64(ir).toInt()]) {
                // SWAP(arr[l], arr[ir])
                temp = arr[Int64(l).toInt()]
                arr[Int64(l).toInt()] = arr[Int64(ir).toInt()]
                arr[Int64(ir).toInt()] = temp
            }

            if (arr[Int64(l + 1).toInt()] > arr[Int64(l).toInt()]) {
                // SWAP(arr[l + 1], arr[l])
                temp = arr[Int64(l + 1).toInt()]
                arr[Int64(l + 1).toInt()] = arr[Int64(l).toInt()]
                arr[Int64(l).toInt()] = temp
            }

            i = l + 1
            j = ir
            a = arr[Int64(l).toInt()]
            flag2 = false

            while (!flag2) {
                i = i + 1
                while (arr[Int64(i).toInt()] < a) {
                    i = i + 1
                }
                j = j - 1
                while (arr[Int64(j).toInt()] > a) {
                    j = j - 1
                }
                if (j < i) {
                    flag2 = true
                }
                if (!flag2) {
                    // SWAP(arr[i], arr[j])
                    temp = arr[Int64(i).toInt()]
                    arr[Int64(i).toInt()] = arr[Int64(j).toInt()]
                    arr[Int64(j).toInt()] = temp
                }
            }

            arr[Int64(l).toInt()] = arr[Int64(j).toInt()]
            arr[Int64(j).toInt()] = a

            if (j >= k) {
                ir = j - 1
            }
            if (j <= k) {
                l = i
            }
        }
    }

    return arr[Int64(k).toInt()]
}

func main() {
    select(10, 20)
}