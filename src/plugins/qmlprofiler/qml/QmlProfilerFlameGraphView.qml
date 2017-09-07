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
import TimelineTheme 1.0
import "../flamegraph/"

ScrollView {
    id: root

    property int selectedTypeId: -1
    property int sizeRole: QmlProfilerFlameGraphModel.DurationRole

    readonly property var trRoleNames: [
        QmlProfilerFlameGraphModel.DurationRole,      qsTr("Total Time"),
        QmlProfilerFlameGraphModel.CallCountRole,     qsTr("Calls"),
        QmlProfilerFlameGraphModel.DetailsRole,       qsTr("Details"),
        QmlProfilerFlameGraphModel.TimePerCallRole,   qsTr("Mean Time"),
        QmlProfilerFlameGraphModel.TimeInPercentRole, qsTr("In Percent"),
        QmlProfilerFlameGraphModel.LocationRole,      qsTr("Location"),
        QmlProfilerFlameGraphModel.AllocationsRole,   qsTr("Allocations"),
        QmlProfilerFlameGraphModel.MemoryRole,        qsTr("Memory")
    ].reduce(function(previousValue, currentValue, currentIndex, array) {
        if (currentIndex % 2 === 1)
            previousValue[array[currentIndex - 1]] = array[currentIndex];
        return previousValue;
    }, {})

    onSelectedTypeIdChanged: tooltip.hoveredNode = null

    Flickable {
        id: flickable
        contentHeight: flamegraph.height
        boundsBehavior: Flickable.StopAtBounds

        FlameGraph {
            property int delegateHeight: Math.max(30, flickable.height / depth)
            property color blue: "blue"
            property color blue1: Qt.lighter(blue)
            property color blue2: Qt.rgba(0.375, 0, 1, 1)
            property color grey1: "#B0B0B0"
            property color grey2: "#A0A0A0"
            property color highlight: Theme.color(Theme.Timeline_HighlightColor)

            function checkBindingLoop(otherTypeId) {return false;}

            id: flamegraph
            width: parent.width
            height: depth * delegateHeight
            model: flameGraphModel
            sizeRole: root.sizeRole
            sizeThreshold: 0.002
            maximumDepth: 25
            y: flickable.height > height ? flickable.height - height : 0

            delegate: FlameGraphDelegate {
                id: flamegraphItem

                property int typeId: FlameGraph.data(QmlProfilerFlameGraphModel.TypeIdRole) || -1
                property bool isBindingLoop: parent.checkBindingLoop(typeId)

                itemHeight: flamegraph.delegateHeight
                isSelected: typeId !== -1 && typeId === root.selectedTypeId

                borderColor: {
                    if (isSelected)
                        return flamegraph.blue2;
                    else if (tooltip.hoveredNode === flamegraphItem)
                        return flamegraph.blue1;
                    else if (note() !== "" || isBindingLoop)
                        return flamegraph.highlight;
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
                            + Math.floor(width / flamegraph.width * 1000) / 10 + "%)";
                }
                text: textVisible ? buildText() : ""
                FlameGraph.onDataChanged: if (textVisible) text = buildText();

                onMouseEntered: {
                    tooltip.hoveredNode = flamegraphItem;
                }

                onMouseExited: {
                    if (tooltip.hoveredNode === flamegraphItem) {
                        // Keep the window around until something else is hovered or selected.
                        if (tooltip.selectedNode === null
                                || tooltip.selectedNode.typeId !== root.selectedTypeId) {
                            tooltip.selectedNode = flamegraphItem;
                        }
                        tooltip.hoveredNode = null;
                    }
                }

                onClicked: {
                    if (flamegraphItem.FlameGraph.dataValid) {
                        tooltip.selectedNode = flamegraphItem;
                        flameGraphModel.typeSelected(flamegraphItem.FlameGraph.data(
                                              QmlProfilerFlameGraphModel.TypeIdRole));
                        flameGraphModel.gotoSourceLocation(
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
                    function addDetail(index, format) {
                        model.push(trRoleNames[index]);
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

                    function printMemory(a) {
                        if (a === 0)
                            return "0b";

                        var units = ["b", "kb", "Mb", "Gb"];
                        var div = 1;
                        for (var i = 0; i < units.length; ++i, div *= 1024) {
                            if (a > div * 1024)
                                continue;

                            a /= div;
                            var digitsAfterDot = Math.round(3 - Math.log(a) / Math.LN10);
                            var multiplier = Math.pow(10, digitsAfterDot);
                            return Math.round(a * multiplier) / multiplier + units[i];
                        }
                    }

                    if (!FlameGraph.dataValid) {
                        model.push(qsTr("Details"));
                        model.push(qsTr("Various Events"));
                    } else {
                        addDetail(QmlProfilerFlameGraphModel.DetailsRole, noop);
                        addDetail(QmlProfilerFlameGraphModel.CallCountRole, noop);
                        addDetail(QmlProfilerFlameGraphModel.DurationRole, printTime);
                        addDetail(QmlProfilerFlameGraphModel.TimePerCallRole, printTime);
                        addDetail(QmlProfilerFlameGraphModel.TimeInPercentRole, addPercent);
                        addDetail(QmlProfilerFlameGraphModel.LocationRole, noop);
                        addDetail(QmlProfilerFlameGraphModel.MemoryRole, printMemory);
                        addDetail(QmlProfilerFlameGraphModel.AllocationsRole, noop);
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

            titleBarColor: Theme.color(Theme.Timeline_PanelHeaderColor)
            titleBarTextColor: Theme.color(Theme.PanelTextColorLight)
            contentColor: Theme.color(Theme.Timeline_PanelBackgroundColor)
            contentTextColor: Theme.color(Theme.Timeline_TextColor)
            noteTextColor: Theme.color(Theme.Timeline_HighlightColor)
            buttonHoveredColor: Theme.color(Theme.FancyToolButtonHoverColor)
            buttonSelectedColor: Theme.color(Theme.FancyToolButtonSelectedColor)
            borderWidth: 0

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
                flameGraphModel.typeSelected(-1);
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

        Button {
            x: flickable.width - width
            y: flickable.contentY

            // It won't listen to anchors.margins and by default it doesn't add any margin. Great.
            width: implicitWidth + 20

            text: qsTr("Visualize %1").arg(trRoleNames[root.sizeRole])

            menu: Menu {
                MenuItem {
                    text: trRoleNames[QmlProfilerFlameGraphModel.DurationRole]
                    onTriggered: root.sizeRole = QmlProfilerFlameGraphModel.DurationRole
                }

                MenuItem {
                    text: trRoleNames[QmlProfilerFlameGraphModel.MemoryRole]
                    onTriggered: root.sizeRole = QmlProfilerFlameGraphModel.MemoryRole
                }

                MenuItem {
                    text: trRoleNames[QmlProfilerFlameGraphModel.AllocationsRole]
                    onTriggered: root.sizeRole = QmlProfilerFlameGraphModel.AllocationsRole
                }
            }
        }
    }
}
