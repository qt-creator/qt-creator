import Qt 4.7
// DEFAULTMSG blocks do not introduce a new scope, avoid
Rectangle {
    function foo() {
        {} // W 9 9
        if (a) {}
    }

    onXChanged: {
        {} // W 9 9
        while (A) {}
    }

    property int d: {
        {} // W 9 9
    }
}
