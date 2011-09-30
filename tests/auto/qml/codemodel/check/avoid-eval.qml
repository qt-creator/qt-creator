import QtQuick 1.0

Item {
    function foo() {
        eval("a + b") // 23 9 12
        var a = { eval: function (string) { return string; } }
        a.eval("a + b")
    }
}
