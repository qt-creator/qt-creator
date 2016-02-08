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

            id: flamegraph
            width: parent.width
            height: depth * itemHeight
            model: flameGraphModel
            sizeRole: FlameGraphModel.Duration
            sizeThreshold: 0.002
            y: flickable.height > height ? flickable.height - height : 0

            delegate: Item {
                id: flamegraphItem
                property int level: parent.level + (rangeTypeVisible ? 1 : 0)
                property bool isSelected: flamegraphItem.FlameGraph.data(FlameGraphModel.TypeId) ===
                                          root.selectedTypeId
                property bool rangeTypeVisible: root.visibleRangeTypes &
                                                (1 << FlameGraph.data(FlameGraphModel.RangeType))

                Rectangle {
                    border.color: {
                        if (flamegraphItem.isSelected)
                            return flamegraph.blue2;
                        else if (mouseArea.containsMouse)
                            return flamegraph.blue1;
                        else
                            return flamegraph.grey1
                    }
                    border.width: (mouseArea.containsMouse || flamegraphItem.isSelected) ? 2 : 1
                    color: Qt.hsla((level % 12) / 72, 0.9 + Math.random() / 10,
                                   0.45 + Math.random() / 10, 0.9 + Math.random() / 10);
                    height: flamegraphItem.rangeTypeVisible ? flamegraph.itemHeight : 0;
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom

                    FlameGraphText {
                        id: text
                        visible: width > 20
                        anchors.fill: parent
                        anchors.margins: 5
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        text: visible ? buildText() : ""
                        elide: Text.ElideRight
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere

                        function buildText() {
                            if (!flamegraphItem.FlameGraph.dataValid)
                                return "<others>";

                            return flamegraphItem.FlameGraph.data(FlameGraphModel.Details) + " (" +
                                   flamegraphItem.FlameGraph.data(FlameGraphModel.Type) + ", " +
                                   flamegraphItem.FlameGraph.data(FlameGraphModel.TimeInPercent) + "%)";
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true

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

                        onEntered: {
                            var model = [];
                            function addDetail(name, index, format) {
                                model.push(name + ":");
                                model.push(format(flamegraphItem.FlameGraph.data(index)));
                            }

                            function noop(a) {
                                return a;
                            }

                            function addPercent(a) {
                                return a + "%";
                            }

                            if (!flamegraphItem.FlameGraph.dataValid) {
                                model.push(qsTr("Details") + ":");
                                model.push(qsTr("Various Events"));
                            } else {
                                addDetail(qsTr("Details"), FlameGraphModel.Details, noop);
                                addDetail(qsTr("Type"), FlameGraphModel.Type, noop);
                                addDetail(qsTr("Calls"), FlameGraphModel.CallCount, noop);
                                addDetail(qsTr("Total Time"), FlameGraphModel.Duration, printTime);
                                addDetail(qsTr("Mean Time"), FlameGraphModel.TimePerCall, printTime);
                                addDetail(qsTr("In Percent"), FlameGraphModel.TimeInPercent,
                                          addPercent);

                            }
                            tooltip.model = model;
                        }

                        onExited: {
                            tooltip.model = [];
                        }

                        onClicked: {
                            if (flamegraphItem.FlameGraph.dataValid) {
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

        Rectangle {
            color: "white"
            border.width: 1
            border.color: flamegraph.grey2
            width: tooltip.model.length > 0 ? tooltip.width + 10 : 0
            height: tooltip.model.length > 0 ? tooltip.height + 10 : 0
            y: flickable.contentY
            x: anchorRight ? parent.width - width : 0
            property bool anchorRight: true

            Grid {
                id: tooltip
                anchors.margins: 5
                anchors.top: parent.top
                anchors.left: parent.left
                spacing: 5
                columns: 2
                property var model: [ qsTr("No data available") ]

                Connections {
                    target: flameGraphModel
                    onModelReset: {
                        tooltip.model = (flameGraphModel.rowCount() === 0) ?
                                    [ qsTr("No data available") ] : [];
                    }
                }

                Repeater {
                    model: parent.model
                    FlameGraphText {
                        text: modelData
                        font.bold: (index % 2) === 0
                        width: Math.min(implicitWidth, 200)
                        elide: Text.ElideRight
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: parent.anchorRight = !parent.anchorRight
            }
        }
    }
}
