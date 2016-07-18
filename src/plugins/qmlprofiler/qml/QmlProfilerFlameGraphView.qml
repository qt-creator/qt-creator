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
import QmlProfilerFlameGraphModel 1.0
import "../flamegraph/"

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
            sizeRole: QmlProfilerFlameGraphModel.DurationRole
            sizeThreshold: 0.002
            maximumDepth: 25
            y: flickable.height > height ? flickable.height - height : 0

            delegate: FlameGraphDelegate {
                id: flamegraphItem

                property int typeId: FlameGraph.data(QmlProfilerFlameGraphModel.TypeIdRole) || -1
                property bool isBindingLoop: parent.checkBindingLoop(typeId)
                property bool rangeTypeVisible:
                    root.visibleRangeTypes & (1 << FlameGraph.data(QmlProfilerFlameGraphModel.RangeTypeRole))

                itemHeight: rangeTypeVisible ? flamegraph.itemHeight : 0
                isSelected: typeId !== -1 && typeId === root.selectedTypeId && rangeTypeVisible

                borderColor: {
                    if (isSelected)
                        return flamegraph.blue2;
                    else if (tooltip.hoveredNode === flamegraphItem)
                        return flamegraph.blue1;
                    else if (note() !== "" || isBindingLoop)
                        return flamegraph.orange;
                    else
                        return flamegraph.grey1;
                }
                borderWidth: {
                    if (tooltip.hoveredNode === flamegraphItem ||
                            tooltip.selectedNode === flamegraphItem) {
                        return 2;
                    } else if (note() !== "") {
                        return 3;
                    } else {
                        return 1;
                    }
                }

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

                function buildText() {
                    if (!FlameGraph.dataValid)
                        return "<others>";

                    return FlameGraph.data(QmlProfilerFlameGraphModel.DetailsRole) + " ("
                            + FlameGraph.data(QmlProfilerFlameGraphModel.TypeRole) + ", "
                            + FlameGraph.data(QmlProfilerFlameGraphModel.TimeInPercentRole) + "%)";
                }
                text: textVisible ? buildText() : ""
                FlameGraph.onDataChanged: if (textVisible) text = buildText();

                onMouseEntered: {
                    tooltip.hoveredNode = flamegraphItem;
                }

                onMouseExited: {
                    if (tooltip.hoveredNode === flamegraphItem)
                        tooltip.hoveredNode = null;
                }

                onClicked: {
                    if (flamegraphItem.FlameGraph.dataValid) {
                        tooltip.selectedNode = flamegraphItem;
                        root.typeSelected(flamegraphItem.FlameGraph.data(
                                              QmlProfilerFlameGraphModel.TypeIdRole));
                        root.gotoSourceLocation(
                                    flamegraphItem.FlameGraph.data(
                                        QmlProfilerFlameGraphModel.FilenameRole),
                                    flamegraphItem.FlameGraph.data(
                                        QmlProfilerFlameGraphModel.LineRole),
                                    flamegraphItem.FlameGraph.data(
                                        QmlProfilerFlameGraphModel.ColumnRole));
                    }
                }

                // Functions, not properties to limit the initial overhead when creating the nodes,
                // and because FlameGraph.data(...) cannot be notified anyway.
                function title() { return FlameGraph.data(QmlProfilerFlameGraphModel.TypeRole) || ""; }
                function note() { return FlameGraph.data(QmlProfilerFlameGraphModel.NoteRole) || ""; }
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
                        addDetail(qsTr("Details"), QmlProfilerFlameGraphModel.DetailsRole, noop);
                        addDetail(qsTr("Type"), QmlProfilerFlameGraphModel.TypeRole, noop);
                        addDetail(qsTr("Calls"), QmlProfilerFlameGraphModel.CallCountRole, noop);
                        addDetail(qsTr("Total Time"), QmlProfilerFlameGraphModel.DurationRole, printTime);
                        addDetail(qsTr("Mean Time"), QmlProfilerFlameGraphModel.TimePerCallRole, printTime);
                        addDetail(qsTr("In Percent"), QmlProfilerFlameGraphModel.TimeInPercentRole,
                                  addPercent);
                        addDetail(qsTr("Location"), QmlProfilerFlameGraphModel.LocationRole, noop);
                    }
                    return model;
                }
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
