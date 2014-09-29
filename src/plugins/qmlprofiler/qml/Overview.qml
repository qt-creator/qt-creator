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
import "Overview.js" as Plotter

Canvas {
    id: canvas
    objectName: "Overview"
    contextType: "2d"

    readonly property int eventsPerPass: 512
    property int increment: -1
    property int offset: -1
    readonly property int bump: 10;
    readonly property int blockHeight: (height - bump) / qmlProfilerModelProxy.models.length;

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
        var newStartTime = Math.round(rangeMover.rangeLeft * qmlProfilerModelProxy.traceDuration() / width) + qmlProfilerModelProxy.traceStartTime();
        var newEndTime = Math.round(rangeMover.rangeRight * qmlProfilerModelProxy.traceDuration() / width) + qmlProfilerModelProxy.traceStartTime();
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
                var widthChanged = Math.abs(newWidth - rangeMover.rangeWidth) > 1;
                var leftChanged = Math.abs(newRangeX - rangeMover.rangeLeft) > 1;
                if (leftChanged)
                    rangeMover.rangeLeft = newRangeX;

                if (leftChanged || widthChanged)
                    rangeMover.rangeRight = newRangeX + newWidth;
                recursionGuard = false;
            }
        }
    }

    Connections {
        target: qmlProfilerModelProxy
        onDataAvailable: {
            dataReady = true;
            increment = 0;
            for (var i = 0; i < qmlProfilerModelProxy.modelCount(); ++i)
                increment += qmlProfilerModelProxy.count(i);
            increment = Math.ceil(increment / eventsPerPass);
            offset = -1;
            requestPaint();
        }
    }

    Timer {
        id: paintTimer
        repeat: true
        running: offset >= 0
        interval: offset == 0 ? 1000 : 14 // Larger initial delay to avoid flickering on resize
        onTriggered: canvas.requestPaint()
    }

    // ***** slots
    onPaint: {
        var context = (canvas.context === null) ? getContext("2d") : canvas.context;

        Plotter.qmlProfilerModelProxy = qmlProfilerModelProxy;

        if (offset < 0) {
            context.reset();
            Plotter.drawGraph(canvas, context);
            if (dataReady) {
                Plotter.drawTimeBar(canvas, context);
                offset = 0;
            }
        } else if (offset < increment) {
            Plotter.drawData(canvas, context);
            ++offset;
        } else if (offset < 2 * increment) {
            Plotter.drawBindingLoops(canvas, context);
            ++offset;
        } else {
            offset = -1;
        }
    }

    // ***** child items
    MouseArea {
        anchors.fill: canvas
        function jumpTo(posX) {
            var newX = posX - rangeMover.rangeWidth / 2;
            if (newX < 0)
                newX = 0;
            if (newX + rangeMover.rangeWidth > canvas.width)
                newX = canvas.width - rangeMover.rangeWidth;

            if (newX < rangeMover.rangeLeft) {
                // Changing left border will change width, so precompute right border here.
                var right = newX + rangeMover.rangeWidth;
                rangeMover.rangeLeft = newX;
                rangeMover.rangeRight = right;
            } else if (newX > rangeMover.rangeLeft) {
                rangeMover.rangeRight = newX + rangeMover.rangeWidth;
                rangeMover.rangeLeft = newX;
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
        onRangeLeftChanged: canvas.updateRange()
        onRangeRightChanged: canvas.updateRange()
    }

    Rectangle {
        height: 1
        width: parent.width
        color: "#858585"
    }
}
