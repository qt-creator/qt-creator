import QtQuick 1.0
import "custom" as Components
import "plugin"

ScrollArea {
    id:area
    color: "white"
    width: 280
    height: 120
    contentWidth: 200

    property alias text: edit.text
    property alias wrapMode: edit.wrapMode
    property alias readOnly: edit.readOnly

    highlightOnFocus: true
    property int documentMargins: 4
    frame: true

    Item {
        anchors.left: parent.left
        anchors.top: parent.top
        height: edit.height - 8
        anchors.margins: documentMargins

        TextEdit {
            id: edit
            text: loremIpsum + loremIpsum;
            wrapMode: TextEdit.WordWrap;
            width: area.contentWidth
            selectByMouse: true
            readOnly: false
            focus: true

            // keep textcursor within scrollarea
            onCursorPositionChanged: {
                if (cursorRectangle.y >= area.contentY + area.height - 1.5*cursorRectangle.height)
                    area.contentY = cursorRectangle.y - area.height + 1.5*cursorRectangle.height
                else if (cursorRectangle.y < area.contentY)
                    area.contentY = cursorRectangle.y
            }
        }
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_PageUp) {
            verticalValue = verticalValue - area.height
        } else if (event.key == Qt.Key_PageDown)
            verticalValue = verticalValue + area.height
   }
}
