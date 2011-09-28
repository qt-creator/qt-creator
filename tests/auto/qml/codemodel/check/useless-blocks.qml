import Qt 4.7

Rectangle {
    function foo() {
        {} // 115 9 9
        if (a) {}
    }

    onXChanged: {
        {} // 115 9 9
        while (A) {}
    }

    property int d: {
        {} // 115 9 9
    }
}
