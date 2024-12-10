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
// \file	EdgeStraightPath.qml
// \author	benoit@destrat.io
// \date	2022 10 02
//-----------------------------------------------------------------------------

import QtQuick
import QtQuick.Shapes

ShapePath {
    id: edgeShapePath

    // Set in EdgeTemplate.qml createObject() from global qanEdgeStraightPathComponent
    property var edgeTemplate: undefined
    property var edgeItem: edgeTemplate.edgeItem

    startX: edgeItem.p1.x
    startY: edgeItem.p1.y
    capStyle: ShapePath.FlatCap
    strokeWidth: edgeItem?.style?.lineWidth ?? 2
    strokeColor: edgeTemplate.color
    strokeStyle: edgeTemplate.dashed
    dashPattern: edgeItem?.style?.dashPattern ?? [2, 2]
    fillColor: Qt.rgba(0,0,0,0)
    PathLine {
        x: edgeItem.p2.x
        y: edgeItem.p2.y
    }
}
