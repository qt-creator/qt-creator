/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 2.1

Item {
    id: labelContainer
    property string text: qmlProfilerModelProxy.categoryLabel(modelIndex, categoryIndex)
    property bool expanded: false
    property int categoryIndex: qmlProfilerModelProxy.correctedCategoryIndexForModel(modelIndex, index)
    property int modelIndex: qmlProfilerModelProxy.modelIndexForCategory(index);

    property var descriptions: []
    property var extdescriptions: []
    property var eventIds: []

    visible: qmlProfilerModelProxy.categoryDepth(modelIndex, categoryIndex) > 0;

    height: root.singleRowHeight
    width: 150

    Component.onCompleted: {
        updateHeight();
    }

    function updateHeight() {
        height = root.singleRowHeight * qmlProfilerModelProxy.categoryDepth(modelIndex, categoryIndex);
    }

    function getDescriptions() {
        visible = qmlProfilerModelProxy.categoryDepth(modelIndex, categoryIndex) > 0;
        if (!visible)
            return;

        var desc=[];
        var ids=[];
        var extdesc=[];
        var labelList = qmlProfilerModelProxy.getLabelsForCategory(modelIndex, categoryIndex);
        for (var i = 0; i < labelList.length; i++ ) {
            desc[i] = labelList[i].description;
            ids[i] = labelList[i].id;
            extdesc[i] = labelList[i].displayName + ":" + labelList[i].description;
        }
        descriptions = desc;
        eventIds = ids;
        extdescriptions = extdesc;
        updateHeight();
    }

    Connections {
        target: qmlProfilerModelProxy
        onExpandedChanged: {
            expanded = qmlProfilerModelProxy.expanded(modelIndex, categoryIndex);
            backgroundMarks.requestPaint();
            getDescriptions();
        }

        onStateChanged: {
            getDescriptions();
        }
    }

    Text {
        id: txt
        x: 5
        font.pixelSize: 12
        text: labelContainer.text
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
                    textFormat: Text.PlainText
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
                            view.selectPrevFromId(modelIndex,eventIds[index]);
                        else
                            view.selectNextFromId(modelIndex,eventIds[index]);
                    }
                }
            }
        }
    }

    Image {
        source: expanded ? "arrow_down.png" : "arrow_right.png"
        x: parent.width - 12
        y: root.singleRowHeight / 2 - height / 2
        smooth: false
        MouseArea {
            anchors.fill: parent
            anchors.rightMargin: -10
            anchors.leftMargin: -10
            anchors.topMargin: -10
            anchors.bottomMargin: -10
            onClicked: {
                // Don't try to expand empty models.
                if (expanded || qmlProfilerModelProxy.count(modelIndex) > 0)
                    qmlProfilerModelProxy.setExpanded(modelIndex, categoryIndex, !expanded);
            }
        }
    }
}
