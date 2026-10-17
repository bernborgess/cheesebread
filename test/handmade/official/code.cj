package llvm_style_public_example

public func mask(v: Int64): Int64 {
    v & 7
}

func foo(): Int64 {
    var p: Int64 = 0
    for (k in 0..=2) {
        var x: Int64 = 0
        if (k == 0) {
            x = 13
        } else {
            if (k == 1) {
                x = 24
            } else {
                x = 40
            }
        }
        var b: Int64 = mask(x)
        p = p + b + k
    }
    return p
}

main() { foo() }