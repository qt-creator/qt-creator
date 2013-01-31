/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0

Item {
    id: labelContainer
    property alias text: txt.text
    property bool expanded: false
    property int typeIndex: index

    property variant descriptions: []
    property variant extdescriptions: []
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
            (expanded ? qmlProfilerDataModel.uniqueEventsOfType(typeIndex) :
                        qmlProfilerDataModel.maxNestingForType(typeIndex)));
    }

    function getDescriptions() {
        var desc=[];
        var ids=[];
        var extdesc=[];
        for (var i=0; i<qmlProfilerDataModel.uniqueEventsOfType(typeIndex); i++) {
            desc[i] = qmlProfilerDataModel.eventTextForType(typeIndex, i);
            ids[i] = qmlProfilerDataModel.eventIdForType(typeIndex, i);
            extdesc[i] = qmlProfilerDataModel.eventDisplayNameForType(typeIndex, i) +
                        " : " + desc[i];
        }
        descriptions = desc;
        eventIds = ids;
        extdescriptions = extdesc;
        updateHeight();
    }

    Connections {
        target: qmlProfilerDataModel
        onReloadDetailLabels: getDescriptions();
        onStateChanged: {
            // Empty
            if (qmlProfilerDataModel.getCurrentStateFromQml() == 0) {
                descriptions = [];
                eventIds = [];
                extdescriptions = [];
                updateHeight();
            } else
            // Done
            if (qmlProfilerDataModel.getCurrentStateFromQml() == 3) {
                getDescriptions();
            }
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
                    hoverEnabled: true
                    onEntered: changeToolTip(extdescriptions[index]);
                    onExited: changeToolTip("");
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
