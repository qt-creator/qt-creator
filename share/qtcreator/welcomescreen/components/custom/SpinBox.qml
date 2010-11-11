import QtQuick 1.0

FocusScope {
    id: spinbox
    SystemPalette{id:syspal}

    property int minimumWidth: 0
    property int minimumHeight: 0

    property int leftMargin: 0
    property int topMargin: 0
    property int rightMargin: 0
    property int bottomMargin: 0

    width: Math.max(minimumWidth,
                    input.width + leftMargin + rightMargin)

    height: Math.max(minimumHeight,
                     input.height + topMargin + bottomMargin)

    property real value: 0.0
    property real maximumValue: 99
    property real minimumValue: 0
    property real singleStep: 1
    property string postfix

    property bool upEnabled: value != maximumValue;
    property bool downEnabled: value != minimumValue;
    property alias upPressed: mouseUp.pressed
    property alias downPressed: mouseDown.pressed
    property alias upHovered: mouseUp.containsMouse
    property alias downHovered: mouseDown.containsMouse
    property alias containsMouse: mouseArea.containsMouse
    property color textColor: syspal.text
    property alias font: input.font

    property Component background: null
    property Item backgroundItem: backgroundComponent.item
    property Component up: null
    property Component down: null

    QtObject {
        id: componentPrivate
        property bool valueUpdate: false
    }

    function increment() {
        setValue(input.text)
        value += singleStep
        if (value > maximumValue)
            value = maximumValue
        input.text = value
    }

    function decrement() {
        setValue(input.text)
        value -= singleStep
        if (value < minimumValue)
            value = minimumValue
        input.text = value
    }

    function setValue(v) {
        var newval = parseFloat(v)
        if (newval > maximumValue)
            newval = maximumValue
        else if (v < minimumValue)
            newval = minimumValue
        value = newval
        input.text = value
    }

    Component.onCompleted: setValue(value)

    onValueChanged: {
        componentPrivate.valueUpdate = true
        input.text = value
        componentPrivate.valueUpdate = false
    }

    // background
    Loader {
        id: backgroundComponent
        anchors.fill: parent
        sourceComponent: background
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
    }

    TextInput {
        id: input

        font.pixelSize: 14
        anchors.margins: 5
        anchors.leftMargin: leftMargin
        anchors.topMargin: topMargin
        anchors.rightMargin: rightMargin
        anchors.bottomMargin: bottomMargin
        anchors.fill: parent
        selectByMouse: true

        // validator: DoubleValidator { bottom: minimumValue; top: maximumValue; }
        onAccepted: {setValue(input.text)}
        onActiveFocusChanged: setValue(input.text)
        color: textColor
        opacity: parent.enabled ? 1 : 0.5
        Text {
            text: postfix
            anchors.rightMargin: 4
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Loader {
        id: upButton
        property alias pressed : spinbox.upPressed
        property alias hover : spinbox.upHovered
        property alias enabled : spinbox.upEnabled
        sourceComponent: up
        MouseArea {
            id: mouseUp
            anchors.fill: upButton.item
            onClicked: increment()

            property bool autoincrement: false;
            onReleased: autoincrement = false
            Timer { running: mouseUp.pressed; interval: 350 ; onTriggered: mouseUp.autoincrement = true }
            Timer { running: mouseUp.autoincrement; interval: 60 ; repeat: true ; onTriggered: increment() }
        }
        onLoaded: {
            item.parent = spinbox
            mouseUp.parent = item
        }
    }

    Loader {
        id: downButton
        property alias pressed : spinbox.downPressed
        property alias hover : spinbox.downHovered
        property alias enabled : spinbox.downEnabled
        sourceComponent: down
        MouseArea {
            id: mouseDown
            anchors.fill: downButton.item
            onClicked: decrement()

            property bool autoincrement: false;
            onReleased: autoincrement = false
            Timer { running: mouseDown.pressed; interval: 350 ; onTriggered: mouseDown.autoincrement = true }
            Timer { running: mouseDown.autoincrement; interval: 60 ; repeat: true ; onTriggered: decrement() }
        }
        onLoaded: {
            item.parent = spinbox
            mouseDown.parent = item
        }
    }
    Keys.onUpPressed: increment()
    Keys.onDownPressed: decrement()
}
