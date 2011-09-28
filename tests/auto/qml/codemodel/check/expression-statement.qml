import Qt 4.7

Rectangle {
    function foo() {
        a // 127 9 9
        a + b // 127 9 13
        a()
        delete b
        a = 12
        a += 12
        d().foo // 127 9 15
    }
}
