import std.env.getStdIn
import std.convert.*

func getInt(): Int64 {
    Int64.parse(getStdIn().readln().getOrDefault({=> ""}))
}

func calc(n: Int64): Int64 {
    if (n < 1) {
        return 0; // n = [-inf, 0:1]
    }
    if (n == 42) {
        return 1; // n = [42, 42:1]
    }
    return 2; // n = [1, +inf:1]
}

main() {
    var x = getInt();
    var y = calc(x); // y = 0, 1, 2
    println(y);
}
