/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

import QtQuick 1.1
import qtcomponents 1.0 as Components

Item {
    id: root

    property int currentItem: 0
    property alias model: repeater.model
    property alias itemCount: repeater.count

    Timer {
        id: nextItemTimer
        repeat: true
        interval: 30*1000
        onTriggered: repeater.incrementIndex()
    }

    Timer {
        id: modelUpdateTimer
        repeat: false
        interval: 1000
        onTriggered: repeater.handleModelUpdate();
    }

    Repeater {
        id: repeater
        function incrementIndex() {
            repeater.itemAt(currentItem).active = false
            currentItem = (currentItem+1) % repeater.count
            repeater.itemAt(currentItem).active = true
        }

        function handleModelUpdate() {
            nextItemTimer.stop();
            currentItem = 0;
            for (var i = 0; i < count; ++i) {
                if (i != currentItem)
                    repeater.itemAt(i).active = false;
                else
                    repeater.itemAt(i).active = true;
            }
            nextItemTimer.start();
        }

        function handleModelChanged() {
            modelUpdateTimer.restart();
        }

        function handleItemRemoved(index, item) {
            modelUpdateTimer.restart();
        }

        function handleItemAdded(index, item) {
            modelUpdateTimer.restart();
        }

        anchors.fill: parent
        onModelChanged: handleModelChanged()
        onItemAdded: handleItemAdded(index, item)
        onItemRemoved: handleItemRemoved(index, item)
        delegate: Item {
            property bool active: false
            id: delegateItem
            opacity: 0
            height: root.height
            width: root.width
            Column {
                spacing: 10
                width: parent.width
                id: column
                Text {
                    id: heading1;
                    text: title;
                    font.bold: true;
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere;
                    textFormat: Text.RichText;
                    width: parent.width-icon.width-16
                }
                Row {
                    spacing: 5
                    width: parent.width
                    Image {
                        id: icon;
                        source: blogIcon;
                        asynchronous: true
                    }
                    Text {
                        id: heading2;
                        text: blogName;
                        font.italic: true;
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere;
                        textFormat: Text.RichText;
                        width: parent.width-icon.width-5
                    }
                }
                Text {
                    id: text;
                    text: description;
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    textFormat: Text.RichText
                    width: parent.width-10
                }
                Text { visible: link !== "";
                    id: readmore;
                    text: qsTr("Click to read more...");
                    font.italic: true;
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere;
                    textFormat: Text.RichText
                    width: parent.width-10
                }
            }
            Components.QStyleItem {
                id: styleItem;
                cursor: "pointinghandcursor";
                anchors.fill: column
            }
            Timer {
                id: toolTipTimer
                interval: 500
                onTriggered: styleItem.showToolTip(link)
            }

            MouseArea {
                anchors.fill: column;
                onClicked: Qt.openUrlExternally(link);
                hoverEnabled: true;
                onEntered: { nextItemTimer.stop(); toolTipTimer.start(); }
                onExited: { nextItemTimer.restart(); toolTipTimer.stop(); }
                id: mouseArea
            }

            StateGroup {
                id: activeState
                states: [ State { name: "active"; when: delegateItem.active; PropertyChanges { target: delegateItem; opacity: 1 } } ]
                transitions: [
                    Transition { from: ""; to: "active"; reversible: true; NumberAnimation { target: delegateItem; property: "opacity"; duration: 1000 } }
                ]
            }

            states: [
                State { name: "clicked"; when: mouseArea.pressed;  PropertyChanges { target: text; color: "black" } },
                State { name: "hovered"; when: mouseArea.containsMouse;  PropertyChanges { target: text; color: "#074C1C" } }
            ]


        }
    }
}
