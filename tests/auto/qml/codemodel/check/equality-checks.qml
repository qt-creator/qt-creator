import Qt 4.7
// DEFAULTMSG == and != perform type coercion, use === or !== instead to avoid
Rectangle {
    onXChanged: {
        if (0 == undefined) {} // W 15 16
    }

    function foo() {
        var st = ""
        var nu = 0
        var un = undefined
        if (st == "") {} // no warning
        if (nu == "") {} // W 16 17
        if (un == "") {} // W 16 17
        if (st == 0) {} // W 16 17
        if (nu == 0) {} // no warning
        if (un == 0) {} // W 16 17
        if (st == null) {} // W 16 17
        if (nu == null) {} // W 16 17
        if (un == null) {} // W 16 17
        if (st == undefined) {} // W 16 17
        if (nu == undefined) {} // W 16 17
        if (un == undefined) {} // W 16 17
    }
}
