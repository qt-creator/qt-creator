import QtQuick 2.0
import QtQuick.Templates 2.15

Button {
    id: btn

    implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding

    leftPadding: 28
    topPadding: 20
    rightPadding: 28
    bottomPadding: 20

    contentItem: Text {
        text: btn.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        color: "green"
        opacity: btn.down ? 0.8 : 1
        radius: 4
    }
}
