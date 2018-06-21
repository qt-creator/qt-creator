/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.10

Canvas {
    width: 100
    height: 100

    function drawCornerMarkers(ctx)
    {
        var cornerMarkerSize = 10;

        ctx.beginPath();

        // top left
        ctx.moveTo(0, 0.5);
        ctx.lineTo(cornerMarkerSize, 0.5);
        ctx.moveTo(0.5, 0);
        ctx.lineTo(0.5, cornerMarkerSize);

        // top right
        ctx.moveTo(width - cornerMarkerSize, 0.5);
        ctx.lineTo(width, 0.5);
        ctx.moveTo(width - 0.5, 0);
        ctx.lineTo(width - 0.5, cornerMarkerSize);

        // bottom left
        ctx.moveTo(0, height - 0.5);
        ctx.lineTo(cornerMarkerSize, height - 0.5);
        ctx.moveTo(0.5, height - cornerMarkerSize);
        ctx.lineTo(0.5, height);

        // bottom right
        ctx.moveTo(width - cornerMarkerSize, height - 0.5);
        ctx.lineTo(width, height - 0.5);
        ctx.moveTo(width - 0.5, height - cornerMarkerSize);
        ctx.lineTo(width - 0.5, height);

        ctx.stroke();
    }

    function drawMetrics(ctx)
    {
        ctx.font = '9px sans-serif';

        // top left
        ctx.textAlign = "left";
        ctx.textBaseline = "top";
        ctx.fillStyle = Qt.rgba(0, 0, 1, 1);
        ctx.fillText(x + '|' + y, 1, 1);

        // size
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";
        ctx.fillText(width + 'x' + height, width / 2, height / 2);

        // bottom right
        ctx.textAlign = "right";
        ctx.textBaseline = "bottom";
        ctx.fillText((x + width) + '|' + (y + height), width - 1, height - 1);
    }

    onPaint: {
        var ctx = getContext("2d");

        ctx.fillStyle = Qt.rgba(1, 0, 0, 0.2);
        ctx.fillRect(0, 0, width, height);

        drawCornerMarkers(ctx);
        drawMetrics(ctx);
    }
}
