
class Dog {
    public func bark(loud!: Bool = false) {
        if (loud) {
            println("WOOF!");
        } else {
            println("woof!");
        }
    }
}

main() {
    let duke = Dog();
    duke.bark(loud:true);
    duke.bark(loud:false);
    duke.bark(); // not loud by default
}