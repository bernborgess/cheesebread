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

    return fvalue
}

main() {
    binarySearch(8)
}
