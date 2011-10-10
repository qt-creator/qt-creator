import Qt 4.7

Rectangle {
    function foo(k) {
        // k is a unknown value
        var s = ""
        var n = 0
        var N = null
        var u = undefined
        var b = true
        var o = {}

        if (s == s) {}
        if (s == n) {} // 126 15 16
        if (s == N) {} // ### should warn: always false
        if (s == u) {} // ### should warn: always false
        if (s == b) {} // 126 15 16
        if (s == o) {} // 126 15 16
        if (s == k) {} // 126 15 16

        if (n == s) {} // 126 15 16
        if (n == n) {}
        if (n == N) {} // ### should warn: always false
        if (n == u) {} // ### should warn: always false
        if (n == b) {} // 126 15 16
        if (n == o) {} // 126 15 16
        if (n == k) {} // 126 15 16

        if (N == s) {} // ### should warn: always false
        if (N == n) {} // ### should warn: always false
        if (N == N) {}
        if (N == u) {} // ### should warn: always true
        // ### should warn: always false
        if (N == b) {} // 126 15 16
        if (N == o) {} // ### should warn: always false
        if (N == k) {} // 126 15 16

        if (u == s) {} // ### should warn: always false
        if (u == n) {} // ### should warn: always false
        if (u == N) {} // ### should warn: always true
        if (u == u) {} // ### should warn: always true
        if (u == b) {} // ### should warn: always false
        if (u == o) {} // ### should warn: always false
        if (u == k) {} // 126 15 16

        if (b == s) {} // 126 15 16
        if (b == n) {} // 126 15 16
        // ### should warn: always false
        if (b == N) {} // 126 15 16
        if (b == u) {} // ### should warn: always false
        if (b == b) {}
        if (b == o) {} // 126 15 16
        if (b == k) {} // 126 15 16

        if (o == s) {} // 126 15 16
        if (o == n) {} // 126 15 16
        if (o == N) {} // ### should warn: always false
        if (o == u) {} // ### should warn: always false
        if (o == b) {} // 126 15 16
        if (o == o) {}
        if (o == k) {} // 126 15 16

        if (k == s) {} // 126 15 16
        if (k == n) {} // 126 15 16
        if (k == N) {} // 126 15 16
        if (k == u) {} // 126 15 16
        if (k == b) {} // 126 15 16
        if (k == o) {} // 126 15 16
        if (k == k) {} // 126 15 16
    }
}
