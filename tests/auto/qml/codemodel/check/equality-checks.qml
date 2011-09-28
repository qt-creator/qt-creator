import Qt 4.7

Rectangle {
    onXChanged: {
        if (0 == undefined) {} // 126 15 16
    }

    function foo() {
        var s = ""
        var n = 0
        var N = null
        var u = undefined
        var b = true
        var o = {}

        if (s == s) {}
        if (s == n) {} // 126 15 16
        if (s == N) {} // ### should warn: always false
        if (s == u) {} // 126 15 16
        if (s == b) {} // 126 15 16
        if (s == o) {} // 126 15 16

        if (n == s) {} // 126 15 16
        if (n == n) {}
        if (n == N) {} // ### should warn: always false
        if (n == u) {} // 126 15 16
        if (n == b) {} // 126 15 16
        if (n == o) {} // 126 15 16

        if (N == s) {} // ### should warn: always false
        if (N == n) {} // ### should warn: always false
        if (N == N) {}
        if (N == u) {} // 126 15 16
        // ### should warn: always false
        if (N == b) {} // 126 15 16
        if (N == o) {} // ### should warn: always false

        if (u == s) {} // 126 15 16
        if (u == n) {} // 126 15 16
        if (u == N) {} // 126 15 16
        if (u == u) {} // 126 15 16
        if (u == b) {} // 126 15 16
        if (u == o) {} // 126 15 16

        if (b == s) {} // 126 15 16
        if (b == n) {} // 126 15 16
        // ### should warn: always false
        if (b == N) {} // 126 15 16
        if (b == u) {} // 126 15 16
        if (b == b) {}
        if (b == o) {} // 126 15 16

        if (o == s) {} // 126 15 16
        if (o == n) {} // 126 15 16
        if (o == N) {} // ### should warn: always false
        if (o == u) {} // 126 15 16
        if (o == b) {} // 126 15 16
        if (o == o) {}
    }
}
