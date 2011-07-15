/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

.pragma library

var ranges = [ ];
var xmargin = 0;
var ymargin = 0;
var nestingDepth = [];

var names = [ "Painting", "Compiling", "Creating", "Binding", "Handling Signal"]
//### need better way to manipulate color from QML. In the meantime, these need to be kept in sync.
var colors = [ "#99CCB3", "#99CCCC", "#99B3CC", "#9999CC", "#CC99B3", "#CC99CC", "#CCCC99", "#CCB399" ];
var origColors = [ "#99CCB3", "#99CCCC", "#99B3CC", "#9999CC", "#CC99B3", "#CC99CC", "#CCCC99", "#CCB399" ];
var xRayColors = [ "#6699CCB3", "#6699CCCC", "#6699B3CC", "#669999CC", "#66CC99B3", "#66CC99CC", "#66CCCC99", "#66CCB399" ];

function reset()
{
    ranges = [];
    xmargin = 0;
    ymargin = 0;
    nestingDepth = [];
}

//draw background of the graph
function drawGraph(canvas, ctxt, region)
{
    var grad = ctxt.createLinearGradient(0, 0, 0, canvas.canvasSize.height);
    grad.addColorStop(0,   '#fff');
    grad.addColorStop(1, '#ccc');
    ctxt.fillStyle = grad;

    ctxt.fillRect(0, 0, canvas.canvasSize.width + xmargin, canvas.canvasSize.height - ymargin);
}

//draw the actual data to be graphed
function drawData(canvas, ctxt, region)
{
    if (ranges.length == 0)
        return;

    var width = canvas.canvasSize.width - xmargin;
    var height = canvas.height - ymargin;

    var sumValue = ranges[ranges.length - 1].start + ranges[ranges.length - 1].duration - ranges[0].start;
    var spacing = width / sumValue;

    ctxt.fillStyle = "rgba(0,0,0,1)";
    var highest = [0,0,0,0,0];

    //### only draw those in range
    for (var ii = 0; ii < ranges.length; ++ii) {

        var xx = (ranges[ii].start - ranges[0].start) * spacing + xmargin;
        if (xx > region.x + region.width)
            continue;

        var size = ranges[ii].duration * spacing;
        if (xx + size < region.x)
            continue;

        if (size < 0.5)
            size = 0.5;

        xx = Math.round(xx);
        if (xx + size > highest[ranges[ii].type]) {
            ctxt.fillRect(xx, ranges[ii].type*10, size, 10);
            highest[ranges[ii].type] = xx+size;
        }
    }
}

function plot(canvas, ctxt, region)
{
    drawGraph(canvas, ctxt, region);
    drawData(canvas, ctxt, region);
}

function xScale(canvas)
{
    if (ranges.length === 0)
        return;

    var width = canvas.canvasSize.width - xmargin;

    var sumValue = ranges[ranges.length - 1].start + ranges[ranges.length - 1].duration - ranges[0].start;
    var spacing = sumValue / width;
    return spacing;
}
