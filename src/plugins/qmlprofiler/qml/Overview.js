/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

.pragma library

var qmlEventList = 0;

//draw background of the graph
function drawGraph(canvas, ctxt, region)
{
    var grad = ctxt.createLinearGradient(0, 0, 0, canvas.height);
    grad.addColorStop(0,   '#fff');
    grad.addColorStop(1, '#ccc');
    ctxt.fillStyle = grad;

    ctxt.fillRect(0, 0, canvas.width, canvas.height);
}

//draw the actual data to be graphed
function drawData(canvas, ctxt, region)
{
    if ((!qmlEventList) || qmlEventList.count() == 0)
        return;

    var width = canvas.width;
    var height = canvas.height;

    var spacing = width / qmlEventList.traceDuration();

    ctxt.fillStyle = "rgba(0,0,0,1)";
    var highest = [0,0,0,0,0];

    // todo: use "region" in the timemarks?
    //### only draw those in range
    for (var ii = 0; ii < qmlEventList.count(); ++ii) {

        var xx = (qmlEventList.getStartTime(ii) - qmlEventList.traceStartTime()) * spacing;
        if (xx > region.x + region.width)
            continue;

        var size = qmlEventList.getDuration(ii) * spacing;
        if (xx + size < region.x)
            continue;

        if (size < 0.5)
            size = 0.5;

        xx = Math.round(xx);
        var ty = qmlEventList.getType(ii);
        if (xx + size > highest[ty]) {
            ctxt.fillRect(xx, ty*10, size, 10);
            highest[ty] = xx+size;
        }
    }
}

function plot(canvas, ctxt, region)
{
    drawGraph(canvas, ctxt, region);
    drawData(canvas, ctxt, region);
}
