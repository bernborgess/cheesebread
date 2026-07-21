func foo() {
	var i = 0
	while (i < 10) {
		i++
	}
	if (i != 10) {
		println("dead")
	}
}

main() { foo() }
