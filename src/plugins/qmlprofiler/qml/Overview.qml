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
import "Overview.js" as Plotter

Canvas {
    id: canvas
    objectName: "Overview"
    contextType: "2d"

    property int eventsPerPass: 4096
    property int increment: -1
    property int offset: -1

    // ***** properties
    height: 50
    property bool dataReady: false
    property double startTime : 0
    property double endTime : 0
    property bool recursionGuard: false

    onWidthChanged: offset = -1

    // ***** functions
    function clear()
    {
        dataReady = false;
        increment = -1;
        offset = -1;
        requestPaint();
    }

    function updateRange() {
        if (recursionGuard)
            return;
        var newStartTime = Math.round(rangeMover.getLeft() * qmlProfilerModelProxy.traceDuration() / width) + qmlProfilerModelProxy.traceStartTime();
        var newEndTime = Math.round(rangeMover.getRight() * qmlProfilerModelProxy.traceDuration() / width) + qmlProfilerModelProxy.traceStartTime();
        if (startTime !== newStartTime || endTime !== newEndTime)
            zoomControl.setRange(newStartTime, Math.max(newEndTime, newStartTime + 500));
    }

    function clamp(val, min, max) {
        return Math.min(Math.max(val, min), max);
    }

    // ***** connections to external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            if (qmlProfilerModelProxy) {
                recursionGuard = true;
                startTime = clamp(zoomControl.startTime(), qmlProfilerModelProxy.traceStartTime(), qmlProfilerModelProxy.traceEndTime());
                endTime = clamp(zoomControl.endTime(), startTime, qmlProfilerModelProxy.traceEndTime());
                var newRangeX = (startTime - qmlProfilerModelProxy.traceStartTime()) * width / qmlProfilerModelProxy.traceDuration();
                var newWidth = (endTime - startTime) * width / qmlProfilerModelProxy.traceDuration();
                var widthChanged = Math.abs(newWidth - rangeMover.getWidth()) > 1;
                var leftChanged = Math.abs(newRangeX - rangeMover.getLeft()) > 1;
                if (leftChanged)
                    rangeMover.setLeft(newRangeX);

                if (leftChanged || widthChanged)
                    rangeMover.setRight(newRangeX + newWidth);
                recursionGuard = false;
            }
        }
    }

    Connections {
        target: qmlProfilerModelProxy
        onDataAvailable: {
            dataReady = true;
            increment = Math.ceil(qmlProfilerModelProxy.count() / eventsPerPass);
            offset = -1;
            requestPaint();
        }
    }

    Timer {
        id: paintTimer
        onTriggered: canvas.requestPaint();
    }

    // ***** slots
    onPaint: {
        if (context === null)
            return; // canvas isn't ready

        Plotter.qmlProfilerModelProxy = qmlProfilerModelProxy;

        if (offset < 0) {
            context.reset();
            Plotter.drawGraph(canvas, context);
            if (dataReady) {
                Plotter.drawTimeBar(canvas, context);
                offset = 0;
                // Larger initial delay to avoid flickering on resize
                paintTimer.interval = 1000;
                paintTimer.start();
            }
        } else if (offset < increment) {
            Plotter.drawData(canvas, context);
            ++offset;
            paintTimer.interval = 1;
            paintTimer.start();
        }
    }

    // ***** child items
    MouseArea {
        anchors.fill: canvas
        function jumpTo(posX) {
            var rangeWidth = rangeMover.getWidth();
            var newX = posX - rangeWidth / 2;
            if (newX < 0)
                newX = 0;
            if (newX + rangeWidth > canvas.width)
                newX = canvas.width - rangeWidth;

            if (newX < rangeMover.getLeft()) {
                rangeMover.setLeft(newX);
                rangeMover.setRight(newX + rangeWidth);
            } else if (newX > rangeMover.getLeft()) {
                rangeMover.setRight(newX + rangeWidth);
                rangeMover.setLeft(newX);
            }
        }

        onPressed: {
            jumpTo(mouse.x);
        }
        onPositionChanged: {
            jumpTo(mouse.x);
        }
    }

    RangeMover {
        id: rangeMover
        visible: dataReady
        onRangeChanged: canvas.updateRange()
    }

    Rectangle {
        height: 1
        width: parent.width
        color: "#858585"
    }
}
