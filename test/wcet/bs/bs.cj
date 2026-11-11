struct Data {
    var key: Int64
    var value: Int64

    public init(key: Int64, value: Int64) {
        this.key = key
        this.value = value
    }
}

let data: Array<Data> = [
    Data(1, 100),
    Data(5, 200),
    Data(6, 300),
    Data(7, 700),
    Data(8, 900),
    Data(9, 250),
    Data(10, 400),
    Data(11, 600),
    Data(12, 800),
    Data(13, 1500),
    Data(14, 1200),
    Data(15, 110),
    Data(16, 140),
    Data(17, 133),
    Data(18, 10)
]

func binarySearch(x: Int64): Int64 {
    var fvalue: Int64 = -1
    var mid: Int64 = 0
    var up: Int64 = 14
    var low: Int64 = 0

    while (low <= up) {
        // @range-cat: branch-bound - only call site binarySearch(8) fixes x to 8 for every iteration
        sassert(x == 8)
        mid = (low + up) >> 1

        if (data[mid].key == x) {
            up = low - 1
            fvalue = data[mid].value
        } else if (data[mid].key > x) {
            up = mid - 1
        } else {
            low = mid + 1
        }
    }

    // @range-cat: arith-bound - fvalue is initialised to -1 and only ever overwritten by a table value
    sassert(fvalue >= -1)
    // @range-cat: arith-bound - the largest constant stored in the table is 1500
    sassert(fvalue <= 1500)
    // @range-cat: loop-bound - up only decreases in the code
    sassert(up <= 14)
     // @range-cat: loop-bound - low only increases in the code
    sassert(low >= 0)
    
    return fvalue
}

main() {
    binarySearch(8)
}