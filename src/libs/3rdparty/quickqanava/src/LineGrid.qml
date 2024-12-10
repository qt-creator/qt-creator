/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file	LineGrid.qml
// \author	benoit@destrat.io
// \date	2017 11 16
//-----------------------------------------------------------------------------

import QtQuick
import QuickQanava as Qan

Qan.AbstractLineGrid {
    id: lineGrid
    opacity: 0.9
    anchors.fill: parent

    gridScale: 25
    property int minorLineToDrawCount: 0
    property int majorLineToDrawCount: 0

    onRedrawLines: function(minorLineToDrawCount, majorLineToDrawCount) {
        lineGrid.minorLineToDrawCount = minorLineToDrawCount
        lineGrid.majorLineToDrawCount = majorLineToDrawCount
        gridCanvas.requestPaint()
    }

    Canvas {
        id: gridCanvas
        anchors.fill: parent
        visible: lineGrid.visible
        enabled: false  // Disable mouse event catching
        onPaint: {
            var ctx = gridCanvas.getContext('2d')
            ctx.reset();
            ctx.strokeStyle = lineGrid.thickColor

            // iterate over minor lines...
            if (minorLineToDrawCount <= lineGrid.minorLines.length) {
                ctx.lineWidth = 1.
                context.beginPath();
                for (var l = 0; l < minorLineToDrawCount; l++) {
                    let line = lineGrid.minorLines[l];
                    if (!line)
                        break;
                    ctx.moveTo(line.p1.x, line.p1.y)
                    ctx.lineTo(line.p2.x, line.p2.y)
                }
                context.stroke();
            }

            // iterate over major lines...
            if (majorLineToDrawCount <= lineGrid.majorLines.length) {
                //ctx.lineWidth = 2.
                ctx.strokeStyle = Qt.darker(lineGrid.thickColor, 1.3)
                context.beginPath();
                for (l = 0; l < majorLineToDrawCount; l++) {
                    let line = lineGrid.majorLines[l];
                    if (!line)
                        break;
                    ctx.moveTo(line.p1.x, line.p1.y)
                    ctx.lineTo(line.p2.x, line.p2.y)
                }
                context.stroke();
            }
        }
    } // Canvas gridCanvas
} // Qan.AbstractLineGrid2
