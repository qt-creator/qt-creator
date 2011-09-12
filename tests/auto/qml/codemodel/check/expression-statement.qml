import Qt 4.7
// DEFAULTMSG expression statements should be assignments, calls or delete expressions only
Rectangle {
    function foo() {
        a // W 9 9
        a + b // W 9 13
        a()
        delete b
        a = 12
        a += 12
        d().foo // W 9 15
    }
}
