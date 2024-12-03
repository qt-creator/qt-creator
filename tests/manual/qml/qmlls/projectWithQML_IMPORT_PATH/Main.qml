import QtQuick
// this qmlls warning should not be there: Warnings occurred while importing "MyModule"
import MyModule

Window {
    id: window
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    // this qmlls warning should be there (to prove that qmlls is indeed running): Property "invalid" does not exist
    invalid: 123
}
