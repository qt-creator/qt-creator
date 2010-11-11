import QtQuick 1.0
import "custom" as Components
import "plugin"

Item {
    id: tabWidget
    width: 100
    height: 100
    focus: true
    property TabBar tabbar
    property int current: 0
    property int count: stack.children.length
    property bool frame:true
    property bool tabsVisible: true
    property string position: "North"
    default property alias tabs : stack.children

    onCurrentChanged: __setOpacities()
    Component.onCompleted: __setOpacities()
    onTabbarChanged: {
        tabbar.tabFrame = tabWidget
        tabbar.anchors.top = tabWidget.top
        tabbar.anchors.left = tabWidget.left
        tabbar.anchors.right = tabWidget.right
    }

    property int __baseOverlap : frameitem.pixelMetric("tabbaseoverlap")// add paintmargins;
    function __setOpacities() {
        for (var i = 0; i < stack.children.length; ++i) {
            stack.children[i].visible = (i == current ? true : false)
        }
    }

    QStyleItem {
        id: frameitem
        z: style == "oxygen" ? 1 : 0
        elementType: "tabframe"
        info: position
        value: tabbar && tabsVisible && tabbar.tab(current) ? tabbar.tab(current).x : 0
        minimum: tabbar && tabsVisible && tabbar.tab(current) ? tabbar.tab(current).width : 0
        maximum: tabbar && tabsVisible ? tabbar.tabWidth : width
        anchors.fill: parent

        property int frameWidth: pixelMetric("defaultframewidth")

        Item {
            id: stack
            anchors.fill: parent
            anchors.margins: (frame ? frameitem.frameWidth : 0)
            anchors.topMargin: anchors.margins + (frameitem.style =="mac" ? 6 : 0)
            anchors.bottomMargin: anchors.margins + (frameitem.style =="mac" ? 6 : 0)
        }

        anchors.topMargin: tabbar && tabsVisible && position == "North" ? tabbar.height - __baseOverlap : 0

        states: [
            State {
                name: "South"
                when: position == "South" && tabbar!= undefined
                PropertyChanges {
                    target: frameitem
                    anchors.topMargin: 0
                    anchors.bottomMargin: tabbar ? tabbar.height - __baseOverlap: 0
                }
                PropertyChanges {
                    target: tabbar
                    anchors.topMargin: -__baseOverlap
                }
                AnchorChanges {
                    target: tabbar
                    anchors.top: frameitem.bottom
                    anchors.bottom: undefined
                }
            }
        ]
    }
}
