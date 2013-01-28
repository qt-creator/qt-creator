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

.pragma library

var qmlProfilerDataModel = 0;

//draw background of the graph
function drawGraph(canvas, ctxt, region)
{
    ctxt.fillStyle = "#eaeaea";
    ctxt.fillRect(0, 0, canvas.width, canvas.height);
}

//draw the actual data to be graphed
function drawData(canvas, ctxt, region)
{
    if ((!qmlProfilerDataModel) || qmlProfilerDataModel.count() == 0)
        return;

    var typeCount = 5;
    var width = canvas.width;
    var bump = 10;
    var height = canvas.height - bump;
    var blockHeight = height / typeCount;

    var spacing = width / qmlProfilerDataModel.traceDuration();

    var highest = [0,0,0,0,0]; // note: change if typeCount changes

    for (var ii = 0; ii < qmlProfilerDataModel.count(); ++ii) {

        var xx = (qmlProfilerDataModel.getStartTime(ii) -
                  qmlProfilerDataModel.traceStartTime()) * spacing;
        if (xx > region.x + region.width)
            continue;

        var eventWidth = qmlProfilerDataModel.getDuration(ii) * spacing;
        if (xx + eventWidth < region.x)
            continue;

        if (eventWidth < 1)
            eventWidth = 1;

        xx = Math.round(xx);
        var ty = qmlProfilerDataModel.getType(ii);
        if (xx + eventWidth > highest[ty]) {
            // special: animations
            if (ty === 0 && qmlProfilerDataModel.getAnimationCount(ii) >= 0) {
                var vertScale = qmlProfilerDataModel.getMaximumAnimationCount() -
                        qmlProfilerDataModel.getMinimumAnimationCount();
                if (vertScale < 1)
                    vertScale = 1;
                var fraction = (qmlProfilerDataModel.getAnimationCount(ii) -
                                qmlProfilerDataModel.getMinimumAnimationCount()) / vertScale;
                var eventHeight = blockHeight * (fraction * 0.85 + 0.15);
                var yy = bump + ty*blockHeight + blockHeight - eventHeight;

                var fpsFraction = qmlProfilerDataModel.getFramerate(ii) / 60.0;
                if (fpsFraction > 1.0)
                    fpsFraction = 1.0;
                ctxt.fillStyle = "hsl("+(fpsFraction*0.27+0.028)+",0.3,0.65)";
                ctxt.fillRect(xx, yy, eventWidth, eventHeight);
            } else {
                var hue = ( qmlProfilerDataModel.getEventId(ii) * 25 ) % 360;
                ctxt.fillStyle = "hsl("+(hue/360.0+0.001)+",0.3,0.65)";
                ctxt.fillRect(xx, bump + ty*blockHeight, eventWidth, blockHeight);
            }
            highest[ty] = xx+eventWidth;
        }
    }

    // binding loops
    ctxt.strokeStyle = "orange";
    ctxt.lineWidth = 2;
    var radius = 1;
    for (var ii = 0; ii < qmlProfilerDataModel.count(); ++ii) {
        if (qmlProfilerDataModel.getBindingLoopDest(ii) >= 0) {
            var xcenter = Math.round(qmlProfilerDataModel.getStartTime(ii) +
                                     qmlProfilerDataModel.getDuration(ii) -
                                     qmlProfilerDataModel.traceStartTime()) * spacing;
            var ycenter = Math.round(bump + qmlProfilerDataModel.getType(ii) *
                                     blockHeight + blockHeight/2);
            ctxt.arc(xcenter, ycenter, radius, 0, 2*Math.PI, true);
            ctxt.stroke();
        }
    }
}

function drawTimeBar(canvas, ctxt, region)
{
    if (!qmlProfilerDataModel)
        return;

    var width = canvas.width;
    var height = 10;
    var startTime = qmlProfilerDataModel.traceStartTime();
    var endTime = qmlProfilerDataModel.traceEndTime();

    var totalTime = qmlProfilerDataModel.traceDuration();
    var spacing = width / totalTime;

    var initialBlockLength = 120;
    var timePerBlock = Math.pow(2, Math.floor( Math.log( totalTime / width *
                       initialBlockLength ) / Math.LN2 ) );
    var pixelsPerBlock = timePerBlock * spacing;
    var pixelsPerSection = pixelsPerBlock / 5;
    var blockCount = width / pixelsPerBlock;

    var realStartTime = Math.floor(startTime/timePerBlock) * timePerBlock;
    var realStartPos = (startTime-realStartTime) * spacing;

    var timePerPixel = timePerBlock/pixelsPerBlock;

    ctxt.fillStyle = "#000000";
    ctxt.font = "6px sans-serif";

    ctxt.fillStyle = "#cccccc";
    ctxt.fillRect(0, 0, width, height);
    for (var ii = 0; ii < blockCount+1; ii++) {
        var x = Math.floor(ii*pixelsPerBlock - realStartPos);

        // block boundary
        ctxt.strokeStyle = "#525252";
        ctxt.beginPath();
        ctxt.moveTo(x, height/2);
        ctxt.lineTo(x, height);
        ctxt.stroke();

        // block time label
        ctxt.fillStyle = "#000000";
        var timeString = prettyPrintTime((ii+0.5)*timePerBlock + realStartTime);
        ctxt.textAlign = "center";
        ctxt.fillText(timeString, x + pixelsPerBlock/2, height/2 + 3);
    }

    ctxt.fillStyle = "#808080";
    ctxt.fillRect(0, height-1, width, 1);
}

function prettyPrintTime( t )
{
    if (t <= 0) return "0";
    if (t<1000) return t+" ns";
    t = t/1000;
    if (t<1000) return t+" μs";
    t = Math.floor(t/100)/10;
    if (t<1000) return t+" ms";
    t = Math.floor(t)/1000;
    if (t<60) return t+" s";
    var m = Math.floor(t/60);
    t = Math.floor(t - m*60);
    return m+"m"+t+"s";
}

function plot(canvas, ctxt, region)
{
    drawGraph(canvas, ctxt, region);
    drawData(canvas, ctxt, region);
    drawTimeBar(canvas, ctxt, region);

}
