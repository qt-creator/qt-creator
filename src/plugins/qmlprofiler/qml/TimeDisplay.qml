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

Canvas {
    id: timeDisplay
    objectName: "TimeDisplay"
    contextType: "2d"

    property real startTime : 0
    property real endTime : 0

    Connections {
        target: zoomControl
        onRangeChanged: {
            startTime = zoomControl.startTime();
            endTime = zoomControl.endTime();
            requestPaint();
        }
    }

    onPaint: {
        if (context === null)
            return; // canvas isn't ready
        context.fillStyle = "white";
        context.fillRect(0, 0, width, height);

        var realWidth = width - 1; // account for left border
        var totalTime = Math.max(1, endTime - startTime);
        var spacing = realWidth / totalTime;

        var initialBlockLength = 120;
        var timePerBlock = Math.pow(2, Math.floor( Math.log( totalTime / realWidth * initialBlockLength ) / Math.LN2 ) );
        var pixelsPerBlock = timePerBlock * spacing;
        var pixelsPerSection = pixelsPerBlock / 5;
        var blockCount = realWidth / pixelsPerBlock;

        var realStartTime = Math.floor(startTime/timePerBlock) * timePerBlock;
        var realStartPos = (startTime - realStartTime) * spacing - 1;

        var timePerPixel = timePerBlock/pixelsPerBlock;

        var initialColor = Math.floor(realStartTime/timePerBlock) % 2;

        context.fillStyle = "#000000";
        context.font = "8px sans-serif";
        for (var ii = 0; ii < blockCount+1; ii++) {
            var x = Math.floor(ii*pixelsPerBlock - realStartPos);

            context.fillStyle = (ii+initialColor)%2 ? "#E6E6E6":"white";
            context.fillRect(x, 0, pixelsPerBlock, height);

            context.strokeStyle = "#B0B0B0";
            context.beginPath();
            context.moveTo(x, 0);
            context.lineTo(x, height);
            context.stroke();

            context.fillStyle = "#000000";
            context.fillText(prettyPrintTime(ii*timePerBlock + realStartTime), x + 5, height/2 + 5);
        }

        context.strokeStyle = "#525252";
        context.beginPath();
        context.moveTo(0, height-1);
        context.lineTo(width, height-1);
        context.stroke();

        // left border
        context.fillStyle = "#858585";
        context.fillRect(0, 0, 1, height);
    }

    function clear()
    {
        startTime = endTime = 0;
        requestPaint();
    }

    function prettyPrintTime( t )
    {
        var round = 1;
        var barrier = 1;
        var range = endTime - startTime;
        var units = ["μs", "ms", "s"];

        for (var i = 0; i < units.length; ++i) {
            barrier *= 1000;
            if (range < barrier)
                round *= 1000;
            else if (range < barrier * 10)
                round *= 100;
            else if (range < barrier * 100)
                round *= 10;
            if (t < barrier * 1000)
                return Math.floor(t / (barrier / round)) / round + units[i];
        }

        t /= barrier;
        var m = Math.floor(t / 60);
        var s = Math.floor((t - m * 60) * round) / round;
        return m + "m" + s + "s";
    }
}
