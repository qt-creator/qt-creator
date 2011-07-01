import QtQuick 1.0
import components 1.0 as Components

BorderImage {
    id: inner_background
    height: openProjectButton.height + 10
    source: "qrc:welcome/images/background_center_frame_v2.png"
    border.left: 2
    border.right: 2

    Rectangle { color: "black"; width: parent.width; height: 1; anchors.top: parent.top; anchors.left: parent.left }

    Components.QStyleItem { id: styleItem; visible: false }

    // whitelist
    property bool _hasDesktopTheme: welcomeMode.platform() === "linux"

    Components.Button {
        id: openProjectButton
        text: qsTr("Open Project...")
        iconSource: _hasDesktopTheme ? "image://desktoptheme/document-open" : ""
        onClicked: welcomeMode.openProject();
        height: 32
        anchors.left: parent.left
        anchors.margins: 5
        anchors.verticalCenter: parent.verticalCenter
    }

    Components.Button {
        id: createProjectButton
        text: qsTr("Create Project...")
        iconSource: _hasDesktopTheme ? "image://desktoptheme/document-new" : ""
        onClicked: welcomeMode.newProject();
        height: 32
        anchors.left: openProjectButton.right
        anchors.margins: 5
        anchors.verticalCenter: parent.verticalCenter
    }


    Components.Button {
        id: feedbackButton
        text: qsTr("Feedback")
        iconSource: "qrc:welcome/images/feedback_arrow.png"
        height: 32
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: feedbackText.left
        anchors.margins: 5
        onClicked: welcomeMode.sendFeedback()
    }

    Text {
        id: feedbackText
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.margins: 5
        text: qsTr("Help us make Qt Creator even better")
    }
}
