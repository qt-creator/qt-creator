// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtCreator.Tracing

Item {
    id: timeMarks
    visible: model && (mockup || (!model.hidden && !model.empty))
    property TimelineModel model
    property bool startOdd
    property bool mockup

    readonly property int scaleMinHeight: 60
    readonly property int scaleStepping: 30
    readonly property string units: " kMGT"


    property int rowCount: model ? model.rowCount : 0

    function roundTo3Digits(number: real) : real  {
        var factor;

        if (number < 10)
            factor = 100;
        else if (number < 100)
            factor = 10;
        else
            factor = 1;

        return Math.round(number * factor) / factor;
    }

    function prettyPrintScale(amount: real) : string {
        var sign;
        if (amount < 0) {
            sign = "-";
            amount = -amount;
        } else {
            sign = "";
        }
        var unitOffset = 0;
        var unitAmount = 1;
        for (unitOffset = 0; amount > unitAmount * 1024; ++unitOffset, unitAmount *= 1024) {}
        var result = amount / unitAmount;
        return sign + roundTo3Digits(result) + units[unitOffset];
    }

    Connections {
        target: timeMarks.model
        function onExpandedRowHeightChanged(row: real, height: real) {
            if (timeMarks.model && timeMarks.model.expanded && row >= 0)
                rowRepeater.itemAt(row).height = height;
        }
    }


    Column {
        id: scaleArea
        anchors.left: parent.left
        anchors.right: parent.right
        Repeater {
            id: rowRepeater
            model: timeMarks.rowCount
            Rectangle {
                id: scaleItem
                required property int index
                property TimeMarks scope: timeMarks

                color: ((index + (scope.startOdd ? 1 : 0)) % 2)
                       ? Theme.color(Theme.Timeline_BackgroundColor1)
                       : Theme.color(Theme.Timeline_BackgroundColor2)
                anchors.left: scaleArea.left
                anchors.right: scaleArea.right
                height: scope.model ? scope.model.rowHeight(index) : 0

                property double minVal: scope.model ? scope.model.rowMinValue(index) : 0
                property double maxVal: scope.model ? scope.model.rowMaxValue(index) : 0
                property double valDiff: maxVal - minVal
                property bool scaleVisible: scope.model && scope.model.expanded &&
                                            height > timeMarks.scaleMinHeight && valDiff > 0

                property double stepVal: {
                    var ret = 1;
                    var ugly = Math.ceil(valDiff / Math.floor(height / timeMarks.scaleStepping));
                    while (isFinite(ugly) && ugly > 1) {
                        ugly /= 2;
                        ret *= 2;
                    }
                    return ret;
                }

                Loader {
                    anchors.fill: parent
                    active: parent.scaleVisible
                    sourceComponent: Item {
                        id: scaleParent
                        anchors.fill: parent

                        TimelineText {
                            id: scaleTopLabel
                            font.pixelSize: 8
                            anchors.top: parent.top
                            anchors.leftMargin: 2
                            anchors.topMargin: 2
                            anchors.left: parent.left
                            text: prettyPrintScale(scaleItem.maxVal)
                        }

                        Repeater {
                            model: scaleItem.valDiff / scaleItem.stepVal

                            Item {
                                anchors.left: scaleParent.left
                                anchors.right: scaleParent.right
                                height: scaleItem.stepVal * scaleItem.height / scaleItem.valDiff
                                y: scaleItem.height - (index + 1) * height
                                visible: y > scaleTopLabel.height
                                TimelineText {
                                    font.pixelSize: 8
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 2
                                    anchors.leftMargin: 2
                                    anchors.left: parent.left
                                    text: prettyPrintScale(scaleItem.minVal
                                                           + index * scaleItem.stepVal)
                                }

                                Rectangle {
                                    height: 1
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    color: Theme.color(Theme.Timeline_DividerColor)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
