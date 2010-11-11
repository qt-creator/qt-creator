import QtQuick 1.0
import "custom" as Components
import "plugin"

Item {
    id: scrollbar

    property bool upPressed
    property bool downPressed
    property int orientation : Qt.Horizontal
    property alias minimumValue: slider.minimumValue
    property alias maximumValue: slider.maximumValue
    property alias value: slider.value

    width: orientation == Qt.Horizontal ? 200 : internal.scrollbarExtent
    height: orientation == Qt.Horizontal ? internal.scrollbarExtent : 200

    onValueChanged: internal.updateHandle()
    // onMaximumValueChanged: internal.updateHandle()
    // onMinimumValueChanged: internal.updateHandle()

    MouseArea {
        id: internal

        anchors.fill: parent

        property bool autoincrement: false
        property int scrollbarExtent : styleitem.pixelMetric("scrollbarExtent");

        // Update hover item
        onEntered: styleitem.activeControl = styleitem.hitTest(mouseX, mouseY)
        onExited: styleitem.activeControl = "none"
        onMouseXChanged: styleitem.activeControl = styleitem.hitTest(mouseX, mouseY)
        hoverEnabled: true

        Timer {
            running: upPressed || downPressed
            interval: 350
            onTriggered: internal.autoincrement = true
        }

        Timer {
            running: internal.autoincrement
            interval: 60
            repeat: true
            onTriggered: upPressed ? internal.decrement() : internal.increment()
        }

        onPressed: {
            var control = styleitem.hitTest(mouseX,mouseY)
            if (control == "up") {
                upPressed = true
            } else if (control == "down") {
                downPressed = true
            }
        }

        onReleased: {
            autoincrement = false;
            if (upPressed) {
                upPressed = false;
                decrement()
            } else if (downPressed) {
                increment()
                downPressed = false;
            }
        }

        function increment() {
            value += 30
            if (value > maximumValue)
                value = maximumValue
        }

        function decrement() {
            value -= 30
            if (value < minimumValue)
                value = minimumValue
        }

        QStyleItem {
            id: styleitem
            anchors.fill:parent
            elementType: "scrollbar"
            hover: activeControl != "none"
            activeControl: "none"
            sunken: upPressed | downPressed
            minimum: slider.minimumValue
            maximum: slider.maximumValue
            value: slider.value
            horizontal: orientation == Qt.Horizontal
            enabled: parent.enabled
        }

        property variant handleRect: Qt.rect(0,0,0,0)
        function updateHandle() {
            internal.handleRect = styleitem.subControlRect("handle")
            var grooveRect = styleitem.subControlRect("groove");
            var extra = 0
            if (orientation == Qt.Vertical) {
                slider.anchors.topMargin = grooveRect.y + extra
                slider.anchors.bottomMargin = height - grooveRect.y - grooveRect.height + extra
            } else {
                slider.anchors.leftMargin = grooveRect.x + extra
                slider.anchors.rightMargin = width - grooveRect.x - grooveRect.width + extra
            }
        }


        Components.Slider {
            id: slider
            hoverEnabled: false // Handled by the scrollbar background
            orientation: scrollbar.orientation
            anchors.fill: parent
            leftMargin: (orientation === Qt.Horizontal) ? internal.handleRect.width / 2 : internal.handleRect.height / 2
            rightMargin: leftMargin
            handle: Item {
                width: orientation == Qt.Vertical ? internal.handleRect.height : internal.handleRect.width;
                height: orientation == Qt.Vertical ? internal.handleRect.width : internal.handleRect.height
            }
            groove:null
            containsMouse: false
            valueIndicator:null
            inverted:orientation != Qt.Horizontal
        }
    }
}
