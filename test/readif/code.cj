import std.env.getStdIn
import std.convert.*

func getInt(): Int64 {
    Int64.parse(getStdIn().readln().getOrDefault({=> ""}))
}

main() {
    var x = getInt();
    if (x == 42) {
        println("x=42");
    }
}
