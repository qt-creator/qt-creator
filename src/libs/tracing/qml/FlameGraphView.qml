/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

import FlameGraph 1.0
import TimelineTheme 1.0

import QtQml 2.2
import QtQuick 2.9
import QtQuick.Controls 1.3

ScrollView {
    id: root

    property int selectedTypeId: -1
    signal typeSelected(int typeId)

    onSelectedTypeIdChanged: {
        // selectedTypeId can be set from outside. Don't send typeSelected() from here.
        tooltip.hoveredNode = null;
        flamegraph.selectedTypeId = selectedTypeId;
    }

    function resetRoot() { flamegraph.resetRoot(); }
    property bool zoomed: flamegraph.zoomed

    property int sizeRole: -1
    property var model: null

    property int typeIdRole: -1
    property int sourceFileRole: -1
    property int sourceLineRole: -1
    property int sourceColumnRole: -1
    property int detailsTitleRole: -1
    property int summaryRole: -1
    property int noteRole: -1

    property var trRoleNames: []

    property var modes: []

    property var details: function(flameGraph) { return []; }
    property var summary: function(attached) {
        if (!attached.dataValid)
            return qsTr("others");

        return attached.data(summaryRole) + " (" + percent(sizeRole, attached) + "%)";
    }

    property var isHighlighted: function(node) {
        return false;
    }

    function percent(role, attached) {
        return Math.round(attached.data(role) / flamegraph.total(role) * 1000) / 10;
    }

    function toMap(previousValue, currentValue, currentIndex, array) {
        if (currentIndex % 2 === 1)
            previousValue[array[currentIndex - 1]] = array[currentIndex];
        return previousValue;
    }

    function addDetail(role, format, model, attached) {
        model.push(trRoleNames[role]);
        model.push(format(role, attached)
                   + (modes.indexOf(role) >= 0 ? " (" + percent(role, attached) + "%)" : ""));
    }

    property var detailFormats: {
        "noop": function(role, flameGraph) {
            return flameGraph.data(role);
        },
        "addLine": function(role, flameGraph) {
            var result = flameGraph.data(role);
            var line = flameGraph.data(root.sourceLineRole);
            return line > 0 ? result + ":" + line : result;
        },
        "printTime": function(role, flameGraph) {
            var time = flameGraph.data(role);
            if (time <= 0)
                return "0";
            if (time < 1000)
                return time + " ns";
            time = Math.floor(time / 1000);
            if (time < 1000)
                return time + " μs";
            if (time < 1e6)
                return (time / 1000) + " ms";
            return (time / 1e6) + " s";
        },
        "printMemory": function(role, flameGraph) {
            var bytes = flameGraph.data(role);
            if (bytes === 0)
                return "0b";

            var units = ["b", "kb", "Mb", "Gb"];
            var div = 1;
            for (var i = 0; i < units.length; ++i, div *= 1024) {
                if (bytes > div * 1024)
                    continue;

                bytes /= div;
                var digitsAfterDot = Math.round(3 - Math.log(bytes) / Math.LN10);
                var multiplier = Math.pow(10, digitsAfterDot);
                return Math.round(bytes * multiplier) / multiplier + units[i];
            }
        }
    }

    Flickable {
        id: flickable
        contentHeight: flamegraph.height
        boundsBehavior: Flickable.StopAtBounds

        FlameGraph {
            property int delegateHeight: Math.min(60, Math.max(30, flickable.height / depth))
            property color blue: "blue"
            property color blue1: Qt.lighter(blue)
            property color blue2: Qt.rgba(0.375, 0, 1, 1)
            property color grey1: "#B0B0B0"
            property color highlight: Theme.color(Theme.Timeline_HighlightColor)

            id: flamegraph
            width: parent.width
            height: Math.max(depth * delegateHeight, flickable.height)
            model: root.model
            sizeRole: root.sizeRole
            sizeThreshold: 0.002
            maximumDepth: 128
            y: flickable.height > height ? flickable.height - height : 0

            onSelectedTypeIdChanged: {
                if (selectedTypeId !== root.selectedTypeId) {
                    // Change originates from inside. Send typeSelected().
                    root.selectedTypeId = selectedTypeId;
                    root.typeSelected(selectedTypeId);
                }
            }

            delegate: FlameGraphDelegate {
                id: flamegraphItem

                property var typeId: FlameGraph.data(root.typeIdRole) || -1
                property bool isHighlighted: root.isHighlighted(flamegraphItem)

                itemHeight: flamegraph.delegateHeight
                isSelected: typeId !== -1 && typeId === flamegraph.selectedTypeId

                borderColor: {
                    if (isSelected)
                        return flamegraph.blue2;
                    else if (tooltip.currentNode === flamegraphItem)
                        return flamegraph.blue1;
                    else if (note() !== "" || isHighlighted)
                        return flamegraph.highlight;
                    else
                        return flamegraph.grey1;
                }
                borderWidth: {
                    if (tooltip.currentNode === flamegraphItem) {
                        return 2;
                    } else if (note() !== "" || isHighlighted) {
                        return 3;
                    } else {
                        return 1;
                    }
                }

                onIsSelectedChanged: {
                    if (isSelected && (tooltip.selectedNode === null ||
                            tooltip.selectedNode.typeId !== flamegraph.selectedTypeId)) {
                        tooltip.selectedNode = flamegraphItem;
                    } else if (!isSelected && tooltip.selectedNode === flamegraphItem) {
                        tooltip.selectedNode = null;
                    }
                }

                text: textVisible ? root.summary(FlameGraph) : ""
                FlameGraph.onModelIndexChanged: {
                    if (textVisible)
                        text = root.summary(FlameGraph);

                    // refresh to trigger reevaluation
                    if (tooltip.selectedNode == flamegraphItem) {
                        var selectedNode = tooltip.selectedNode;
                        tooltip.selectedNode = null;
                        tooltip.selectedNode = selectedNode;
                    }
                    if (tooltip.hoveredNode == flamegraphItem) {
                        var hoveredNode = tooltip.hoveredNode;
                        tooltip.hoveredNode = null;
                        tooltip.hoveredNode = hoveredNode;
                    }
                }

                onMouseEntered: {
                    tooltip.hoveredNode = flamegraphItem;
                }

                onMouseExited: {
                    if (tooltip.hoveredNode === flamegraphItem) {
                        // Keep the window around until something else is hovered or selected.
                        if (tooltip.selectedNode === null
                                || tooltip.selectedNode.typeId !== flamegraph.selectedTypeId) {
                            tooltip.selectedNode = flamegraphItem;
                        }
                        tooltip.hoveredNode = null;
                    }
                }

                function selectClicked() {
                    if (FlameGraph.dataValid) {
                        tooltip.selectedNode = flamegraphItem;
                        flamegraph.selectedTypeId = FlameGraph.data(root.typeIdRole);
                        model.gotoSourceLocation(
                                    FlameGraph.data(root.sourceFileRole),
                                    FlameGraph.data(root.sourceLineRole),
                                    FlameGraph.data(root.sourceColumnRole));
                    }
                }

                onClicked: selectClicked()
                onDoubleClicked: {
                    tooltip.selectedNode = null;
                    tooltip.hoveredNode = null;
                    flamegraph.root = FlameGraph.modelIndex;
                    selectClicked();
                }

                // Functions, not properties to limit the initial overhead when creating the nodes,
                // and because FlameGraph.data(...) cannot be notified anyway.
                function title() {
                    return FlameGraph.data(root.detailsTitleRole) || qsTr("unknown");
                }

                function note() {
                    return FlameGraph.data(root.noteRole) || "";
                }

                function details() {
                    return root.details(FlameGraph);
                }
            }
        }

        RangeDetails {
            id: tooltip

            minimumX: 0
            maximumX: flickable.width
            minimumY: flickable.contentY
            maximumY: flickable.contentY + flickable.height
            noteReadonly: true

            borderWidth: 0

            property var hoveredNode: null;
            property var selectedNode: null;

            property var currentNode: {
                if (!locked && hoveredNode !== null)
                    return hoveredNode;
                else if (selectedNode !== null)
                    return selectedNode;
                else
                    return null;
            }

            onClearSelection: {
                flamegraph.selectedTypeId = -1;
                selectedNode = null;
            }

            dialogTitle: {
                if (currentNode)
                    return currentNode.title();
                else if (root.model === null || root.model.rowCount() === 0)
                    return qsTr("No data available");
                else
                    return "";
            }

            model: currentNode ? currentNode.details() : []
            noteText: currentNode ? currentNode.note() : ""

            Connections {
                target: root.model
                onModelReset: {
                    tooltip.hoveredNode = null;
                    tooltip.selectedNode = null;
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
                id: modesMenu
                Instantiator {
                    model: root.modes
                    MenuItem {
                        text: root.trRoleNames[modelData]
                        onTriggered: root.sizeRole = modelData
                    }
                    onObjectAdded: modesMenu.insertItem(index, object)
                    onObjectRemoved: modesMenu.removeItem(object)
                }
            }
        }
    }
}
