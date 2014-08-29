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
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

Item {
    id: labelContainer
    property string text: trigger(1) ? qmlProfilerModelProxy.displayName(modelIndex) : ""
    property bool expanded: trigger(qmlProfilerModelProxy.expanded(modelIndex))
    property int modelIndex: index
    property int bindingTrigger: 1
    property var descriptions: []
    property var extdescriptions: []
    property var selectionIds: []
    property bool dragging
    property Item draggerParent

    signal dragStarted;
    signal dragStopped;

    readonly property int dragHeight: 5

    function trigger(i) {
        return i * bindingTrigger * bindingTrigger;
    }

    property bool reverseSelect: false

    visible: trigger(qmlProfilerModelProxy.rowCount(modelIndex)) > 0

    height: trigger(qmlProfilerModelProxy.height(modelIndex))
    width: 150

    function updateDescriptions() {
        bindingTrigger = -bindingTrigger;
        if (!visible)
            return;

        var desc=[];
        var ids=[];
        var extdesc=[];
        var labelList = qmlProfilerModelProxy.labels(modelIndex);
        for (var i = 0; i < labelList.length; i++ ) {
            extdesc[i] = desc[i] = (labelList[i].description || qsTr("<bytecode>"));
            ids[i] = labelList[i].id;
            if (labelList[i].displayName !== undefined)
                extdesc[i] += " (" + labelList[i].displayName + ")";
        }
        descriptions = desc;
        selectionIds = ids;
        extdescriptions = extdesc;
    }

    Connections {
        target: qmlProfilerModelProxy.models[modelIndex]
        onExpandedChanged: updateDescriptions()
        onRowHeightChanged: updateDescriptions()
        onHiddenChanged: updateDescriptions()
    }

    Connections {
        target: qmlProfilerModelProxy
        onStateChanged: updateDescriptions()
        onModelsChanged: updateDescriptions()
    }

    MouseArea {
        id: dragArea
        anchors.fill: txt
        drag.target: dragger
        cursorShape: dragging ? Qt.DragMoveCursor : Qt.OpenHandCursor
    }

    DropArea {
        id: dropArea

        onPositionChanged: {
            if ((drag.source.modelIndex > labelContainer.modelIndex &&
                    drag.source.y < labelContainer.y + drag.source.height) ||
                    (drag.source.modelIndex < labelContainer.modelIndex &&
                    drag.source.y > labelContainer.y + labelContainer.height -
                    drag.source.height)) {
                qmlProfilerModelProxy.swapModels(drag.source.modelIndex,
                                                 labelContainer.modelIndex);
                drag.source.modelIndex = labelContainer.modelIndex;
            }
        }

        anchors.fill: parent
    }

    Text {
        id: txt
        x: 5
        font.pixelSize: 12
        text: labelContainer.text
        color: "#232323"
        height: trigger(qmlProfilerModelProxy.rowHeight(modelIndex, 0))
        width: 140
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
    }

    Rectangle {
        height: 1
        width: parent.width
        color: "#999999"
        anchors.bottom: parent.bottom
        z: 2
    }

    Column {
        anchors.top: txt.bottom
        visible: expanded
        Repeater {
            model: descriptions.length
            Button {
                width: labelContainer.width
                height: trigger(qmlProfilerModelProxy.rowHeight(modelIndex, index + 1))
                action: Action {
                    onTriggered: {
                        if (reverseSelect)
                            view.selectPrevFromSelectionId(modelIndex,selectionIds[index]);
                        else
                            view.selectNextFromSelectionId(modelIndex,selectionIds[index]);
                    }

                    tooltip: extdescriptions[index]
                }

                style: ButtonStyle {
                    background: Rectangle {
                        border.width: 1
                        border.color: "#c8c8c8"
                        color: "#eaeaea"
                    }
                    label: Text {
                        text: descriptions[index]
                        textFormat: Text.PlainText
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        renderType: Text.NativeRendering
                    }
                }
                MouseArea {
                    hoverEnabled: true
                    property bool resizing: false
                    onPressed: resizing = true
                    onReleased: resizing = false

                    height: dragHeight
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    cursorShape: Qt.SizeVerCursor

                    onMouseYChanged: {
                        if (resizing)
                            qmlProfilerModelProxy.setRowHeight(modelIndex, index + 1, y + mouseY);
                    }
                }
            }
        }
    }

    ToolButton {
        id: expandButton
        anchors.verticalCenter: txt.verticalCenter
        anchors.right: parent.right
        implicitWidth: 17
        implicitHeight: txt.height - 1
        enabled: expanded || trigger(qmlProfilerModelProxy.count(modelIndex)) > 0
        iconSource: expanded ? "arrow_down.png" : "arrow_right.png"
        tooltip: qsTr(expanded ? "Collapse category" : "Expand category.")
        onClicked: qmlProfilerModelProxy.setExpanded(modelIndex, !expanded);
    }

    Rectangle {
        id: dragger
        property int modelIndex
        width: labelContainer.width
        height: 0
        color: "black"
        opacity: 0.5
        anchors.left: parent.left

        // anchor to top so that it reliably snaps back after dragging
        anchors.top: parent.top

        Drag.active: dragArea.drag.active
        Drag.onActiveChanged: {
            // We don't want height, text, or modelIndex to be changed when reordering occurs, so we
            // don't make them properties.
            draggerText.text = txt.text;
            modelIndex = labelContainer.modelIndex;
            if (Drag.active) {
                height = labelContainer.height;
                labelContainer.dragStarted();
            } else {
                height = 0;
                labelContainer.dragStopped();
            }
        }

        states: [
            State {
                when: dragger.Drag.active
                ParentChange {
                    target: dragger
                    parent: draggerParent
                }
                PropertyChanges {
                    target: dragger
                    anchors.top: undefined
                }
            }
        ]

        Text {
            id: draggerText
            visible: parent.Drag.active
            x: txt.x
            font.pixelSize: txt.font.pixelSize
            color: "white"
            width: txt.width
            height: txt.height
            verticalAlignment: txt.verticalAlignment
            renderType: txt.renderType
        }
    }

    MouseArea {
        anchors.top: dragArea.bottom
        anchors.bottom: labelContainer.dragging ? labelContainer.bottom : dragArea.bottom
        anchors.left: labelContainer.left
        anchors.right: labelContainer.right
        cursorShape: dragArea.cursorShape
    }

}
