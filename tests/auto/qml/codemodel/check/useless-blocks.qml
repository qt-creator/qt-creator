import QtQuick 2.0

Rectangle {
    function foo() {
        { // 115 9 9
            var x = 10
        }
        {
            let x = 10
        }

        if (a) {}
    }

    onXChanged: {
        { // 115 9 9
            var y = 11
        }
        {
            let y = 11
        }

        while (A) {}
    }

    property int d: {
        { // 115 9 9
            var z = 12
        }
        {
            let y = 12
        }
    }
}
