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
import Monitor 1.0

Canvas2D {
    id: timeDisplay
    objectName: "TimeMarks"

    property real startTime
    property real endTime
    property real timePerPixel

    Connections {
        target: labels
        onHeightChanged: { requestRedraw(); }
    }

    onDrawRegion: {
        drawBackgroundBars( ctxt, region );

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

        var lineStart = y < 0 ? -y : 0;
        var lineEnd = Math.min(height, labels.height - y);

        ctxt.fillStyle = "#000000";
        ctxt.font = "8px sans-serif";
        for (var ii = 0; ii < blockCount+1; ii++) {
            var x = Math.floor(ii*pixelsPerBlock - realStartPos);
            ctxt.strokeStyle = "#B0B0B0";
            ctxt.beginPath();
            ctxt.moveTo(x, lineStart);
            ctxt.lineTo(x, lineEnd);
            ctxt.stroke();

            ctxt.strokeStyle = "#CCCCCC";
            for (var jj=1; jj < 5; jj++) {
                var xx = Math.floor(ii*pixelsPerBlock + jj*pixelsPerSection - realStartPos);
                ctxt.beginPath();
                ctxt.moveTo(xx, lineStart);
                ctxt.lineTo(xx, lineEnd);
                ctxt.stroke();
            }
        }
    }

    function updateMarks(start, end) {
        if (startTime !== start || endTime !== end) {
            startTime = start;
            endTime = end;
            requestRedraw();
        }
    }

    function drawBackgroundBars( ctxt, region ) {
        var colorIndex = true;

        // row background
        var backgroundOffset = y < 0 ? -y : -(y % (2 * root.singleRowHeight));
        for (var currentY= backgroundOffset; currentY < Math.min(height, labels.height - y); currentY += root.singleRowHeight) {
            ctxt.fillStyle = colorIndex ? "#f0f0f0" : "white";
            ctxt.strokeStyle = colorIndex ? "#f0f0f0" : "white";
            ctxt.fillRect(0, currentY, width, root.singleRowHeight);
            colorIndex = !colorIndex;
        }

        // separators
        var cumulatedHeight = 0;
        for (var modelIndex = 0; modelIndex < qmlProfilerModelProxy.modelCount() && cumulatedHeight < y + height; modelIndex++) {
            for (var i=0; i<qmlProfilerModelProxy.categoryCount(modelIndex); i++) {
                cumulatedHeight += root.singleRowHeight * qmlProfilerModelProxy.categoryDepth(modelIndex, i);
                if (cumulatedHeight < y)
                    continue;

                ctxt.strokeStyle = "#B0B0B0";
                ctxt.beginPath();
                ctxt.moveTo(0, cumulatedHeight - y);
                ctxt.lineTo(width, cumulatedHeight - y);
                ctxt.stroke();
            }
        }

        // bottom
        if (height > labels.height - y) {
            ctxt.fillStyle = "#f5f5f5";
            ctxt.fillRect(0, labels.height - y, width, Math.min(height - labels.height + y, labelsTail.height));
        }
    }
}
