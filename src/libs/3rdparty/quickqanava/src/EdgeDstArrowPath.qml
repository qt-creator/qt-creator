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
// \file	EdgeDstArrowPath.qml
// \author	benoit@destrat.io
// \date	2022 10 02
//-----------------------------------------------------------------------------

import QtQuick
import QtQuick.Shapes

import QuickQanava as Qan

ShapePath {
    property var edgeTemplate: undefined
    property var edgeItem: edgeTemplate.edgeItem

    strokeColor: edgeTemplate.color
    fillColor: edgeItem &&
               edgeItem.dstShape === Qan.EdgeStyle.ArrowOpen ? Qt.rgba(0.,0.,0.,0.) :
                                                               edgeTemplate.color
    strokeWidth: 2   // Fix to 2 to have "non sharp" edges
    // Adding lineWidth / 2. to vertices A1 (base top) and A3 (base bottom) to take into
    // account lineWidth in arrowLength
    readonly property real lineWidth: edgeItem?.style?.lineWidth / 2. ?? 1
    startX: edgeItem ? edgeItem.dstA1.x : 0
    startY: edgeItem ? edgeItem.dstA1.y - lineWidth: 0
    PathLine {
        x: edgeItem ? edgeItem.dstA3.x : 0
        y: edgeItem ? edgeItem.dstA3.y + lineWidth: 0
    }
    PathLine {
        x: edgeItem ? edgeItem.dstA2.x : 0
        y: edgeItem ? edgeItem.dstA2.y : 0
    }
    PathLine {
        x: edgeItem ? edgeItem.dstA1.x : 0
        y: edgeItem ? edgeItem.dstA1.y - lineWidth: 0
    }
}
