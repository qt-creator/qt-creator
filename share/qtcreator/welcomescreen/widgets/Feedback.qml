import QtQuick 1.1

Row {
    id: feedback
    spacing: 4
    Image {
        y: 10
        visible: false
        source: "images/icons/userguideIcon.png"
    }

    LinkedText {
        x: 19
        y: 5
        text: qsTr("Feedback")
        height: 38

        verticalAlignment: Text.AlignBottom
        onClicked: welcomeMode.sendFeedback()
    }
}
