main() {
    var arr: Array<Int64> = [42, 64];
    var i: Int64 = 0;

    if (arr[i] != 42) {
        println("dead");
        i = i + 1;
    }
    sassert(i >= 0)
}
