import Qt 4.7

Item {
    // DEFAULTMSG 'new' should only be used with functions that start with an uppercase letter
    function foo() {
        a = new A
        a = new A()
        a = new a // W 17 17
        a = new a() // W 17 17
    }

    // DEFAULTMSG calls of functions that start with an uppercase letter should use 'new'
    function foo() {
        a = A() // W 13 13
        a = a()
    }
}
