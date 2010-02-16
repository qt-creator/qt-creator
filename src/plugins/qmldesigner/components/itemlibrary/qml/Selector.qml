import Qt 4.6

Item {
    id: selector

    property var style
    property var scrollFlickable

    signal moveScrollbarHandle(int viewportPos)

    visible: false

    property var section: null
    property var item: null
    property int sectionXOffset: 0
    property int sectionYOffset: 0

    property real staticX: (section && item)? (section.x + item.x + sectionXOffset):0;
    property real staticY: (section && item)? (section.y + style.selectionSectionOffset + item.y + sectionYOffset):0;

    property bool animateMove: true

    Connection {
        sender: itemLibraryModel
        signal: "visibilityUpdated()"
        script: selector.unselectNow()
    }

    function select(section, item, sectionXOffset, sectionYOffset) {

        if (!selector.visible) {
            selector.animateMove = false
        }

        selector.item = item
        selector.section = section
        selector.sectionXOffset = sectionXOffset
        selector.sectionYOffset = sectionYOffset

        if (!selector.visible) {
//            print("no prev selection");

//            selector.opacity = 0
            selector.visible = true
//            selectAnimation.start();
        }

        focusSelection();
    }

    function focusSelection() {
        var pos = -1;

        if (selector.staticY < scrollFlickable.viewportY)
            pos = selector.staticY
        else if ((selector.staticY + selector.height) >
                 (scrollFlickable.viewportY + scrollFlickable.height - 1))
            pos = selector.staticY + selector.height - scrollFlickable.height + 1

        if (pos >= 0) {
/*
            followSelectionAnimation.to = pos
            followSelectionAnimation.start()
*/
            scrollFlickable.viewportY = pos;
            selector.moveScrollbarHandle(pos)
        }
    }

    function unselect() {
//        unselectAnimation.start();
        unselectNow();
    }

    function unselectNow() {
        selector.section = null
        selector.item = null
        selector.sectionXOffset = 0
        selector.sectionYOffset = 0
        selector.visible = false
        selector.opacity = 1
    }
/*
    NumberAnimation {
        id: selectAnimation
        target: selector
        property: "opacity"
        from: 0
        to: 1
        duration: 200
        onRunningChanged: {
            if (!running)
                selector.animateMove = true
        }
    }

    NumberAnimation {
        id: unselectAnimation
        target: selector
        property: "opacity"
        from: 1
        to: 0
        duration: 150
        onRunningChanged: {
            if (!running)
                selector.unselectNow();
        }
    }

    NumberAnimation {
        id: followSelectionAnimation
        target: scrollFlickable
        property: "viewportY"
        duration: 200
    }

    x: SpringFollow {
        source: selector.staticX;
        spring: selector.animateMove? 3.0:0.0;
        damping: 0.35
        epsilon: 0.25
    }

    y: SpringFollow {
        source: selector.staticY;
        spring: selector.animateMove? 3.0:0.0;
        damping: 0.35
        epsilon: 0.25
    }
*/

    x: selector.staticX
    y: selector.staticY

    Rectangle {
        anchors.fill: parent
        color: "steelblue"
    }
}

