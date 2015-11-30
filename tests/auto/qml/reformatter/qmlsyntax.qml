// imports
import QtQuick 2.0
import Qt.labs.particles 1.0 as Part
import "/usr" as Foo
import "." 1.0

Text {
    // properties
    property int foo
    property alias bar: x
    property list<QtObject> pro
    default property int def
    property var baz: Rectangle {
        width: 20
    }

    // script binding
    x: x + y

    // object bindings
    Rectangle on font.family {
        x: x
    }
    anchors.bottom: AnchorAnimation {
        running: true
    }

    // array binding
    states: [
        State {
            name: x
        },
        State {
            name: y
        }
    ]

    // nested with qualified id
    Part.ParticleMotion {
        id: foo
    }

    // functions
    function foo(a, b) {}
    function foo(a, b) {
        x = a + 12 * b
    }
}
