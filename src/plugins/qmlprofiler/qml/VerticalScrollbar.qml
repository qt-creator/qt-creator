import QtQuick 1.0

Item {
    property variant flickable: this;
    property int viewPosition: 0;
    property int viewSize: ( flickable.height>=0 ? flickable.height : 0 );
    property int contentSize: ( flickable.contentHeight >= 0 ? flickable.contentHeight : 0 );

    id: verticalScrollbar

    onViewPositionChanged: flickable.contentY = viewPosition;
    onViewSizeChanged: {
        if ((contentSize > viewSize) && (viewPosition > contentSize - viewSize))
            viewPosition = contentSize - viewSize;
    }

    onContentSizeChanged:  {
        contentSizeDecreased();
    }

    function contentSizeDecreased()  {
        if ((contentSize > viewSize) && (viewPosition > contentSize - viewSize))
            viewPosition = contentSize - viewSize;
    }

    Rectangle {
        id: groove
        height: parent.height - 4
        width: 6
        color: "#FFFFFF"
        radius: 3
        border.width: 1
        border.color: "#666666"
        anchors.top: parent.top
        anchors.topMargin: 2
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        y: 2
    }

    // the scrollbar
    Item {
        id: bar
        height: parent.height
        width: parent.width
        y: Math.floor( (verticalScrollbar.contentSize > 0 ? verticalScrollbar.viewPosition * verticalScrollbar.height / verticalScrollbar.contentSize : 0));

        Rectangle {
            id: handle
            height: {
                if (verticalScrollbar.contentSize > 0) {
                    if (verticalScrollbar.viewSize > verticalScrollbar.contentSize || parent.height < 0) {
                        verticalScrollbar.visible = false;
                        return parent.height;
                    } else {
                        verticalScrollbar.visible = true;
                        return Math.floor(verticalScrollbar.viewSize / verticalScrollbar.contentSize  * parent.height);
                    }
                } else {
                    return 0;
                }
            }

            width: parent.width
            y:0
            border.color: "#666666"
            border.width: 1
            radius: 3

            gradient: Gradient {
                GradientStop { position: 0.20; color: "#CCCCCC" }
                GradientStop { position: 0.23; color: "#AAAAAA" }
                GradientStop { position: 0.85; color: "#888888" }
            }

            MouseArea {
                property int dragging:0;
                property int originalY:0;

                anchors.fill: parent
                onPressed: { dragging = 1; originalY = mouse.y; }
                onReleased: { dragging = 0; }
                onPositionChanged: {
                    if (dragging) {
                        var newY = mouse.y - originalY + bar.y;
                        if (newY<0) newY=0;
                        if (newY>verticalScrollbar.height - handle.height)
                            newY=verticalScrollbar.height - handle.height;
                        verticalScrollbar.viewPosition = (newY * verticalScrollbar.contentSize / verticalScrollbar.height);
                    }
                }
            }
        }
    }
}
