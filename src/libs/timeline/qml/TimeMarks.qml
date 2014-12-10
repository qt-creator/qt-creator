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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

Item {
    id: timeMarks
    visible: model && (mockup || (!model.hidden && !model.empty))
    property QtObject model
    property bool startOdd
    property bool mockup

    readonly property int scaleMinHeight: 60
    readonly property int scaleStepping: 30
    readonly property string units: " kMGT"


    property int rowCount: model ? model.rowCount : 0

    function prettyPrintScale(amount) {
        var unitOffset = 0;
        for (unitOffset = 0; amount > (1 << ((unitOffset + 1) * 10)); ++unitOffset) {}
        var result = (amount >> (unitOffset * 10));
        if (result < 100) {
            var comma = Math.round(((amount >> ((unitOffset - 1) * 10)) & 1023) *
                                   (result < 10 ? 100 : 10) / 1024);
            if (comma < 10 && result < 10)
                return result + ".0" + comma + units[unitOffset];
            else
                return result + "." + comma + units[unitOffset];
        } else {
            return result + units[unitOffset];
        }
    }

    Connections {
        target: model
        onExpandedRowHeightChanged: {
            if (model && model.expanded && row >= 0)
                rowRepeater.itemAt(row).height = height;
        }
    }


    Column {
        id: rows
        anchors.left: parent.left
        anchors.right: parent.right
        Repeater {
            id: rowRepeater
            model: timeMarks.rowCount
            Rectangle {
                id: row
                color: ((index + (startOdd ? 1 : 0)) % 2) ? "#f0f0f0" : "white"
                anchors.left: rows.left
                anchors.right: rows.right
                height: timeMarks.model ? timeMarks.model.rowHeight(index) : 0

                property int minVal: timeMarks.model ? timeMarks.model.rowMinValue(index) : 0
                property int maxVal: timeMarks.model ? timeMarks.model.rowMaxValue(index) : 0
                property int valDiff: maxVal - minVal
                property bool scaleVisible: timeMarks.model && timeMarks.model.expanded &&
                                            height > scaleMinHeight && valDiff > 0

                property int stepVal: {
                    var ret = 1;
                    var ugly = Math.ceil(valDiff / Math.floor(height / scaleStepping));
                    while (isFinite(ugly) && ugly > 1) {
                        ugly >>= 1;
                        ret <<= 1;
                    }
                    return ret;
                }

                Text {
                    id: scaleTopLabel
                    renderType: Text.NativeRendering
                    visible: parent.scaleVisible
                    color: "#B0B0B0"
                    font.pixelSize: 8
                    anchors.top: parent.top
                    anchors.leftMargin: 2
                    anchors.topMargin: 2
                    anchors.left: parent.left
                    text: prettyPrintScale(row.maxVal)
                }

                Repeater {
                    model: parent.scaleVisible ? row.valDiff / row.stepVal : 0

                    Item {
                        anchors.left: row.left
                        anchors.right: row.right
                        height: row.stepVal * row.height / row.valDiff
                        y: row.height - (index + 1) * height
                        visible: y > scaleTopLabel.height
                        Text {
                            renderType: Text.NativeRendering
                            color: "#B0B0B0"
                            font.pixelSize: 8
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 2
                            anchors.leftMargin: 2
                            anchors.left: parent.left
                            text: prettyPrintScale(index * row.stepVal)
                        }

                        Rectangle {
                            height: 1
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            color: "#B0B0B0"
                        }
                    }
                }
            }
        }
    }
}
