// Relational Polyhedra
func foo(x : Int64) {
    var y = 0
    if (x >= 0) {
        y = x
    } else {
        y = -x
    }
    if (y >= 1) {
        sassert(x != 0)
    }
}

main() {
    foo(-2)
    foo(2)
}
