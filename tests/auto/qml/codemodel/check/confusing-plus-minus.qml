import QtQuick 1.0

Item {
    function foo() {
        var x, y
        x = 1 + +2 // 117 15 17
        x = 1 + ++x // 117 15 17
        x = x++ + x // 117 15 17
        x = 1 - -2 // 119 15 17
        x = 1 - --x // 119 15 17
        x = x-- - x // 119 15 17
        x = x + --y + y-- + x++ - y
    }
}
