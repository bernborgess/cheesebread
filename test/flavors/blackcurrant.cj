// Policy Iterations
var rand_state : Int64 = 0
func rand() {
    rand_state += 1
    rand_state %= 10
    return rand_state
}

main() {
    var i = 0;
    while (true) {
        if (rand() != 0) {
            i += 1;
            if (i >= 10) {
                i = 0
            }
        }
        sassert(i <= 9)
    }
}
