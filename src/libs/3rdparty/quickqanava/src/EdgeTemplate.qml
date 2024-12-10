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
// \file	EdgeTemplate.qml
// \author	benoit@destrat.io
// \date	2017 11 17
//-----------------------------------------------------------------------------

import QtQuick
import QtQuick.Shapes

import QuickQanava as Qan

Item {
    id: edgeTemplate
    property var edgeItem: undefined

    property color color: edgeItem?.style?.lineColor ?? Qt.rgba(0.,0.,0.,1.)
    // Allow direct bypass of style
    property var    lineType: edgeItem?.style?.lineType ?? Qan.EdgeStyle.Straight
    property var    dashed  : edgeItem?.style?.dashed ? ShapePath.DashLine : ShapePath.SolidLine

    visible: edgeItem.visible && !edgeItem.hidden

    Shape {
        id: dstShape
        visible: dstShapeType !== Qan.EdgeStyle.None
        transformOrigin: Item.TopLeft
        rotation: edgeItem.dstAngle
        x: edgeItem.p2.x
        y: edgeItem.p2.y
        preferredRendererType: Shape.CurveRenderer

        property var dstArrow : undefined
        property var dstCircle: undefined
        property var dstRect  : undefined
        property var dstShapeType: edgeItem.dstShape
        onDstShapeTypeChanged: {
            switch (dstShapeType) {
            case Qan.EdgeStyle.None:
                break;
            case Qan.EdgeStyle.Arrow:       // falltrought
            case Qan.EdgeStyle.ArrowOpen:
                if (dstCircle) dstCircle.destroy()
                if (dstRect) dstRect.destroy()
                dstShape.data = dstArrow = qanEdgeDstArrowPathComponent.createObject(dstShape, {edgeTemplate: edgeTemplate});
                break;
            case Qan.EdgeStyle.Circle:      // falltrought
            case Qan.EdgeStyle.CircleOpen:
                if (dstArrow) dstArrow.destroy()
                if (dstRect) dstRect.destroy()
                dstShape.data = dstCircle = qanEdgeDstCirclePathComponent.createObject(dstShape, {edgeTemplate: edgeTemplate})
                break;
            case Qan.EdgeStyle.Rect:        // falltrought
            case Qan.EdgeStyle.RectOpen:
                if (dstArrow) dstArrow.destroy()
                if (dstCircle) dstCircle.destroy()
                dstShape.data = dstRect = qanEdgeDstRectPathComponent.createObject(dstShape, {edgeTemplate: edgeTemplate})
                break;
            }
        }
    }  // Shape: dstShape

    Shape {
        id: srcShape
        visible: srcShapeType !== Qan.EdgeStyle.None

        preferredRendererType: Shape.CurveRenderer
        transformOrigin: Item.TopLeft
        rotation: edgeItem.srcAngle
        x: edgeItem.p1.x
        y: edgeItem.p1.y

        property var srcArrow : undefined
        property var srcCircle: undefined
        property var srcRect  : undefined
        property var srcShapeType: edgeItem.srcShape
        onSrcShapeTypeChanged: {
            switch (srcShapeType) {
            case Qan.EdgeStyle.None:
                break;
            case Qan.EdgeStyle.Arrow:       // falltrought
            case Qan.EdgeStyle.ArrowOpen:
                if (srcCircle) srcCircle.destroy()
                if (srcRect) srcRect.destroy()
                srcShape.data = srcArrow = qanEdgeSrcArrowPathComponent.createObject(srcShape, {edgeTemplate: edgeTemplate});
                break;
            case Qan.EdgeStyle.Circle:      // falltrought
            case Qan.EdgeStyle.CircleOpen:
                if (srcArrow) srcArrow.destroy()
                if (srcRect) srcRect.destroy()
                srcShape.data = srcCircle = qanEdgeSrcCirclePathComponent.createObject(dstShape, {edgeTemplate: edgeTemplate})
                break;
            case Qan.EdgeStyle.Rect:        // falltrought
            case Qan.EdgeStyle.RectOpen:
                if (srcArrow) srcArrow.destroy()
                if (srcCircle) srcCircle.destroy()
                srcShape.data = srcRect = qanEdgeSrcRectPathComponent.createObject(dstShape, {edgeTemplate: edgeTemplate})
                break;
            }
        }
    }  // Shape: srcShape

    Shape {
        id: edgeSelectionShape
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        visible: edgeItem.visible &&
                 !edgeItem.hidden &&
                 edgeItem.selected     // Not very efficient, use a Loader there...
        property var curvedLine : undefined
        property var straightLine : undefined
        property var orthoLine : undefined
        property var lineType: edgeTemplate.lineType
        property var lineWidth: edgeItem?.style?.lineWidth + 2. ?? 4.
        property var lineColor: edgeItem &&
                                edgeItem.graph ? edgeItem.graph.selectionColor :
                                                 Qt.rgba(0.1176, 0.5647, 1., 1.)  // dodgerblue=rgb(30, 144, 255)
        onLineTypeChanged: updateSelectionShape()
        onVisibleChanged: updateSelectionShape()
        function updateSelectionShape() {
            if (!visible)
                return
            switch (lineType) {
            case Qan.EdgeStyle.Undefined:   // falltrought
            case Qan.EdgeStyle.Straight:
                if (orthoLine) orthoLine.destroy()
                if (curvedLine) curvedLine.destroy()
                edgeSelectionShape.data = straightLine = qanEdgeStraightPathComponent.createObject(edgeSelectionShape, {
                                                                                              edgeTemplate: edgeTemplate,
                                                                                              strokeWidth: lineWidth,
                                                                                              strokeColor: lineColor
                                                                                          });
                break;
            case Qan.EdgeStyle.Ortho:
                if (straightLine) straightLine.destroy()
                if (curvedLine) curvedLine.destroy()
                edgeSelectionShape.data = orthoLine = qanEdgeOrthoPathComponent.createObject(edgeSelectionShape, {
                                                                                                 edgeTemplate: edgeTemplate,
                                                                                                 strokeWidth: lineWidth,
                                                                                                 strokeColor: lineColor
                                                                                             })
                break;
            case Qan.EdgeStyle.Curved:
                if (straightLine) straightLine.destroy()
                if (orthoLine) orthoLine.destroy()
                edgeSelectionShape.data = curvedLine = qanEdgeCurvedPathComponent.createObject(edgeSelectionShape, {
                                                                                                   edgeTemplate: edgeTemplate,
                                                                                                   strokeWidth: lineWidth,
                                                                                                   strokeColor: lineColor
                                                                                               })
                break;
            }
        }
    }  // Shape: edgeSelectionShape

    Shape {
        id: edgeShape
        anchors.fill: parent
        visible: edgeItem.visible && !edgeItem.hidden
        // Note 20240815: Do not pay the curve renderer cost for horiz/vert ortho lines
        preferredRendererType: lineType === Qan.EdgeStyle.Ortho ? Qan.EdgeStyle.Ortho : Shape.CurveRenderer
        property var curvedLine : undefined
        property var straightLine : undefined
        property var orthoLine : undefined
        property var lineType: edgeTemplate.lineType
        onLineTypeChanged: {
            switch (lineType) {
            case Qan.EdgeStyle.Undefined:   // falltrought
            case Qan.EdgeStyle.Straight:
                if (orthoLine) orthoLine.destroy()
                if (curvedLine) curvedLine.destroy()
                edgeShape.data = straightLine = qanEdgeStraightPathComponent.createObject(edgeShape, {edgeTemplate: edgeTemplate});
                break;
            case Qan.EdgeStyle.Ortho:
                if (straightLine) straightLine.destroy()
                if (curvedLine) curvedLine.destroy()
                edgeShape.data = orthoLine = qanEdgeOrthoPathComponent.createObject(edgeShape, {edgeTemplate: edgeTemplate})
                break;
            case Qan.EdgeStyle.Curved:
                if (straightLine) straightLine.destroy()
                if (orthoLine) orthoLine.destroy()
                edgeShape.data = curvedLine = qanEdgeCurvedPathComponent.createObject(edgeShape, {edgeTemplate: edgeTemplate})
                break;
            }
        }
    }  // Shape: edgeShape
}  // Item: edgeTemplate
