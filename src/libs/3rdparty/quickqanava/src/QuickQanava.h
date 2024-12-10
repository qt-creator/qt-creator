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
// \file	QuickQanava.h
// \author	benoit@destrat.io
// \date	2016 02 04
//-----------------------------------------------------------------------------

#pragma once

// QuickContainers headers
#include "./quickcontainers/QuickContainers.h"

// Qt header
#include <QQmlEngine>

// QuickQanava headers
#include "./qanEdge.h"
#include "./qanEdgeItem.h"
#include "./qanNode.h"
#include "./qanNodeItem.h"
#include "./qanPortItem.h"
#include "./qanConnector.h"
#include "./qanGroup.h"
#include "./qanGroupItem.h"
#include "./qanTableGroupItem.h"
#include "./qanTableBorder.h"
#include "./qanGraph.h"
#include "./qanNavigable.h"
#include "./qanGrid.h"
#include "./qanLineGrid.h"
#include "./qanGraphView.h"
#include "./qanStyle.h"
#include "./qanStyleManager.h"
#include "./qanBottomRightResizer.h"
#include "./qanRightResizer.h"
#include "./qanBottomResizer.h"
#include "./qanNavigablePreview.h"
#include "./qanAnalysisTimeHeatMap.h"
#include "./qanTreeLayouts.h"

struct QuickQanava {
    static void initialize(QQmlEngine* engine) {
        QuickContainers::initialize();

        if (engine != nullptr) {
            engine->rootContext()->setContextProperty("defaultNodeStyle", QVariant::fromValue(qan::Node::style()));
            engine->rootContext()->setContextProperty("defaultEdgeStyle", QVariant::fromValue(qan::Edge::style()));
            engine->rootContext()->setContextProperty("defaultGroupStyle", QVariant::fromValue(qan::Group::style()));

            engine->rootContext()->setContextProperty("qanEdgeStraightPathComponent", new QQmlComponent(engine, "qrc:/QuickQanava/EdgeStraightPath.qml", engine));
            engine->rootContext()->setContextProperty("qanEdgeOrthoPathComponent", new QQmlComponent(engine, "qrc:/QuickQanava/EdgeOrthoPath.qml", engine));
            engine->rootContext()->setContextProperty("qanEdgeCurvedPathComponent", new QQmlComponent(engine, "qrc:/QuickQanava/EdgeCurvedPath.qml", engine));

            engine->rootContext()->setContextProperty("qanEdgeSrcArrowPathComponent", new QQmlComponent(engine, "qrc:/QuickQanava/EdgeSrcArrowPath.qml", engine));
            engine->rootContext()->setContextProperty("qanEdgeSrcCirclePathComponent", new QQmlComponent(engine, "qrc:/QuickQanava/EdgeSrcCirclePath.qml", engine));
            engine->rootContext()->setContextProperty("qanEdgeSrcRectPathComponent", new QQmlComponent(engine, "qrc:/QuickQanava/EdgeSrcRectPath.qml", engine));

            engine->rootContext()->setContextProperty("qanEdgeDstArrowPathComponent", new QQmlComponent(engine, "qrc:/QuickQanava/EdgeDstArrowPath.qml", engine));
            engine->rootContext()->setContextProperty("qanEdgeDstCirclePathComponent", new QQmlComponent(engine, "qrc:/QuickQanava/EdgeDstCirclePath.qml", engine));
            engine->rootContext()->setContextProperty("qanEdgeDstRectPathComponent", new QQmlComponent(engine, "qrc:/QuickQanava/EdgeDstRectPath.qml", engine));
        }
    } // initialize()
};

namespace qan { // ::qan

} // ::qan
