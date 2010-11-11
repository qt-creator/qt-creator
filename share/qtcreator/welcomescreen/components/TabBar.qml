import QtQuick 1.0
import "custom" as Components
import "plugin"


Item {
    id: tabbar
    property int tabHeight: tabrow.height
    property int tabWidth: tabrow.width

    Keys.onRightPressed: {
        if (tabFrame && tabFrame.current < tabFrame.count - 1)
            tabFrame.current = tabFrame.current + 1
    }
    Keys.onLeftPressed: {
        if (tabFrame && tabFrame.current > 0)
            tabFrame.current = tabFrame.current - 1
    }

    height: tabHeight

    property Item tabFrame
    onTabFrameChanged:parent = tabFrame
    visible: tabFrame ? tabFrame.tabsVisible : true
    property int __overlap : styleitem.pixelMetric("tabvshift");
    property string position: tabFrame ? tabFrame.position : "North"
    property string tabBarAlignment: styleitem.styleHint("tabbaralignment");
    property int tabOverlap: styleitem.pixelMetric("taboverlap");
    property int tabBaseOverlap: styleitem.pixelMetric("tabbaseoverlap");
    property int tabHSpace: styleitem.pixelMetric("tabhspace");
    property int tabVSpace: styleitem.pixelMetric("tabvspace");

    function tab(index) {
        for (var i = 0; i < tabrow.children.length; ++i) {
            if (tabrow.children[i].tabindex == index) {
                return tabrow.children[i]
            }
        }
        return null;
    }

    QStyleItem {
        visible:false
        id:styleitem
        elementType: "tab"
        text: "generic"
    }

    Row {
        id: tabrow
        focus: true
        property int paintMargins: 1
        states:
                State {
            when: tabBarAlignment == "center"
            name: "centered"
            AnchorChanges {
                target:tabrow
                anchors.horizontalCenter: tabbar.horizontalCenter
            }
        }

        Repeater {
            id:repeater
            focus:true
            model: tabFrame ? tabFrame.tabs.length : null
            delegate: Item {
                id:tab
                focus:true
                property int tabindex: index
                property bool selected : tabFrame.current == index
                z: selected ? 1 : -1
                function updateRect() {
                    var rect = style.sizeFromContents(textitem.width + tabHSpace + 2, Math.max(style.fontHeight + tabVSpace + 6, 0))
                    width = rect.width
                    height = rect.height
                }
                // Component.onCompleted: print("taboverlap" + tabOverlap + " tabbaseoverlap " + tabBaseOverlap + " overlap " +__overlap + " hspace " + tabHSpace)
                QStyleItem {
                    id: style
                    elementType: "tab"
                    selected: tab.selected
                    info: tabbar.position
                    text: tabFrame.tabs[index].title
                    hover: mousearea.containsMouse
                    focus: tabbar.focus && selected
                    property bool first: index === 0
                    paintMargins: tabrow.paintMargins
                    activeControl: tabFrame.count == 1 ? "only" : index === 0 ? "beginning" :
                            index == tabFrame.count-1 ? "end" : "middle"
                    anchors.fill: parent
                    anchors.margins: -paintMargins
                    Text {
                        id: textitem
                        // Used for size hint
                        visible: false
                        onWidthChanged: updateRect()
                        onHeightChanged: updateRect()
                        text: tabFrame.tabs[index].title
                    }
                }
                MouseArea {
                    id: mousearea
                    anchors.fill: parent
                    hoverEnabled: true
                    onPressed: tabFrame.current = index
                }
            }
        }
    }
}
