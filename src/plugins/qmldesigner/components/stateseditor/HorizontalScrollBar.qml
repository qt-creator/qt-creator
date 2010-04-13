import Qt 4.7

Item {
    property variant flickable: this;
    property int viewPosition: 0;
    property int viewSize: ( flickable.width>=0 ? flickable.width : 0 );
    property int contentSize: ( flickable.contentWidth >= 0 ? flickable.contentWidth : 0 );

    signal unFocus();

    id: horizontalScrollbar
    width: 300
    height: 10

    onViewPositionChanged: flickable.contentX=viewPosition;
    onViewSizeChanged: {
        if ((contentSize > viewSize) && (viewPosition > contentSize - viewSize))
            viewPosition = contentSize - viewSize;
    }

    function contentSizeDecreased()  {
        if ((contentSize > viewSize) && (viewPosition > contentSize - viewSize))
            viewPosition = contentSize - viewSize;
    }

    Rectangle {
        id: groove
        width: parent.width - 4
        height: 6
        color: "#444444"
        radius: 3
        border.width: 1
        border.color: "#333333"
        anchors.right: parent.right
        anchors.rightMargin: 2
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.verticalCenter: parent.verticalCenter
        x: 2
    }

    // the scrollbar
    Item {
        id: bar
        width: parent.width
        height: parent.height
        x: Math.floor( (horizontalScrollbar.contentSize > 0 ? horizontalScrollbar.viewPosition * horizontalScrollbar.width / horizontalScrollbar.contentSize : 0));

        Rectangle {
            id: handle
            width: {
                if (horizontalScrollbar.contentSize > 0) {
                    if (horizontalScrollbar.viewSize > horizontalScrollbar.contentSize || parent.width < 0) {
                        horizontalScrollbar.visible = false;
                        return parent.width;
                    } else {
                        horizontalScrollbar.visible = true;
                        return Math.floor(horizontalScrollbar.viewSize / horizontalScrollbar.contentSize  * parent.width);
                    }
                } else {
                    return 0;
                }
            }

            height: parent.height
            y:0
            border.color: "#333333"
            border.width: 1
            radius: 3

            gradient: Gradient {
                GradientStop { position: 0.20; color: "#888888" }
                GradientStop { position: 0.23; color: "#656565" }
                GradientStop { position: 0.85; color: "#393939" }
            }

            MouseArea {
                property int dragging:0;
                property int originalX:0;

                anchors.fill: parent
                onPressed: { dragging = 1; originalX = mouse.x; horizontalScrollbar.unFocus(); }
                onReleased: { dragging = 0; }
                onPositionChanged: {
                    if (dragging) {
                        var newX = mouse.x - originalX + bar.x;
                        if (newX<0) newX=0;
                        if (newX>horizontalScrollbar.width - handle.width)
                            newX=horizontalScrollbar.width - handle.width;
                        horizontalScrollbar.viewPosition = (newX * horizontalScrollbar.contentSize / horizontalScrollbar.width);
                    }
                }
            }
        }
    }
}
