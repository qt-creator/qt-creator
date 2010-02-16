import Qt 4.6

Item {
    id: bar

    property var handleBar: handle

    property var scrollFlickable
    property var style

    property bool scrolling: (scrollFlickable.viewportHeight > scrollFlickable.height)
    property int scrollHeight: height - handle.height

    Binding {
        target: scrollFlickable
        property: "viewportY"
        value: Math.max(0, scrollFlickable.viewportHeight - scrollFlickable.height) *
               handle.y / scrollHeight
    }

    Rectangle {
        anchors.fill: parent;
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        color: "transparent"
        border.width: 1;
        border.color: "#8F8F8F";
    }

    function moveHandle(viewportPos) {
        var pos;

        if (bar.scrollFlickable) {//.visibleArea.yPosition) {
            pos = bar.scrollHeight * viewportPos / (bar.scrollFlickable.viewportHeight - bar.scrollFlickable.height);
        } else
            pos = 0;

//        handleMoveAnimation.to = Math.min(bar.scrollHeight, pos)
//        handleMoveAnimation.start();
        handle.y = Math.min(bar.scrollHeight, pos)
    }

    function limitHandle() {
        // the following "if" is needed to get around NaN when starting up
        if (scrollFlickable)
            handle.y = Math.min(handle.height * scrollFlickable.visibleArea.yPosition,
                                scrollHeight);
        else
            handle.y = 0;
    }
/*
    NumberAnimation {
        id: handleResetAnimation
        target: handle
        property: "y"
        from: handle.y
        to: 0
        duration: 500
    }
*/
    Connection {
        sender: scrollFlickable
        signal: "heightChanged"
        script: {
            /* since binding loops prevent setting the handle properly,
               let's animate it to 0 */
            if (scrollFlickable.viewportY > (scrollFlickable.viewportHeight - scrollFlickable.height))
//                handleResetAnimation.start()
                handle.y = 0
        }
    }

    onScrollFlickableChanged: handle.y = 0

/*
    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 3
        anchors.rightMargin: 3
        anchors.topMargin: 2
        anchors.bottomMargin: 2
        radius: width / 2
        color: style.scrollbarBackgroundColor
    }
*/
    MouseRegion {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: handle.top
        onClicked: {
//            handleMoveAnimation.to = Math.max(0, handle.y - 40)
//            handleMoveAnimation.start();
            handle.y = Math.max(0, handle.y - 40)
        }
    }

    Item {
        id: handle

        anchors.left: parent.left
        anchors.right: parent.right
        height: Math.max(width, bar.height * Math.min(1, scrollFlickable.visibleArea.heightRatio))

//        radius: width / 2
//        color: style.scrollbarHandleColor

        Rectangle {
            width: parent.height
            height: parent.width
            y: parent.height

            rotation: -90
            transformOrigin: Item.TopLeft

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#C6C6C6" }
                GradientStop { position: 1.0; color: "#7E7E7E" }
            }
        }

        MouseRegion {
            anchors.fill: parent
            drag.target: parent
            drag.axis: "YAxis"
            drag.minimumY: 0
            drag.maximumY: scrollHeight
        }
    }

    MouseRegion {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: handle.bottom
        anchors.bottom: parent.bottom
        onClicked: {
//            handleMoveAnimation.to = Math.min(scrollHeight, handle.y + 40)
//            handleMoveAnimation.start();
            handle.y = Math.min(scrollHeight, handle.y + 40)
        }
    }
/*
    NumberAnimation {
        id: handleMoveAnimation
        target: handle
        property: "y"
        duration: 200
    }
*/
}

