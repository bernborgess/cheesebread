// Relative Completeness
main() {
    var a = Array<Int64>(100, repeat: 0)
    var i = 0;
    while (i < 100) {
        a[i] = 0
        i += 1;
    }

    sassert(a[50] == 0)
}
