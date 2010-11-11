import QtQuick 1.0

Rectangle {
    id: root
    height: 110
    color: "#00ffffff"
    radius: 6

    Text {
        id: title
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10
        text: model.name
        font.bold: true
        font.pixelSize: 16;

    }

    RatingBar { id: rating; anchors.top: parent.top; anchors.topMargin: 10; anchors.right: parent.right; anchors.rightMargin: 10; rating: model.difficulty; visible: model.difficulty !== 0 }

    Image {
        property bool hideImage : model.imageUrl === "" || status === Image.Error
        id: image
        anchors.top: title.bottom
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 10
        width: hideImage ? 0 : 90
        height: hideImage ? 0 : 66
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: model.imageUrl !== "" ? "image://helpimage/" + encodeURI(model.imageUrl) : ""
    }

    Text {
        id: description
        clip: true
        anchors.left: image.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.top: rating.bottom
        anchors.topMargin: 6
        wrapMode: Text.WordWrap
        text: model.description
    }
    Text { id: labelText; anchors.top: description.bottom; anchors.topMargin: 10; anchors.left: image.right; text: "Tags: "; font.bold: true; }
    Row { id: tagLine; anchors.top: description.bottom; anchors.topMargin: 10; anchors.left: labelText.right; Text { text: model.tags.join(", "); color: "grey" } }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            if (model.hasSourceCode)
                gettingStarted.openProject(model.projectPath, model.filesToOpen, model.docUrl)
            else
                gettingStarted.openSplitHelp(model.docUrl);
        }
        onEntered: parent.state = "hover"
        onExited: parent.state = ""
    }

    states: [ State { name: "hover"; PropertyChanges { target: root; color: "#eeeeeeee" } } ]

    transitions:
        Transition {
        from: ""
        to: "hover"
        reversible: true
        ColorAnimation { duration: 100; easing.type: Easing.OutQuad }
    }
}
