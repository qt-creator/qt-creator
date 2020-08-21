import QtQuick 2.12
import loginui1 1.0

Rectangle {
    width: Constants.width
    height: Constants.height
    color: "#fdfdfd"

    Text {
        id: pageTitle
        text: qsTr("Qt Account")
        font.pixelSize: 24
        anchors.verticalCenterOffset: -153
        anchors.horizontalCenterOffset: 1
        anchors.centerIn: parent
        font.family: Constants.font.family
    }

    Image {
        id: logo
        x: 13
        y: 0
        width: 100
        height: 100
        source: "qt_logo_green_64x64px.png"
        fillMode: Image.PreserveAspectFit
    }

    PushButton {
        id: loginButton
        x: 262
        y: 343
        width: 120
        height: 40
        text: qsTr("Log In")
    }

    PushButton {
        id: registerButton
        x: 262
        y: 389
        width: 120
        height: 40
        text: qsTr("Create Account")
    }
}
