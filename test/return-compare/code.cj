func calc(n: Int64): Int64 {
    if (n < 1) {
        return 0;
    }
    return n;
}

main() {
    var y = calc(10);
    if (y != 10) {
        println("dead as a dino")
    }
}
