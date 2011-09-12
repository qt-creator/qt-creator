import Qt 4.7
// DEFAULTMSG == and != perform type coercion, use === or !== instead to avoid
Rectangle {
    onXChanged: {
        if (0 == undefined) {} // W 15 16
    }

    function foo() {
        var s = ""
        var n = 0
        var N = null
        var u = undefined
        var b = true
        var o = {}

        if (s == s) {}
        if (s == n) {} // W 15 16
        if (s == N) {} // ### should warn: always false
        if (s == u) {} // W 15 16
        if (s == b) {} // W 15 16
        if (s == o) {} // W 15 16

        if (n == s) {} // W 15 16
        if (n == n) {}
        if (n == N) {} // ### should warn: always false
        if (n == u) {} // W 15 16
        if (n == b) {} // W 15 16
        if (n == o) {} // W 15 16

        if (N == s) {} // ### should warn: always false
        if (N == n) {} // ### should warn: always false
        if (N == N) {}
        if (N == u) {} // W 15 16
        // ### should warn: always false
        if (N == b) {} // W 15 16
        if (N == o) {} // ### should warn: always false

        if (u == s) {} // W 15 16
        if (u == n) {} // W 15 16
        if (u == N) {} // W 15 16
        if (u == u) {} // W 15 16
        if (u == b) {} // W 15 16
        if (u == o) {} // W 15 16

        if (b == s) {} // W 15 16
        if (b == n) {} // W 15 16
        // ### should warn: always false
        if (b == N) {} // W 15 16
        if (b == u) {} // W 15 16
        if (b == b) {}
        if (b == o) {} // W 15 16

        if (o == s) {} // W 15 16
        if (o == n) {} // W 15 16
        if (o == N) {} // ### should warn: always false
        if (o == u) {} // W 15 16
        if (o == b) {} // W 15 16
        if (o == o) {}
    }
}
