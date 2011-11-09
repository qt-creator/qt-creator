import Qt 4.7

Rectangle {
    function foo() {
        a + b // 127 9 13
        // @disable-check M127
        a + b
        a + b // @disable-check M127

        // @disable-check M127 31 12 30

        // @disable-check M126 31 12 30
        a + b // 127 9 13
    }
}
