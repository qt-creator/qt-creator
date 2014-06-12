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

.pragma library

var qmlProfilerModelProxy = 0;

//draw background of the graph
function drawGraph(canvas, ctxt)
{
    ctxt.fillStyle = "#eaeaea";
    ctxt.fillRect(0, 0, canvas.width, canvas.height);
}

//draw the actual data to be graphed
function drawData(canvas, ctxt)
{
    if ((!qmlProfilerModelProxy) || qmlProfilerModelProxy.count() === 0)
        return;

    var width = canvas.width;
    var bump = 10;
    var height = canvas.height - bump;

    var typeCount = qmlProfilerModelProxy.visibleCategories();
    var blockHeight = height / typeCount;

    var spacing = width / qmlProfilerModelProxy.traceDuration();

    var modelRowStart = 0;

    for (var modelIndex = 0; modelIndex < qmlProfilerModelProxy.modelCount(); ++modelIndex) {
        for (var ii = canvas.offset; ii < qmlProfilerModelProxy.count(modelIndex);
             ii += canvas.increment) {

            var xx = (qmlProfilerModelProxy.getStartTime(modelIndex,ii) -
                      qmlProfilerModelProxy.traceStartTime()) * spacing;

            var eventWidth = qmlProfilerModelProxy.getDuration(modelIndex,ii) * spacing;

            if (eventWidth < 1)
                eventWidth = 1;

            xx = Math.round(xx);

            var rowNumber = modelRowStart + qmlProfilerModelProxy.getEventCategoryInModel(modelIndex, ii);

            var itemHeight = qmlProfilerModelProxy.getHeight(modelIndex,ii) * blockHeight;
            var yy = (rowNumber + 1) * blockHeight - itemHeight ;

            ctxt.fillStyle = qmlProfilerModelProxy.getColor(modelIndex, ii);
            ctxt.fillRect(xx, bump + yy, eventWidth, itemHeight);
        }
        modelRowStart += qmlProfilerModelProxy.categoryCount(modelIndex);
    }

    // binding loops
    ctxt.strokeStyle = "orange";
    ctxt.lineWidth = 2;
    var radius = 1;
    modelRowStart = 0;
    for (modelIndex = 0; modelIndex < qmlProfilerModelProxy.modelCount(); ++modelIndex) {
        for (ii = canvas.offset; ii < qmlProfilerModelProxy.count(modelIndex);
             ii += canvas.increment) {
            if (qmlProfilerModelProxy.getBindingLoopDest(modelIndex,ii) >= 0) {
                var xcenter = Math.round(qmlProfilerModelProxy.getStartTime(modelIndex,ii) +
                                         qmlProfilerModelProxy.getDuration(modelIndex,ii) -
                                         qmlProfilerModelProxy.traceStartTime()) * spacing;
                var ycenter = Math.round(bump + (modelRowStart +
                                         qmlProfilerModelProxy.getEventCategoryInModel(modelIndex, ii)) *
                                         blockHeight + blockHeight/2);
                ctxt.beginPath();
                ctxt.arc(xcenter, ycenter, radius, 0, 2*Math.PI, true);
                ctxt.stroke();
            }
        }
        modelRowStart += qmlProfilerModelProxy.categoryCount(modelIndex);
    }

}

function drawTimeBar(canvas, ctxt)
{
    if (!qmlProfilerModelProxy)
        return;

    var width = canvas.width;
    var height = 10;
    var startTime = qmlProfilerModelProxy.traceStartTime();
    var endTime = qmlProfilerModelProxy.traceEndTime();

    var totalTime = qmlProfilerModelProxy.traceDuration();
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
