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
import Monitor 1.0

Canvas {
    id: timeMarks
    objectName: "TimeMarks"
    contextType: "2d"

    property QtObject model
    property bool startOdd

    readonly property int scaleMinHeight: 60
    readonly property int scaleStepping: 30
    readonly property string units: " kMGT"

    property real startTime
    property real endTime
    property real timePerPixel

    Connections {
        target: model
        onHeightChanged: requestPaint()
    }

    onStartTimeChanged: requestPaint()
    onEndTimeChanged: requestPaint()
    onYChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var context = (timeMarks.context === null) ? getContext("2d") : timeMarks.context;
        context.reset();
        drawBackgroundBars( context, region );

        var totalTime = endTime - startTime;
        var spacing = width / totalTime;

        var initialBlockLength = 120;
        var timePerBlock = Math.pow(2, Math.floor( Math.log( totalTime / width * initialBlockLength ) / Math.LN2 ) );
        var pixelsPerBlock = timePerBlock * spacing;
        var pixelsPerSection = pixelsPerBlock / 5;
        var blockCount = width / pixelsPerBlock;

        var realStartTime = Math.floor(startTime/timePerBlock) * timePerBlock;
        var realStartPos = (startTime-realStartTime) * spacing;

        timePerPixel = timePerBlock/pixelsPerBlock;


        for (var ii = 0; ii < blockCount+1; ii++) {
            var x = Math.floor(ii*pixelsPerBlock - realStartPos);
            context.strokeStyle = "#B0B0B0";
            context.beginPath();
            context.moveTo(x, 0);
            context.lineTo(x, height);
            context.stroke();

            context.strokeStyle = "#CCCCCC";
            for (var jj=1; jj < 5; jj++) {
                var xx = Math.floor(ii*pixelsPerBlock + jj*pixelsPerSection - realStartPos);
                context.beginPath();
                context.moveTo(xx, 0);
                context.lineTo(xx, height);
                context.stroke();
            }
        }
    }

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

    function drawBackgroundBars( context, region ) {
        var colorIndex = startOdd;

        context.font = "8px sans-serif";

        // separators
        var cumulatedHeight = 0;

        for (var row = 0; row < model.rowCount; ++row) {
            // row background
            var rowHeight = model.rowHeight(row)
            cumulatedHeight += rowHeight;
            colorIndex = !colorIndex;
            if (cumulatedHeight < y - rowHeight)
                continue;
            context.strokeStyle = context.fillStyle = colorIndex ? "#f0f0f0" : "white";
            context.fillRect(0, cumulatedHeight - rowHeight - y, width, rowHeight);

            if (rowHeight >= scaleMinHeight) {
                var minVal = model.rowMinValue(row);
                var maxVal = model.rowMaxValue(row);
                if (minVal !== maxVal) {
                    context.strokeStyle = context.fillStyle = "#B0B0B0";

                    var stepValUgly = Math.ceil((maxVal - minVal) /
                                            Math.floor(rowHeight / scaleStepping));

                    // align to clean 2**x
                    var stepVal = 1;
                    while (stepValUgly >>= 1)
                        stepVal <<= 1;

                    var stepHeight = rowHeight / (maxVal - minVal);

                    for (var step = minVal; step <= maxVal - stepVal; step += stepVal) {
                        var offset = cumulatedHeight - step * stepHeight - y;
                        context.beginPath();
                        context.moveTo(0, offset);
                        context.lineTo(width, offset);
                        context.stroke();
                        context.fillText(prettyPrintScale(step), 5, offset - 2);
                    }
                    context.beginPath();
                    context.moveTo(0, cumulatedHeight - rowHeight - y);
                    context.lineTo(width, cumulatedHeight - rowHeight - y);
                    context.stroke();
                    context.fillText(prettyPrintScale(maxVal), 5,
                                     cumulatedHeight - rowHeight - y + 8);

                }
            }

            if (cumulatedHeight > y + height)
                return;
        }

        context.strokeStyle = "#B0B0B0";
        context.beginPath();
        context.moveTo(0, cumulatedHeight - y);
        context.lineTo(width, cumulatedHeight - y);
        context.stroke();
    }
}
