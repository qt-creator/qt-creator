import Qt 4.7

Rectangle {
    function foo() {
        a + b // 127 9 13
        // @disable M127
        a + b
        a + b // @disable M127

        // @disable M127 31 12 24

        // @disable M126 31 12 24
        a + b // 127 9 13
    }
}
