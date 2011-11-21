import QtQuick 1.0

Item {
    property int x: 10
    property var x: 10 // 311 14 16
    property string x: "abc"
    property var x: "abc" // 311 14 16
    property string x: true
    property var x: true // 311 14 16
    property color x: Qt.rgba(1, 1, 1, 1)
    property var x: Qt.rgba(1, 1, 1, 1) // 311 14 16
    property point x: Qt.point(1, 1)
    property var x: Qt.point(1, 1) // 311 14 16
    property rect x: Qt.rect(1, 1, 1, 1)
    property var x: Qt.rect(1, 1, 1, 1) // 311 14 16
    property size x: Qt.size(1, 1)
    property var x: Qt.size(1, 1) // 311 14 16
    property vector3d x: Qt.vector3d(1, 1, 1)
    property var x: Qt.vector3d(1, 1, 1) // 311 14 16
}
