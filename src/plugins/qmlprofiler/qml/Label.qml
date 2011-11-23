/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

import QtQuick 1.0

Item {
    id: labelContainer
    property alias text: txt.text
    property bool expanded: false
    property int typeIndex: index

    property variant descriptions: []
    property variant eventIds: []

    height: root.singleRowHeight
    width: 150

    onExpandedChanged: {
        var rE = labels.rowExpanded;
        rE[typeIndex] = expanded;
        labels.rowExpanded = rE;
        backgroundMarks.requestRedraw();
        view.setRowExpanded(typeIndex, expanded);
        updateHeight();
    }

    Component.onCompleted: {
        updateHeight();
    }

    function updateHeight() {
        height = root.singleRowHeight * (1 +
            (expanded ? qmlEventList.uniqueEventsOfType(typeIndex) : qmlEventList.maxNestingForType(typeIndex)));
    }

    Connections {
        target: qmlEventList
        onDataReady: {
            var desc=[];
            var ids=[];
            for (var i=0; i<qmlEventList.uniqueEventsOfType(typeIndex); i++) {
                desc[i] = qmlEventList.eventTextForType(typeIndex, i);
                ids[i] = qmlEventList.eventIdForType(typeIndex, i);
            }
            descriptions = desc;
            eventIds = ids;
            updateHeight();
        }
        onDataClear: {
            descriptions = [];
            eventIds = [];
            updateHeight();
        }
    }

    Text {
        id: txt
        x: 5
        font.pixelSize: 12
        color: "#232323"
        height: root.singleRowHeight
        width: 140
        verticalAlignment: Text.AlignVCenter
    }

    Rectangle {
        height: 1
        width: parent.width
        color: "#999999"
        anchors.bottom: parent.bottom
        z: 2
    }

    Column {
        y: root.singleRowHeight
        visible: expanded
        Repeater {
            model: descriptions.length
            Rectangle {
                width: labelContainer.width
                height: root.singleRowHeight
                color: "#eaeaea"
                border.width: 1
                border.color:"#c8c8c8"
                Text {
                    height: root.singleRowHeight
                    x: 5
                    width: 140
                    text: descriptions[index]
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (mouse.modifiers & Qt.ShiftModifier)
                            view.selectPrevFromId(eventIds[index]);
                        else
                            view.selectNextFromId(eventIds[index]);
                    }
                }
            }
        }
    }

    Image {
        visible: descriptions.length > 0
        source: expanded ? "arrow_down.png" : "arrow_right.png"
        x: parent.width - 12
        y: root.singleRowHeight / 2 - height / 2
        MouseArea {
            anchors.fill: parent
            anchors.rightMargin: -10
            anchors.leftMargin: -10
            anchors.topMargin: -10
            anchors.bottomMargin: -10
            onClicked: {
                expanded = !expanded;
            }
        }
    }
}
