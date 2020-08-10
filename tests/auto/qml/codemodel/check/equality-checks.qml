import QtQuick 1.0

Rectangle {
    function foo(k) {
        // k is a unknown value
        var s = ""
        var n = 0
        var N = null
        var u = undefined
        var b = true
        var o = {}

        if (s == s) {} // -1 15 16 # false positive
        if (s == n) {} // 126 15 16
        if (s == N) {} // -2 15 16 # wrong warning (always false)
        if (s == u) {} // -2 15 16 # wrong warning (always false)
        if (s == b) {} // 126 15 16
        if (s == o) {} // 126 15 16
        if (s == k) {} // 126 15 16

        if (n == s) {} // 126 15 16
        if (n == n) {} // -1 15 16 # false positive
        if (n == N) {} // -2 15 16 # wrong warning (always false)
        if (n == u) {} // -2 15 16 # wrong warning (always false)
        if (n == b) {} // 126 15 16
        if (n == o) {} // 126 15 16
        if (n == k) {} // 126 15 16

        if (N == s) {} // -2 15 16 # wrong warning (always false)
        if (N == n) {} // -2 15 16 # wrong warning (always false)
        if (N == N) {} // -1 15 16 # false positive
        if (N == u) {} // -2 15 16 # wrong warning (always true)

        if (N == b) {} // -2 15 16 # wrong warning (always false)
        if (N == o) {} // -2 15 16 # wrong warning (always false)
        if (N == k) {} // 126 15 16

        if (u == s) {} // -2 15 16 # wrong warning (always false)
        if (u == n) {} // -2 15 16 # wrong warning (always false)
        if (u == N) {} // -2 15 16 # wrong warning (always true)
        if (u == u) {} // -2 15 16 # wrong warning (always true)
        if (u == b) {} // -2 15 16 # wrong warning (always false)
        if (u == o) {} // -2 15 16 # wrong warning (always false)
        if (u == k) {} // 126 15 16

        if (b == s) {} // 126 15 16
        if (b == n) {} // 126 15 16

        if (b == N) {} // -2 15 16 # wrong warning (always false)
        if (b == u) {} // -2 15 16 # wrong warning (always false)
        if (b == b) {} // -1 15 16 # false positive
        if (b == o) {} // 126 15 16
        if (b == k) {} // 126 15 16

        if (o == s) {} // 126 15 16
        if (o == n) {} // 126 15 16
        if (o == N) {} // -2 15 16 # wrong warning (always false)
        if (o == u) {} // -2 15 16 # wrong warning (always false)
        if (o == b) {} // 126 15 16
        if (o == o) {} // -1 15 16 # false positive
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
