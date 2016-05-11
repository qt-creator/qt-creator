/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Controls 1.3
import FlameGraph 1.0
import FlameGraphModel 1.0

ScrollView {
    id: root
    signal typeSelected(int typeIndex)
    signal gotoSourceLocation(string filename, int line, int column)

    property int selectedTypeId: -1
    property int visibleRangeTypes: -1

    onSelectedTypeIdChanged: tooltip.hoveredNode = null

    Flickable {
        id: flickable
        contentHeight: flamegraph.height
        boundsBehavior: Flickable.StopAtBounds

        FlameGraph {
            property int itemHeight: Math.max(30, flickable.height / depth)
            property int level: -1
            property color blue: "blue"
            property color blue1: Qt.lighter(blue)
            property color blue2: Qt.rgba(0.375, 0, 1, 1)
            property color grey1: "#B0B0B0"
            property color grey2: "#A0A0A0"
            property color orange: "orange"

            function checkBindingLoop(otherTypeId) {return false;}

            id: flamegraph
            width: parent.width
            height: depth * itemHeight
            model: flameGraphModel
            sizeRole: FlameGraphModel.Duration
            sizeThreshold: 0.002
            maximumDepth: 25
            y: flickable.height > height ? flickable.height - height : 0

            delegate: Item {
                id: flamegraphItem

                property int typeId: FlameGraph.data(FlameGraphModel.TypeId) || -1
                property bool isBindingLoop: parent.checkBindingLoop(typeId)
                property int level: parent.level + (rangeTypeVisible ? 1 : 0)
                property bool isSelected: typeId !== -1 && typeId === root.selectedTypeId
                property bool rangeTypeVisible: root.visibleRangeTypes &
                                                (1 << FlameGraph.data(FlameGraphModel.RangeType))

                onIsSelectedChanged: {
                    if (isSelected && (tooltip.selectedNode === null ||
                            tooltip.selectedNode.typeId !== root.selectedTypeId)) {
                        tooltip.selectedNode = flamegraphItem;
                    } else if (!isSelected && tooltip.selectedNode === flamegraphItem) {
                        tooltip.selectedNode = null;
                    }
                }

                function checkBindingLoop(otherTypeId) {
                    if (typeId === otherTypeId) {
                        isBindingLoop = true;
                        return true;
                    } else {
                        return parent.checkBindingLoop(otherTypeId);
                    }
                }

                // Functions, not properties to limit the initial overhead when creating the nodes,
                // and because FlameGraph.data(...) cannot be notified anyway.
                function title() { return FlameGraph.data(FlameGraphModel.Type) || ""; }
                function note() { return FlameGraph.data(FlameGraphModel.Note) || ""; }
                function details() {
                    var model = [];
                    function addDetail(name, index, format) {
                        model.push(name);
                        model.push(format(FlameGraph.data(index)));
                    }

                    function printTime(t)
                    {
                        if (t <= 0)
                            return "0";
                        if (t < 1000)
                            return t + " ns";
                        t = Math.floor(t / 1000);
                        if (t < 1000)
                            return t + " μs";
                        if (t < 1e6)
                            return (t / 1000) + " ms";
                        return (t / 1e6) + " s";
                    }

                    function noop(a) {
                        return a;
                    }

                    function addPercent(a) {
                        return a + "%";
                    }

                    if (!FlameGraph.dataValid) {
                        model.push(qsTr("Details"));
                        model.push(qsTr("Various Events"));
                    } else {
                        addDetail(qsTr("Details"), FlameGraphModel.Details, noop);
                        addDetail(qsTr("Type"), FlameGraphModel.Type, noop);
                        addDetail(qsTr("Calls"), FlameGraphModel.CallCount, noop);
                        addDetail(qsTr("Total Time"), FlameGraphModel.Duration, printTime);
                        addDetail(qsTr("Mean Time"), FlameGraphModel.TimePerCall, printTime);
                        addDetail(qsTr("In Percent"), FlameGraphModel.TimeInPercent,
                                  addPercent);
                        addDetail(qsTr("Location"), FlameGraphModel.Location, noop);
                    }
                    return model;
                }

                Rectangle {
                    border.color: {
                        if (flamegraphItem.isSelected)
                            return flamegraph.blue2;
                        else if (tooltip.hoveredNode === flamegraphItem)
                            return flamegraph.blue1;
                        else if (flamegraphItem.note() !== "" || flamegraphItem.isBindingLoop)
                            return flamegraph.orange;
                        else
                            return flamegraph.grey1;
                    }
                    border.width: {
                        if (tooltip.hoveredNode === flamegraphItem ||
                                tooltip.selectedNode === flamegraphItem) {
                            return 2;
                        } else if (flamegraphItem.note() !== "") {
                            return 3;
                        } else {
                            return 1;
                        }
                    }
                    color: Qt.hsla((level % 12) / 72, 0.9 + Math.random() / 10,
                                   0.45 + Math.random() / 10, 0.9 + Math.random() / 10);
                    height: flamegraphItem.rangeTypeVisible ? flamegraph.itemHeight : 0;
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom

                    FlameGraphText {
                        id: text
                        visible: width > 20 || flamegraphItem === tooltip.selectedNode
                        anchors.fill: parent
                        anchors.margins: 5
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: visible ? buildText() : ""
                        elide: Text.ElideRight
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        font.bold: flamegraphItem === tooltip.selectedNode

                        function buildText() {
                            if (!flamegraphItem.FlameGraph.dataValid)
                                return "<others>";

                            return flamegraphItem.FlameGraph.data(FlameGraphModel.Details) + " (" +
                                   flamegraphItem.FlameGraph.data(FlameGraphModel.Type) + ", " +
                                   flamegraphItem.FlameGraph.data(FlameGraphModel.TimeInPercent) + "%)";
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true

                        onEntered: {
                            tooltip.hoveredNode = flamegraphItem;
                        }

                        onExited: {
                            if (tooltip.hoveredNode === flamegraphItem)
                                tooltip.hoveredNode = null;
                        }

                        onClicked: {
                            if (flamegraphItem.FlameGraph.dataValid) {
                                tooltip.selectedNode = flamegraphItem;
                                root.typeSelected(
                                            flamegraphItem.FlameGraph.data(FlameGraphModel.TypeId));
                                root.gotoSourceLocation(
                                            flamegraphItem.FlameGraph.data(FlameGraphModel.Filename),
                                            flamegraphItem.FlameGraph.data(FlameGraphModel.Line),
                                            flamegraphItem.FlameGraph.data(FlameGraphModel.Column));
                            }
                        }
                    }
                }

                FlameGraph.onDataChanged: if (text.visible) text.text = text.buildText();

                height: flamegraph.height - level * flamegraph.itemHeight;
                width: parent === null ? flamegraph.width : parent.width * FlameGraph.relativeSize
                x: parent === null ? 0 : parent.width * FlameGraph.relativePosition
            }
        }

        FlameGraphDetails {
            id: tooltip

            minimumX: 0
            maximumX: flickable.width
            minimumY: flickable.contentY
            maximumY: flickable.contentY + flickable.height

            property var hoveredNode: null;
            property var selectedNode: null;

            property var currentNode: {
                if (hoveredNode !== null)
                    return hoveredNode;
                else if (selectedNode !== null)
                    return selectedNode;
                else
                    return null;
            }

            onClearSelection: {
                selectedTypeId = -1;
                selectedNode = null;
                root.typeSelected(-1);
            }

            dialogTitle: {
                if (currentNode)
                    return currentNode.title();
                else if (flameGraphModel.rowCount() === 0)
                    return qsTr("No data available");
                else
                    return "";
            }

            model: currentNode ? currentNode.details() : []
            note: currentNode ? currentNode.note() : ""

            Connections {
                target: flameGraphModel
                onModelReset: {
                    tooltip.hoveredNode = null;
                    tooltip.selectedNode = null;
                }
                onDataChanged: {
                    // refresh to trigger reevaluation of note
                    var selectedNode = tooltip.selectedNode;
                    tooltip.selectedNode = null;
                    tooltip.selectedNode = selectedNode;
                }
            }
        }
    }
}
