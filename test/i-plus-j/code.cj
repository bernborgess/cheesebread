func foo() {
    var i = 0
    var j = 0
    while (i + j < 10) {
        i++
        j++
    }
    if (i + j != 10) {
        println("dead")
    }
}

main() {
    foo()
}
