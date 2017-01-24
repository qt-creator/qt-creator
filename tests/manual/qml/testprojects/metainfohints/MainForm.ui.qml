import QtQuick 2.8
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import Qt.labs.calendar 1.0
import test 1.0

Item {
    property alias mouseArea: mouseArea
    property alias textEdit: textEdit

    width: 360
    height: 360
    TestComponent {
    }

    ForceClip {
    }

    Rectangle {
        id: rectangle
        x: 152
        y: 65
        width: 200
        height: 200
        color: "#ffffff"

        TestComponent {
            id: testComponent
            x: 55
            y: 35
            text: qsTr("This is text")
        }
    }

    Text {
        id: text1
        x: 17
        y: 133
        text: qsTr("Text")
        font.pixelSize: 12
    }

    TestLayout {
        id: testLayout
        x: 8
        y: 252
        width: 100
        height: 100

        CheckBox {
            id: checkBox
            text: qsTr("Check Box")
        }

        CheckBox {
            id: checkBox1
            text: qsTr("Check Box")
        }

        CheckBox {
            id: checkBox2
            text: qsTr("Check Box")
        }
    }
}
