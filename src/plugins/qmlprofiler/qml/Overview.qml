/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 2.0
import Monitor 1.0
import "Overview.js" as Plotter

Canvas2D {
    id: canvas

    // ***** properties
    height: 50
    property bool dataReady: false
    property variant startTime : 0
    property variant endTime : 0

    // ***** functions
    function clearDisplay()
    {
        dataReady = false;
        requestRedraw();
    }

    function updateRange() {
        var newStartTime = Math.round(rangeMover.x * qmlProfilerModelProxy.traceDuration() / width) + qmlProfilerModelProxy.traceStartTime();
        var newEndTime = Math.round((rangeMover.x + rangeMover.width) * qmlProfilerModelProxy.traceDuration() / width) + qmlProfilerModelProxy.traceStartTime();
        if (startTime !== newStartTime || endTime !== newEndTime) {
            zoomControl.setRange(newStartTime, newEndTime);
        }

    }

    // ***** connections to external objects
    Connections {
        target: zoomControl
        onRangeChanged: {
            if (qmlProfilerModelProxy) {
                startTime = zoomControl.startTime();
                endTime = zoomControl.endTime();
                var newRangeX = (startTime - qmlProfilerModelProxy.traceStartTime()) * width / qmlProfilerModelProxy.traceDuration();
                if (rangeMover.x !== newRangeX)
                    rangeMover.x = newRangeX;
                var newWidth = (endTime-startTime) * width / qmlProfilerModelProxy.traceDuration();
                if (rangeMover.width !== newWidth)
                    rangeMover.width = newWidth;
            }
        }
    }

    Connections {
        target: qmlProfilerModelProxy
        onDataAvailable: {
                dataReady = true;
                requestRedraw();
        }
    }


    // ***** slots
    onDrawRegion: {
        Plotter.qmlProfilerModelProxy = qmlProfilerModelProxy;
        if (dataReady) {
            Plotter.plot(canvas, ctxt, region);
        } else {
            Plotter.drawGraph(canvas, ctxt, region)    //just draw the background
        }
    }

    // ***** child items
    MouseArea {
        anchors.fill: canvas
        function jumpTo(posX) {
            var newX = posX - rangeMover.width/2;
            if (newX < 0)
                newX = 0;
            if (newX + rangeMover.width > canvas.width)
                newX = canvas.width - rangeMover.width;
            rangeMover.x = newX;
            updateRange();
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
    }

    Rectangle {
        height: 1
        width: parent.width
        color: "#858585"
    }
}
