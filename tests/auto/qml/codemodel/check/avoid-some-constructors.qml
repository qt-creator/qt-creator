import QtQuick 1.0

Item {
    function foo() {
        var x = new Number(); // 308 21 26
        x = new String(); // 110 17 22
        x = new Array(); // 112 17 21
        x = new Object(); // 111 17 22
        x = new Function(); // 113 17 24
        x = new Boolean() // 109 17 23
        x = new Date()
        x = new Array(6)
    }
}
