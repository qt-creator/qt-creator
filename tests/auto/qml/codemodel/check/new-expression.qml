import Qt 4.7

Item {
    function foo() {
        a = new A
        a = new A()
        a = new a // 307 17 17
        a = new a() // 307 17 17
    }

    function foo() {
        a = A() // 306 13 13
        a = a()
        a = Number("abc")
        a = String(12)
        a = Boolean(12)
        a = Date()
    }
}
