import QtQuick 2.0
import myplugin 1.0

Rectangle {
    width: 100
    height: 100

    MyType {
        id: time
    }

    Component.onCompleted: {
        // bp here should be hit on startup
        backend.greet("C++");
    }

    Connections {
        target: backend
        onGreetBack: {
            console.log("hello \"" + toWhom + "\"");
        }
    }

    Text {
        anchors.centerIn: parent
        // bp here should be hit every second
        text: time.timeText
    }
}
