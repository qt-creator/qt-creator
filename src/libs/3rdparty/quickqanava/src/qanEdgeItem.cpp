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
// \file	qanEdgeItem.cpp
// \author	benoit@destrat.io
// \date	2017 03 02
//-----------------------------------------------------------------------------

// Qt headers
#include <QtGlobal>
#include <QBrush>
#include <QPainter>

// QuickQanava headers
#include "./qanUtils.h"
#include "./qanEdgeItem.h"
#include "./qanNodeItem.h"      // Resolve forward declaration
#include "./qanGroupItem.h"
#include "./qanGraph.h"
#include "./qanEdgeDraggableCtrl.h"

#include "./bezier/include/bezier.h"

namespace qan { // ::qan

/* Edge Object Management *///-------------------------------------------------
EdgeItem::EdgeItem(QQuickItem* parent) :
    QQuickItem{parent}
{
    setStyle(qan::Edge::style(nullptr));
    setObjectName(QStringLiteral("qan::EdgeItem"));

    setParentItem(parent);
    setAntialiasing(true);
    setFlag(QQuickItem::ItemHasContents, true);
    setAcceptedMouseButtons(Qt::RightButton | Qt::LeftButton);
    setAcceptDrops(true);
    setVisible(false);  // Invisible until there is a valid src/dst

    _draggableCtrl = std::unique_ptr<AbstractDraggableCtrl>{std::make_unique<qan::EdgeDraggableCtrl>()};
    const auto edgeDraggableCtrl = static_cast<qan::EdgeDraggableCtrl*>(_draggableCtrl.get());
    edgeDraggableCtrl->setTargetItem(this);

    connect(this,   &qan::EdgeItem::widthChanged,
            this,   &qan::EdgeItem::onWidthChanged);
    connect(this,   &qan::EdgeItem::heightChanged,
            this,   &qan::EdgeItem::onHeightChanged);
}

auto    EdgeItem::getEdge() noexcept -> qan::Edge* { return _edge.data(); }
auto    EdgeItem::getEdge() const noexcept -> const qan::Edge* { return _edge.data(); }
auto    EdgeItem::setEdge(qan::Edge* edge) noexcept -> void
{
    if (_edge != edge) {
        _edge = edge;
        edge->setItem(this);
        const auto edgeDraggableCtrl = static_cast<EdgeDraggableCtrl*>(_draggableCtrl.get());
        edgeDraggableCtrl->setTarget(edge);
    }
}

auto    EdgeItem::getGraph() const -> const qan::Graph*
{
    if (_graph)
        return _graph;
    return _edge ? _edge->getGraph() : nullptr;
}
auto    EdgeItem::getGraph() -> qan::Graph*
{
    if (_graph)
        return _graph;
    return _edge ? _edge->getGraph() : nullptr;
}
auto    EdgeItem::setGraph(qan::Graph* graph) -> void
{
    _graph = graph; emit graphChanged();
    qan::Selectable::configure(this, graph);
}
//-----------------------------------------------------------------------------

/* Edge Topology Management *///-----------------------------------------------
auto    EdgeItem::setSourceItem(qan::NodeItem* source) -> void
{
    if (source == nullptr)
        return;

    // Connect dst x and y monitored properties change notify signal to slot updateEdge()
    QMetaMethod updateItemSlot = metaObject()->method( metaObject()->indexOfSlot( "updateItemSlot()" ) );
    if ( updateItemSlot.isValid() ) {  // Connect src and dst x and y monitored properties change notify signal to slot updateItemSlot()
        auto srcMetaObj = source->metaObject();
        QMetaProperty srcX      = srcMetaObj->property(srcMetaObj->indexOfProperty("x"));
        QMetaProperty srcY      = srcMetaObj->property(srcMetaObj->indexOfProperty("y"));
        QMetaProperty srcZ      = srcMetaObj->property(srcMetaObj->indexOfProperty("z"));
        QMetaProperty srcWidth  = srcMetaObj->property(srcMetaObj->indexOfProperty("width"));
        QMetaProperty srcHeight = srcMetaObj->property(srcMetaObj->indexOfProperty("height"));
        if (!srcX.isValid() || !srcX.hasNotifySignal()) {
            qWarning() << "qan::EdgeItem::setSourceItem(): Error: can't access source x property.";
            return;
        }
        if (!srcY.isValid() || !srcY.hasNotifySignal()) {
            qWarning() << "qan::EdgeItem::setSourceItem(): Error: can't access source y property.";
            return;
        }
        if (!srcWidth.isValid() || !srcWidth.hasNotifySignal()) {
            qWarning() << "qan::EdgeItem::setSourceItem(): Error: can't access source width property.";
            return;
        }
        if (!srcHeight.isValid() || !srcHeight.hasNotifySignal()) {
            qWarning() << "qan::EdgeItem::setSourceItem(): Error: can't access source height property.";
            return;
        }
        connect(source, srcX.notifySignal(),       this, updateItemSlot);
        connect(source, srcY.notifySignal(),       this, updateItemSlot);
        connect(source, srcZ.notifySignal(),       this, updateItemSlot);
        connect(source, srcWidth.notifySignal(),   this, updateItemSlot);
        connect(source, srcHeight.notifySignal(),  this, updateItemSlot);
        _sourceItem = source;
        emit sourceItemChanged();
        if (source->z() < z())
            setZ(source->z() - 0.5);
        updateItem();
    }
}

auto    EdgeItem::setDestinationItem(qan::NodeItem* destination) -> void
{
    if (destination != _destinationItem) {
        configureDestinationItem(destination);
        _destinationItem = destination;
        emit destinationItemChanged();
    }
    updateItem();
}

void    EdgeItem::configureDestinationItem(QQuickItem* item)
{
    if (item == nullptr)
        return;

    // Connect dst x and y monitored properties change notify signal to slot updateItemSlot()
    QMetaMethod updateItemSlot = metaObject()->method(metaObject()->indexOfSlot("updateItemSlot()"));
    if (!updateItemSlot.isValid()) {
        qWarning() << "qan::EdgeItem::setDestinationItem(): Error: no access to edge updateItem slot.";
        return;
    }
    auto dstMetaObj = item->metaObject();
    QMetaProperty dstX      = dstMetaObj->property(dstMetaObj->indexOfProperty("x"));
    QMetaProperty dstY      = dstMetaObj->property(dstMetaObj->indexOfProperty("y"));
    QMetaProperty dstZ      = dstMetaObj->property(dstMetaObj->indexOfProperty("z"));
    QMetaProperty dstWidth  = dstMetaObj->property(dstMetaObj->indexOfProperty("width"));
    QMetaProperty dstHeight = dstMetaObj->property(dstMetaObj->indexOfProperty("height"));
    if (!dstX.isValid() || !dstX.hasNotifySignal()) {
        qWarning() << "qan::EdgeItem::setDestinationItem(): Error: can't access source x property.";
        return;
    }
    if (!dstY.isValid() || !dstY.hasNotifySignal()) {
        qWarning() << "qan::EdgeItem::setDestinationItem(): Error: can't access source y property.";
        return;
    }
    if (!dstWidth.isValid() || !dstWidth.hasNotifySignal()) {
        qWarning() << "qan::EdgeItem::setDestinationItem(): Error: can't access source width property.";
        return;
    }
    if (!dstHeight.isValid() || !dstHeight.hasNotifySignal()) {
        qWarning() << "qan::EdgeItem::setDestinationItem(): Error: can't access source height property.";
        return;
    }
    connect(item, dstX.notifySignal(),       this, updateItemSlot);
    connect(item, dstY.notifySignal(),       this, updateItemSlot);
    connect(item, dstZ.notifySignal(),       this, updateItemSlot);
    connect(item, dstWidth.notifySignal(),   this, updateItemSlot);
    connect(item, dstHeight.notifySignal(),  this, updateItemSlot);
    if (item->z() < z())
        setZ(item->z() - 0.5);
}
//-----------------------------------------------------------------------------

/* Edge Drawing Management *///------------------------------------------------
void    EdgeItem::setHidden(bool hidden) noexcept
{
    if (hidden != _hidden) {
        _hidden = hidden;
        emit hiddenChanged();
    }
}

void    EdgeItem::setArrowSize( qreal arrowSize ) noexcept
{
    if (!qFuzzyCompare(1. + arrowSize, 1. + _arrowSize)) {
        _arrowSize = arrowSize;
        emit arrowSizeChanged();
        updateItem();
    }
}

auto    EdgeItem::setSrcShape(ArrowShape srcShape) noexcept -> bool
{
    if (_srcShape != srcShape) {
        _srcShape = srcShape;
        emit srcShapeChanged();
        updateItem();
        return true;
    }
    return false;
}

auto    EdgeItem::setDstShape(ArrowShape dstShape) noexcept -> bool
{
    if (_dstShape != dstShape) {
        _dstShape = dstShape;
        emit dstShapeChanged();
        updateItem();
        return true;
    }
    return false;
}

void    EdgeItem::updateItem() noexcept
{
    // Algorithm:
        // Generate cache step by step until it become invalid.
        // 1. Generate                 srcBr / dstBr / srcBrCenter / dstBrCenter / z
        // 2. generate edge ends:      P1 / P2
        // 3. generate control points: C1 / C2
    auto cache = generateGeometryCache();       // 1.
    if (cache.isValid()) {
        switch (cache.lineType) {               // 2.
        case qan::EdgeStyle::LineType::Undefined:       // [[fallthrough]] default to Straight
        case qan::EdgeStyle::LineType::Straight: generateStraightEnds(cache); break;
        case qan::EdgeStyle::LineType::Curved:   generateStraightEnds(cache); break;
        case qan::EdgeStyle::LineType::Ortho:    generateOrthoEnds(cache);    break;
        }
        if (cache.isValid()) {
            switch (cache.lineType) {           // 3.
            case qan::EdgeStyle::LineType::Undefined:   // [[fallthrough]] default to Straight
            case qan::EdgeStyle::LineType::Straight: /* Nil */                           break;
            case qan::EdgeStyle::LineType::Curved:   generateCurvedControlPoints(cache); break;
            case qan::EdgeStyle::LineType::Ortho:    /* Nil */                           break; // Ortho C1 control point is generated in generateOrthoEnds()
            }
            generateArrowGeometry(cache);
            generateLabelPosition(cache);
        }
    }

    // A valid geometry has been generated, generate a bounding box for edge,
    // and project all geometry in edge CS.
    if (cache.isValid())
        applyGeometry(cache);
    else
        setHidden(true);
}

EdgeItem::GeometryCache::GeometryCache(GeometryCache&& rha) :
    valid{rha.valid},
    lineType{rha.lineType},
    z{rha.z},
    hidden{rha.hidden},
    srcBs{std::move(rha.srcBs)},    dstBs{std::move(rha.dstBs)},
    srcBr{std::move(rha.srcBr)},    dstBr{std::move(rha.dstBr)},
    srcBrCenter{std::move(rha.srcBrCenter)},
    dstBrCenter{std::move(rha.dstBrCenter)},
    p1{std::move(rha.p1)},          p2{std::move(rha.p2)},
    dstA1{std::move(rha.dstA1)},
    dstA2{std::move(rha.dstA2)},
    dstA3{std::move(rha.dstA3)},
    dstAngle{rha.dstAngle},
    srcA1{std::move(rha.srcA1)},
    srcA2{std::move(rha.srcA2)},
    srcA3{std::move(rha.srcA3)},
    srcAngle{rha.srcAngle},
    c1{std::move(rha.c1)},          c2{std::move(rha.c2)},
    labelPosition{std::move(rha.labelPosition)}
{
    srcItem.swap(rha.srcItem);
    dstItem.swap(rha.dstItem);
    rha.valid = false;
}

EdgeItem::GeometryCache  EdgeItem::generateGeometryCache() const noexcept
{
    // PRECONDITIONS:
        // _sourceItem can't be nullptr
        // _destinationItem can't be nullptr
    if (!_sourceItem)
        return EdgeItem::GeometryCache{};
    if (!_destinationItem)
        return EdgeItem::GeometryCache{};

    EdgeItem::GeometryCache cache{};
    cache.valid = false;

    const QQuickItem* graphContainerItem = (getGraph() != nullptr ? getGraph()->getContainerItem() :
                                                                    nullptr);
    if (graphContainerItem == nullptr) {
        qWarning() << "qan::EdgeItem::generateEdgeGeometry(): No access to valid graph container item.";
        return cache;    // Return INVALID geometry cache
    }

    const QQuickItem* srcItem = _sourceItem.data();
    if (srcItem == nullptr) {
        qWarning() << "qan::EdgeItem::generateEdgeGeometry(): No valid source item.";
        return cache;    // Return INVALID geometry cache
    }

    qan::GroupItem* srcGroupItem = nullptr;
    if (srcItem != nullptr) {
        const auto srcNode = static_cast<qan::Node*>(_sourceItem->getNode());
        if (srcNode != nullptr) {
            const auto srcNodeGroup = qobject_cast<qan::Group*>(srcNode->get_group());
            if (srcNodeGroup != nullptr)
                srcGroupItem = srcNodeGroup->getGroupItem();
        }
    }

    qan::NodeItem*  dstNodeItem = qobject_cast<qan::NodeItem*>(_destinationItem);
    if (dstNodeItem == nullptr &&
        _edge) {
        qan::Node*  dstNode = static_cast<qan::Node*>(_edge->get_dst());
        if (dstNode != nullptr)
            dstNodeItem = dstNode->getItem();
    }

    // Initialize dstGroupItem: eventual group item for dst item
    qan::GroupItem* dstGroupItem = nullptr;
    if (dstNodeItem != nullptr &&
        dstNodeItem->getNode() != nullptr) {
        auto dstNodeGroup = qobject_cast<qan::Group*>(dstNodeItem->getNode()->get_group());
        if (dstNodeGroup != nullptr)
            dstGroupItem = dstNodeGroup->getGroupItem();
    }

    // Finally, generate dstItem wich either dstNodeItem or dstEdgeItem
    const QQuickItem* dstItem = dstNodeItem;

    // Check that we have a valid source and destination Quick Item
    if (srcItem == nullptr || dstItem == nullptr)
        return cache;        // Otherwise, return an invalid cache
    cache.srcItem = srcItem;
    cache.dstItem = dstItem;

    // If the edge "src" or "dst" is inside a collapsed group, generate invalid cache, it will automatically be
    // hidden
    if (srcGroupItem != nullptr &&
        srcGroupItem->getCollapsed())
        return cache;   // Return invalid cache
    if (dstGroupItem != nullptr &&
        dstGroupItem->getCollapsed())
        return cache;   // Return invalid cache

    // Generate bounding shapes for source and destination in global CS
    {
        if (_sourceItem != nullptr) {        // Generate source bounding shape polygon
            const auto srcBs = _sourceItem->getBoundingShape();
            cache.srcBs.resize(srcBs.size());
            int p = 0;
            for (const auto& point: srcBs)
                cache.srcBs[p++] = _sourceItem->mapToItem(graphContainerItem, point);
        }
        // Generate destination bounding shape polygon
        if (dstNodeItem != nullptr) {        // Regular Node -> Node edge
            // Update edge z to source or destination maximum x
            const auto dstBs = dstNodeItem->getBoundingShape();
            cache.dstBs.resize(dstBs.size());
            int p = 0;
            for (const auto& point: dstBs)
                cache.dstBs[p++] = dstNodeItem->mapToItem(graphContainerItem, point);
        }
    }

    // Verify source and destination bounding shapes
    if (cache.srcBs.isEmpty() ||
        cache.dstBs.isEmpty()) {
        qWarning() << "qan::EdgeItem::generateEdgeGeometry(): Invalid source or destination bounding shape.";
        return cache;    // Return INVALID geometry cache
    }

    // Generate edge geometry Z according to actual src and dst z
    const qreal srcZ = qan::getItemGlobalZ_rec(srcItem);
    const qreal dstZ = qan::getItemGlobalZ_rec(dstItem);
    cache.z = qMax(srcZ, dstZ) - 0.1;   // Edge z value should be less than src/dst value to ensure port item and selection is on top of edge

    if (_style)
        cache.lineType = _style->getLineType();

    // Generate edge line P1 and P2 in global graph CS
    const auto srcBr = cache.srcBs.boundingRect();
    const auto dstBr = cache.dstBs.boundingRect();
    const QPointF srcBrCenter = srcBr.center(); // Keep theses value in processor cache
    const QPointF dstBrCenter = dstBr.center();
    cache.srcBr = srcBr;
    cache.dstBr = dstBr;
    cache.srcBrCenter = srcBrCenter;
    cache.dstBrCenter = dstBrCenter;

    cache.valid = true;  // Finally, validate cache
    return cache;        // Expecting RVO
}

void    EdgeItem::generateStraightEnds(GeometryCache& cache) const noexcept
{
    // PRECONDITIONS:
        // cache should be valid
        // cache srcBrCenter and dstBrCenter must be valid
    if (!cache.isValid())
        return;

    const QLineF line = getLineIntersection(cache.srcBrCenter, cache.dstBrCenter,
                                            cache.srcBs, cache.dstBs);

    // Update hidden: Edge is hidden if it's size is less than the src/dst shape size sum
    {
        {
            const auto arrowSize = getArrowSize();
            const auto arrowLength = arrowSize * 3.;
            if (line.length() < 2.0 + arrowLength)
                cache.hidden = true;
            if (cache.hidden)  // Fast exit if edge is hidden
                return;
        }
        const QRectF lineBr = QRectF{line.p1(), line.p2()}.normalized();  // Generate a Br with intersection points
        cache.hidden = (cache.srcBr.contains(lineBr) ||    // Hide edge if the whole line is contained in either src or dst BR
                        cache.dstBr.contains(lineBr));
        if (cache.hidden)  // Fast exit if edge is hidden
            return;
    }

    // Save generated p1 and p2 to gometry cache
    const auto p1 = line.p1();  // Keep a fast cache access to theses coordinates
    const auto p2 = line.p2();
    cache.p1 = p1;
    cache.p2 = p2;

    {   // Take dock configuraiton into account to correct p1 and p2 when connected to/from a dock.

        // Correction is in fact a "point culling":
        // *Left dock*:    |
        // valid position  O invalid position (since node usually lay here for left docks)
        //                 |             With y beeing culled to either br.top or br.bottom
        //
        // *Top dock*:   valid position
        //                  ---O---      With x beeing culled to either br.left or br.right
        //               invalid position (since node usually lay here for top docks)

        auto correctPortPoint = [](const auto& cache, auto dockType, const auto& p,
                                   const auto& brCenter, const auto& br) -> QPointF {
            QPointF c{p}; // c corrected point
            if (cache.lineType == qan::EdgeStyle::LineType::Straight) {
                switch (dockType) {
                case qan::NodeItem::Dock::Left:
                    if (p.x() > brCenter.x())
                        c = QPointF{brCenter.x(), p.y() > brCenter.y() ? br.bottom() : br.top()};
                    break;
                case qan::NodeItem::Dock::Top:
                    if (p.y() > brCenter.y())
                        c = QPointF{p.x() > brCenter.x() ? br.right() : br.left(), brCenter.y()};
                    break;
                case qan::NodeItem::Dock::Right:
                    if (p.x() < brCenter.x())
                        c = QPointF{brCenter.x(), p.y() > brCenter.y() ? br.bottom() : br.top()};
                    break;
                case qan::NodeItem::Dock::Bottom:
                    if (p.y() < brCenter.y())
                        c = QPointF{p.x() > brCenter.x() ? br.right() : br.left(), brCenter.y()};
                    break;
                }
             } else {    // qan::EdgeStyle::LineType::Curved, for curved line, do not intersect ports, generate point according to port type.
                switch (dockType) {
                    case qan::NodeItem::Dock::Left:
                        c = QPointF{br.left(), brCenter.y()};
                        break;
                    case qan::NodeItem::Dock::Top:
                        c = QPointF{brCenter.x(), br.top()};
                        break;
                    case qan::NodeItem::Dock::Right:
                        c = QPointF{br.right(), brCenter.y()};
                        break;
                    case qan::NodeItem::Dock::Bottom:
                        c = QPointF{brCenter.x(), br.bottom()};
                        break;
                }
            }
            return c; // Expect RVO
        }; // correctPortPoint()

        const auto srcPort = qobject_cast<const qan::PortItem*>(cache.srcItem);
        if (srcPort != nullptr)
            cache.p1 = correctPortPoint(cache, srcPort->getDockType(), p1, cache.srcBrCenter, cache.srcBr );

        const auto dstPort = qobject_cast<const qan::PortItem*>(cache.dstItem);
        if (dstPort != nullptr)
            cache.p2 = correctPortPoint(cache, dstPort->getDockType(), p2, cache.dstBrCenter, cache.dstBr );
    } // dock configuration block
}

void    EdgeItem::generateOrthoEnds(GeometryCache& cache) const noexcept
{
    // PRECONDITIONS:
        // cache should be valid
        // cache srcBs and dstBs must not be empty (valid bounding shapes are necessary)
    if ( !cache.isValid() )
        return;

    // Algorithm:
        // See algorithm description in wiki:
        //   https://github.com/cneben/QuickQanava/wiki/Edge-Geometry-Management
        // Orientation (left, right, top, bottom) is defined from an SRC POV (and
        // orientation is SRC to DST direction).
        //
        // 1. Check if horizontal or vertical line should be generated
            // 1.1 Generate h or v line, set edge style to straight since we are just
            //     drawing a line
        // 2. Otherwise identify if a TR, BR, BL or TL edge should be generated
            // 2.1 Check if we have a more "horiz" or "vert" edge to generate
            // 2.1 Generate P1 control point according to pair {(TR, BR, BL, TL), (horiz, vert)}

    // 1.
    if (cache.srcBrCenter.y() > cache.dstBr.top() &&            // Horizontal line
        cache.srcBrCenter.y() < cache.dstBr.bottom() ) {
        if (cache.dstBrCenter.x() < cache.srcBrCenter.x()) {        // DST is on SRC left
            cache.p1 = QPointF{cache.srcBr.left(),  cache.srcBrCenter.y()};
            cache.p2 = QPointF{cache.dstBr.right(), cache.srcBrCenter.y()};
            cache.c1 = QLineF{cache.p1, cache.p2}.center();
        } else {                                                    // DST is on SRC right
            cache.p1 = QPointF{cache.srcBr.right(), cache.srcBrCenter.y()};
            cache.p2 = QPointF{cache.dstBr.left(),  cache.srcBrCenter.y()};
            cache.c1 = QLineF{cache.p1, cache.p2}.center();
        }
    } else if (cache.srcBrCenter.x() < cache.dstBr.right() &&   // Vertical line
               cache.srcBrCenter.x() > cache.dstBr.left() ) {
        if (cache.dstBrCenter.y() < cache.srcBrCenter.y()) {        // DST is on SRC top
            cache.p1 = QPointF{cache.srcBrCenter.x(), cache.srcBr.top()};
            cache.p2 = QPointF{cache.srcBrCenter.x(), cache.dstBr.bottom()};
            cache.c1 = QLineF{cache.p1, cache.p2}.center();
        } else {                                                    // DST is on SRC bottom
            cache.p1 = QPointF{cache.srcBrCenter.x(), cache.srcBr.bottom()};
            cache.p2 = QPointF{cache.srcBrCenter.x(), cache.dstBr.top()};
            cache.c1 = QLineF{cache.p1, cache.p2}.center();
        }
    } else { // 2.
        const bool top = cache.dstBrCenter.y() < cache.srcBr.y();
        const bool bottom = !top;
        const bool right = cache.dstBrCenter.x() > cache.srcBr.x();
        const bool left = !right;
        // Note: vertical layout is privilegied to horizontal since most of the time, we are
        // building vertical taxonomoy / hierarchy layout. Add an option for the user to promote horizontal
        // ortho "flow" layouts.
        const bool horiz = 0.50 * std::fabs(cache.dstBrCenter.x() - cache.srcBrCenter.x()) >
                           std::fabs(cache.dstBrCenter.y() - cache.srcBrCenter.y());
        const bool vert = !horiz;
        if (bottom) {       // BOTTOM
            if (right && vert) {             // BR, vertical edge
                cache.p1 = QPointF{cache.srcBrCenter.x(),   cache.srcBr.bottom()};
                cache.p2 = QPointF{cache.dstBr.left(),      cache.dstBrCenter.y()};
                cache.c1 = QPointF{cache.srcBrCenter.x(),   cache.dstBrCenter.y()};
            } else if (right && horiz) {      // BR, horizontal edge
                cache.p1 = QPointF{cache.srcBr.right(),     cache.srcBrCenter.y()};
                cache.p2 = QPointF{cache.dstBrCenter.x(),   cache.dstBr.top()    };
                cache.c1 = QPointF{cache.dstBrCenter.x(),   cache.srcBrCenter.y()};
            } else if (left && vert) {        // BL, vertical edge
                cache.p1 = QPointF{cache.srcBrCenter.x(),   cache.srcBr.bottom()};
                cache.p2 = QPointF{cache.dstBr.right(),     cache.dstBrCenter.y()};
                cache.c1 = QPointF{cache.srcBrCenter.x(),   cache.dstBrCenter.y()};
            } else if (left && horiz) {       // BL, horizontal edge
                cache.p1 = QPointF{cache.srcBr.left(),      cache.srcBrCenter.y()};
                cache.p2 = QPointF{cache.dstBrCenter.x(),   cache.dstBr.top()    };
                cache.c1 = QPointF{cache.dstBrCenter.x(),   cache.srcBrCenter.y()};
            }
        } else {            // TOP
            if (right && vert) {             // TR, vertical edge
                cache.p1 = QPointF{cache.srcBrCenter.x(),   cache.srcBr.top()};
                cache.p2 = QPointF{cache.dstBr.left(),      cache.dstBrCenter.y()};
                cache.c1 = QPointF{cache.srcBrCenter.x(),   cache.dstBrCenter.y()};
            } else if (right && horiz) {      // TR, horizontal edge
                cache.p1 = QPointF{cache.srcBr.right(),     cache.srcBrCenter.y()};
                cache.p2 = QPointF{cache.dstBrCenter.x(),   cache.dstBr.bottom()    };
                cache.c1 = QPointF{cache.dstBrCenter.x(),   cache.srcBrCenter.y()};
            } else if (left && vert) {        // TL, vertical edge
                cache.p1 = QPointF{cache.srcBrCenter.x(),   cache.srcBr.top()};
                cache.p2 = QPointF{cache.dstBr.right(),     cache.dstBrCenter.y()};
                cache.c1 = QPointF{cache.srcBrCenter.x(),   cache.dstBrCenter.y()};
            } else if (left && horiz) {       // TL, horizontal edge
                cache.p1 = QPointF{cache.srcBr.left(),      cache.srcBrCenter.y()};
                cache.p2 = QPointF{cache.dstBrCenter.x(),   cache.dstBr.bottom()    };
                cache.c1 = QPointF{cache.dstBrCenter.x(),   cache.srcBrCenter.y()};
            }
        }
    }
}

void    EdgeItem::generateArrowGeometry(GeometryCache& cache) const noexcept
{
    // PRECONDITIONS:
        // cache should be valid
    if (!cache.isValid())
        return;

    const qreal arrowSize = getArrowSize();
    const qreal arrowLength = arrowSize * 3.;

    // Prepare points and helper variables
    const QPointF pointA2      = QPointF{arrowLength,     0.          }; // A2 is the same for all shapes

    const QPointF arrowA1      = QPointF{0.,              -arrowSize  };
    const QPointF arrowA3      = QPointF{0.,              arrowSize   };

    const QPointF circleRectA1 = QPointF{arrowLength / 2., -arrowLength / 2.};
    const QPointF circleRectA3 = QPointF{arrowLength / 2.,  arrowLength / 2.};

    // Point A2 is the same for all arrow shapes
    cache.srcA2 = pointA2;
    cache.dstA2 = pointA2;

    // Update source arrow cache points
    const auto srcShape = getSrcShape();
    switch (srcShape) {
        case qan::EdgeItem::ArrowShape::Arrow:      // [[fallthrough]]
        case qan::EdgeItem::ArrowShape::ArrowOpen:
            cache.srcA1 = arrowA1;
            cache.srcA3 = arrowA3;
            break;
        case qan::EdgeItem::ArrowShape::Circle:     // [[fallthrough]]
        case qan::EdgeItem::ArrowShape::CircleOpen: // [[fallthrough]]
        case qan::EdgeItem::ArrowShape::Rect:       // [[fallthrough]]
        case qan::EdgeItem::ArrowShape::RectOpen:
            cache.srcA1 = circleRectA1;
            cache.srcA3 = circleRectA3;
            break;
        case qan::EdgeItem::ArrowShape::None:
            break;
        // No default, anyway the cache will be invalid
    }
    // Update destination arrow cache points
    const auto dstShape = getDstShape();
    switch (dstShape) {
        case qan::EdgeItem::ArrowShape::Arrow:      // [[fallthrough]]
        case qan::EdgeItem::ArrowShape::ArrowOpen:
            cache.dstA1 = arrowA1;
            cache.dstA3 = arrowA3;
            break;
        case qan::EdgeItem::ArrowShape::Rect:       // [[fallthrough]]
        case qan::EdgeItem::ArrowShape::RectOpen:   // [[fallthrough]]
        case qan::EdgeItem::ArrowShape::Circle:     // [[fallthrough]]
        case qan::EdgeItem::ArrowShape::CircleOpen:
            cache.dstA1 = circleRectA1;
            cache.dstA3 = circleRectA3;
            break;
        case qan::EdgeItem::ArrowShape::None:
            break;
        // No default, anyway the cache will be invalid
    }

    // Generate start/end arrow angle
    switch (cache.lineType) {
        case qan::EdgeStyle::LineType::Undefined:      // [[fallthrough]]
        case qan::EdgeStyle::LineType::Straight:
            cache.dstAngle = generateStraightArrowAngle(cache.p1, cache.p2, dstShape, arrowLength);
            cache.srcAngle = generateStraightArrowAngle(cache.p2, cache.p1, srcShape, arrowLength);
            break;

        case qan::EdgeStyle::LineType::Ortho:
            cache.dstAngle = generateStraightArrowAngle(cache.c1, cache.p2, dstShape, arrowLength);
            cache.srcAngle = generateStraightArrowAngle(cache.c1, cache.p1, srcShape, arrowLength);
            break;

        case qan::EdgeStyle::LineType::Curved:
            // Generate source arrow angle (p2 <-> p1 and c2 <-> c1)
            cache.srcAngle = generateCurvedArrowAngle(cache.p2, cache.p1,
                                                      cache.c2, cache.c1,
                                                      srcShape, arrowLength);

            // Generate destination arrow angle
            cache.dstAngle = generateCurvedArrowAngle(cache.p1, cache.p2,
                                                      cache.c1, cache.c2,
                                                      dstShape, arrowLength);
            break;
    }
}

qreal   EdgeItem::generateStraightArrowAngle(QPointF& p1, QPointF& p2,
                                             const qan::EdgeItem::ArrowShape arrowShape,
                                             const qreal arrowLength) const noexcept
{
    static constexpr    qreal MinLength = 0.00001;          // Correct line dst point to take into account the arrow geometry
    const QLineF line{p1, p2};    // Generate dst arrow line angle
    if (line.length() > MinLength &&       // Protect line.length() DIV0
        arrowShape != ArrowShape::None)    // Do not correct edge extremity by arrowLength if there is not arrow
        p2 = line.pointAt( 1.0 - (arrowLength / line.length()) );
    return lineAngle(line);
}

qreal   EdgeItem::generateCurvedArrowAngle(QPointF& p1, QPointF& p2,
                                           const QPointF& c1, const QPointF& c2,
                                           const qan::EdgeItem::ArrowShape arrowShape,
                                           const qreal arrowLength) const noexcept
{
    const QLineF line{p1, p2};    // Generate dst arrow line angle
    qreal angle = 0.;

    // Generate arrow orientation:
    // General case: get cubic tangent at line end.
    // Special case: when line length is small (ie less than 4 time arrow length), curve might
    //               have very sharp orientation, average tangent at curve end AND line angle to avoid
    //               arrow orientation that does not fit the average curve angle.
    static constexpr auto averageDstAngleFactor = 4.0;
    if (line.length() > averageDstAngleFactor * arrowLength)      // General case
        angle = cubicCurveAngleAt(0.99, p1, p2, c1, c2);
    else {                                                          // Special case
        angle = (0.4 * cubicCurveAngleAt(0.99, p1, p2, c1, c2) +
                 0.6 * lineAngle(line));
    }

    // Use dst angle to generate an end point translated by -arrowLength except if there is no end shape
    // Build a (P2, C2) vector
    if (arrowShape != ArrowShape::None) {
        QVector2D dstVector{ QPointF{c2.x() - p2.x(), c2.y() - p2.y()} };
        dstVector.normalize();
        dstVector *= static_cast<float>(arrowLength);
        p2 = QPointF{p2} + dstVector.toPointF();
    }
   return angle;
}

void    EdgeItem::generateCurvedControlPoints(GeometryCache& cache) const noexcept
{
    // PRECONDITIONS:
        // cache should be valid
        // cache srcBs and dstBs must not be empty (valid bounding shapes are necessary)
        // cache style must be straight line
    if ( !cache.isValid() )
        return;
    if ( cache.lineType != qan::EdgeStyle::LineType::Curved )
        return;

    const auto srcPort = qobject_cast<const qan::PortItem*>(cache.srcItem);
    const auto dstPort = qobject_cast<const qan::PortItem*>(cache.dstItem);

    const auto xDelta = cache.p2.x() - cache.p1.x();
    const auto xDeltaAbs = std::abs(xDelta);
    const auto yDelta = cache.p2.y() - cache.p1.y();
    const auto yDeltaAbs = std::abs(yDelta);

    const QLineF line{cache.p1, cache.p2};
    const auto lineLength = line.length();

    if ( srcPort == nullptr ||      // If there is a connection to a non-port item, generate a control point for it
         dstPort == nullptr ) {

        // Invert if:
            // Top left quarter:     do not invert (xDelta < 0 && yDelta < 0)
            // Top right quarter:    xDelta > 0 && yDelta < 0
            // Bottom rigth quarter: do not invert (xDelta > 0 && yDelta >0)
            // Bottom left quarter:  xDelta < 0 && yDelta > 0
        const auto invert =  ( xDelta > 0 && yDelta < 0 ) ||
                             ( xDelta < 0 && yDelta > 0 ) ? -1. : 1.;

        const bool lengthIsZero = std::abs(lineLength) < std::numeric_limits<qreal>::epsilon();
        QPointF offset { 0., 0. };
        if (!lengthIsZero) {
            const QPointF normal = QPointF{ -line.dy(), line.dx() } / ( lineLength * invert );
            // SIMPLE CASE: Generate cubic curve control points with no dock, just use line center and normal
            // Heuristic: bounded to [0;40.], larger when line length is large (line.length()), small when
            //            line is either vertical or horizontal (xDelta/yDelta near 0)
            const auto distance = std::min( line.length(), std::min(xDeltaAbs / 2., yDeltaAbs / 2.) );
            const auto controlPointDistance = qBound( 0.001, distance, 40. );

            offset = normal * controlPointDistance;
        }

        const QPointF center{ ( cache.p1.x() + cache.p2.x() ) / 2.,           // (P1,P2) Line center
                              ( cache.p1.y() + cache.p2.y() ) / 2. };

        if ( srcPort == nullptr )
            cache.c1 = center + offset;
        if ( dstPort == nullptr )
            cache.c2 = center - offset;
    }
    if ( srcPort != nullptr ||      // If there is a connection to a port item, generate a control point for it
         dstPort != nullptr ) {
        static constexpr qreal maxOffset = 40.;
        auto offset = [](auto deltaAbs) -> auto {
            // Heuristic: for [0, maxOffset] delta, return a percentage of maxOffset, return value
            //             is between [4.;maxOffset]
            return std::max(10., std::min(deltaAbs, 100.) / 100. * maxOffset);
        };

        // Offset / Correction:
        //  Offset is translation in port direction (left for left port, top for top port and so on....)
        //  Correction is translation in respect to dX/dY of edge line.
        //
        //          C1 o <-offset->
        //             ^
        //             |
        //         correction
        //             |
        //             v          +----------+
        //                       O|   NODE   |
        //              left port +----------+
        //

        // Heuristics:
            // 1. xCorrection should be really small when xDeltaAbs is small (small == < average bounding rect width)
                // otherwise, it should be proportional to one fifth of xDeltaAbs and always less than maxOffset.
            // 2. yCorrection should be really small when yDeltaAbs is small (small == < average bounding rect height)
                // otherwise, it should be proportional to one fifth of yDeltaAbs and always less than maxOffset.
            // 3. Do not apply correction on out port
        const auto xOffset = offset(xDeltaAbs);
        const auto yOffset = offset(yDeltaAbs);


        static constexpr auto maxCorrection = 100.;
        Q_UNUSED(maxCorrection);
        auto correction = [](auto deltaAbs, auto maxSize) -> auto {
            //           c = correction
            //           ^
            //           |
            // 3*maxSize * p0 (0,maxSize*3)
            //           |   *
            //       p.y +     * p(x, y)
            //           |       *  p1 (maxSize, maxSize)
            // maxSize   +          *****************************
            //           |
            //           +-----+----+---------------------------->   delta
            //           0    p.x maxSize
            //
            // on x in [O,maxSize] we want to interpolate linearly on (p0, p1):
            // p.x = deltaAbs
            //
            // See https://en.wikipedia.org/wiki/Linear_interpolation, then simplify to a polynomial form for our special use-case:
            //
            //       p0.y (p1.x-x) + p1.y(x - p0.x)    3 * maxSize * (maxSize-x) + (maxSize * x)    (3*maxSize²) - (3*maxSize*x) + (maxSize*x)
            // p.y = ------------------------------ = ------------------------------------------ =  ------------------------------------------
            //               p1.x - p0.x                               maxSize                                      maxSize
            qreal c = maxSize;
            if ( deltaAbs < maxSize ) {
                const auto deltaAbsMaxSize = deltaAbs * maxSize;
                const auto maxSize2 = maxSize * maxSize;
                c = ( (3*maxSize2) - (3*deltaAbsMaxSize) + deltaAbsMaxSize ) / maxSize;
            }
            return c;
        };
        const auto maxBrWidth = 100.;
        //const auto maxBrWidth = std::max(cache.srcBr.width(), cache.dstBr.width());
        qreal xCorrection = ( std::signbit(xDelta) ? -1. : 1. ) * correction(xDeltaAbs, maxBrWidth);

        const auto maxBrHeight = 50.;
        //const auto maxBrHeight = std::max(cache.srcBr.height(), cache.dstBr.height());
        qreal yCorrection = ( std::signbit(yDelta) ? -1. : 1. ) * correction(yDeltaAbs, maxBrHeight);
        //qDebug() << "maxBrWidth=" << maxBrWidth << "\tmaxBrHeight=" << maxBrHeight;
        //qDebug() << "xDelta=" << xDelta << "\txCorrection=" << xCorrection;
        //qDebug() << "yDelta=" << yDelta << "\tyCorrection=" << yCorrection;

                                           // Left Tp Rgt Bot None
        qreal xCorrect[5][5] =             { { 1,  0,  0,  1, 0 },     // Dock:Left
                                             { 0,  1,  0,  0, 0 },     // Dock::Top
                                             { 0,  0, -1, -1, 0 },     // Dock::Right
                                             { 1,  0, -1, -1, 0 },     // Dock::Bottom
                                             { 0,  0,  0,  0, 0 } };   // None

        qreal yCorrect[5][5] =             { { 1,  1,  0,  0, 0 },     // Dock:Left
                                             { 1,  0,  0,  0, 0 },     // Dock::Top
                                             { 0,  0,  0, -1, 0 },    // Dock::Right
                                             { 0,  0, -1,  0, 0 },    // Dock::Bottom
                                             { 0,  0,  0,  0, 0 } };   // None

        unsigned int previous = srcPort != nullptr ? static_cast<unsigned int>(srcPort->getDockType()) : 4;;  // 4 = None
        unsigned int next = dstPort != nullptr ? static_cast<unsigned int>(dstPort->getDockType()) : 4;      // 4 = None

        using Dock = qan::NodeItem::Dock;
        const double xSmooth = qBound(-100., xDelta, 100.) / 100.;
        const double ySmooth = qBound(-100., yDelta, 100.) / 100.;
        if ( srcPort != nullptr ) {     // Generate control point for src (C1)
            const auto xCorrectionFinal = xCorrection * xCorrect[previous][next] * ySmooth;
            const auto yCorrectionFinal = yCorrection * yCorrect[previous][next] * xSmooth;
            switch ( srcPort->getDockType() ) {
            case Dock::Left:     cache.c1 = cache.p1 + QPointF{ -xOffset,           yCorrectionFinal  };  break;
            case Dock::Top:      cache.c1 = cache.p1 + QPointF{ xCorrectionFinal,  -yOffset      };  break;
            case Dock::Right:    cache.c1 = cache.p1 + QPointF{ xOffset,            yCorrectionFinal  };  break;
            case Dock::Bottom:   cache.c1 = cache.p1 + QPointF{ xCorrectionFinal,   yOffset      };  break;
            }
        }
        if ( dstPort != nullptr ) {     // Generate control point for dst (C2)
            const auto xCorrectionFinal = xCorrection * xCorrect[previous][next] * ySmooth;
            const auto yCorrectionFinal = yCorrection * yCorrect[previous][next] * xSmooth;
            switch ( dstPort->getDockType() ) {
            case Dock::Left:     cache.c2 = cache.p2 + QPointF{ -xOffset,           yCorrectionFinal };  break;
            case Dock::Top:      cache.c2 = cache.p2 + QPointF{ xCorrectionFinal,   -yOffset    };  break;
            case Dock::Right:    cache.c2 = cache.p2 + QPointF{ xOffset,            yCorrectionFinal };  break;
            case Dock::Bottom:   cache.c2 = cache.p2 + QPointF{ xCorrectionFinal,   yOffset     };  break;
            }
        }
    }

    // Finally, modify p1 and p2 according to c1 and c2
    cache.p1 = getLineIntersection( cache.c1, cache.srcBrCenter, cache.srcBs);
    cache.p2 = getLineIntersection( cache.c2, cache.dstBrCenter, cache.dstBs);
}


void    EdgeItem::generateLabelPosition(GeometryCache& cache) const noexcept
{
    // PRECONDITIONS:
        // cache should be valid
    if ( !cache.isValid() )
        return;

    const QLineF line{cache.p1, cache.p2};
    if ( cache.lineType == qan::EdgeStyle::LineType::Straight ) {
        cache.labelPosition = line.pointAt(0.5) + QPointF{10., 10.};
    } else if ( cache.lineType == qan::EdgeStyle::LineType::Curved ) {
        // Get the barycenter of polygon p1/p2/c1/c2
        QPolygonF p{ {cache.p1, cache.p2, cache.c1, cache.c2 } };
        if (!p.isEmpty())
            cache.labelPosition = p.boundingRect().center();
    }
}

void    EdgeItem::applyGeometry(const GeometryCache& cache) noexcept
{
    // PRECONDITIONS:
        // cache should be valid
        // edge should not be hidden
    if (!cache.isValid())
        return;

    if (cache.hidden) {    // Apply hidden property
        // Note: Do not call setVisible(false), visibility management is left up to the user
        setHidden(true);
        return;
    }

    const QQuickItem*   graphContainerItem = getGraph() != nullptr ? getGraph()->getContainerItem() :
                                                                     nullptr;
    if (graphContainerItem != nullptr) {
        QPolygonF edgeBrPolygon;
        edgeBrPolygon << cache.p1 << cache.p2;
        if (cache.lineType == qan::EdgeStyle::LineType::Curved)
            edgeBrPolygon << cache.c1 << cache.c2;
        const QRectF edgeBr = edgeBrPolygon.boundingRect();
        setPosition(edgeBr.topLeft());    // Note: setPosition() call must occurs before mapFromItem()
        setSize(edgeBr.size());

        _p1 = mapFromItem(graphContainerItem, cache.p1);
        _p2 = mapFromItem(graphContainerItem, cache.p2);
        emit lineGeometryChanged();

        {   // Apply arrow geometry
            _dstAngle = cache.dstAngle;
            emit dstAngleChanged(); // Note: Update dstAngle before arrow geometry.

            _dstA1 = cache.dstA1;    // Arrow geometry is already expressed in edge "local CS"
            _dstA2 = cache.dstA2;
            _dstA3 = cache.dstA3;
            emit dstArrowGeometryChanged();

            _srcAngle = cache.srcAngle;
            emit srcAngleChanged(); // Note: Update srcAngle before arrow geometry.

            _srcA1 = cache.srcA1;    // Arrow geometry is already expressed in edge "local CS"
            _srcA2 = cache.srcA2;
            _srcA3 = cache.srcA3;
            emit srcArrowGeometryChanged();
        }

        // Apply control point geometry
            // For otho edge: 3 points for a line P1 -> C1 -> P2
            // For Curved edge: a cubic spline with C1 and C2
        if (cache.lineType == qan::EdgeStyle::LineType::Ortho) {
            _c1 = mapFromItem(graphContainerItem, cache.c1);
            emit controlPointsChanged();
        } else if (cache.lineType == qan::EdgeStyle::LineType::Curved) { // Apply control point geometry
            _c1 = mapFromItem(graphContainerItem, cache.c1);
            _c2 = mapFromItem(graphContainerItem, cache.c2);
            emit controlPointsChanged();
        }

        setZ(cache.z);
        setLabelPos(mapFromItem(graphContainerItem, cache.labelPosition));
    }

    // Edge item geometry is now valid, set the item visibility to true and "unhide" it
    //setVisible(true);
    setHidden(false);
}

qreal   EdgeItem::lineAngle(const QLineF& line) const noexcept
{
    static constexpr    qreal Pi = 3.141592653;
    static constexpr    qreal TwoPi = 2. * Pi;
    static constexpr    qreal MinLength = 0.00001;
    const qreal lineLength = line.length();
    if ( lineLength < MinLength )
        return -1.;
    double angle = std::acos( line.dx() / lineLength );
    if ( line.dy() < 0. )
        angle = TwoPi - angle;
    return angle * ( 360. / TwoPi );
}

void    EdgeItem::setLine( QPoint src, QPoint dst )
{
    _p1 = src;
    _p2 = dst;
    emit lineGeometryChanged();
}

QPointF  EdgeItem::getLineIntersection(const QPointF& p1, const QPointF& p2,
                                       const QPolygonF& polygon) const noexcept
{
    const QLineF line{p1, p2};
    QPointF source{p1};
    QPointF intersection;
    for (auto p = 0; p < polygon.length() - 1 ; ++p) {
        const QLineF polyLine(polygon[p], polygon[p + 1]);
        if (line.intersects(polyLine, &intersection) == QLineF::BoundedIntersection ) {
            source = intersection;
            break;
        }
    }
    return source;
}

QLineF  EdgeItem::getLineIntersection( const QPointF& p1, const QPointF& p2,
                                       const QPolygonF& srcBp, const QPolygonF& dstBp ) const noexcept
{
    const QLineF line{p1, p2};
    QPointF source{p1};
    QPointF intersection;
    for (auto p = 0; p < srcBp.length() - 1 ; ++p) {
        const QLineF polyLine(srcBp[p], srcBp[p + 1]);
        if (line.intersects(polyLine, &intersection) == QLineF::BoundedIntersection ) {
            source = intersection;
            break;
        }
    }
    QPointF destination{p2};
    for (auto p = 0; p < dstBp.length() - 1 ; ++p) {
        const QLineF polyLine(dstBp[p], dstBp[p + 1]);
        if (line.intersects(polyLine, &intersection) == QLineF::BoundedIntersection) {
            destination = intersection;
            break;
        }
    }
    return QLineF{source, destination};
}
//-----------------------------------------------------------------------------


/* Curve Control Points Management *///----------------------------------------
qreal   EdgeItem::cubicCurveAngleAt(qreal pos, const QPointF& start, const QPointF& end, const QPointF& c1, const QPointF& c2 ) const noexcept
{
    // Preconditions:
        // pos in [0., 1.0]
    if ( pos < 0. || pos > 1. )
        return -1;

    // FIXME 20170914: That code need more testing, std::atan2 return value should be handled with
    // more care: https://stackoverflow.com/questions/1311049/how-to-map-atan2-to-degrees-0-360

    // This code is (C) 2014 Paul W.
    // Description:
        // http://www.paulwrightapps.com/blog/2014/9/4/finding-the-position-and-angle-of-points-along-a-bezier-curve-on-ios

    // Compute cubic polynomial coefficients
    const QPointF coeff3{end.x() - (3. * c2.x()) + (3. * c1.x()) - start.x(),
                         end.y() - (3. * c2.y()) + (3. * c1.y()) - start.y()};

    const QPointF coeff2{(3. * c2.x()) - (6. * c1.x()) + (3. * start.x()),
                         (3. * c2.y()) - (6. * c1.y()) + (3. * start.y())};

    const QPointF coeff1{(3. * c1.x()) - (3. * start.x()),
                         (3. * c1.y()) - (3. * start.y())};

    const auto pos2 = pos * pos;
    const auto dxdt = (3. * coeff3.x()*pos2) + (2. * coeff2.x()*pos) + coeff1.x();
    const auto dydt = (3. * coeff3.y()*pos2) + (2. * coeff2.y()*pos) + coeff1.y();

    static constexpr    qreal Pi = 3.141592653;
    auto degrees = std::atan2(dxdt, dydt) * 180. / Pi;
    if (degrees > 90.)
        degrees = 450. - degrees;
    else
        degrees = 90. - degrees;
    return degrees;
}
//-----------------------------------------------------------------------------


/* Mouse Management *///-------------------------------------------------------
void    EdgeItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    if ((getEdge() != nullptr && !getEdge()->getLocked()) &&
        event->button() == Qt::LeftButton &&
        contains(event->localPos())) {
        emit edgeDoubleClicked(this, event->localPos());
        event->accept();
    }
    else
        event->ignore();
    QQuickItem::mouseDoubleClickEvent(event);
}

void    EdgeItem::mousePressEvent(QMouseEvent* event)
{
    // Note 20211030: Do not take getLocked() into account,
    // otherwise onEdgeDoubleClicked() is no longer fired (and edge
    // can't be unlocked with a visual editor !
    if (contains(event->localPos())) {
        // Selection management
        if ((event->button() == Qt::LeftButton ||
             event->button() == Qt::RightButton) &&
             getEdge() != nullptr &&
             isSelectable() &&
             !getEdge()->getLocked()) {  // Selection allowed for protected
            if (_graph)
                _graph->selectEdge(*getEdge(), event->modifiers());
        }

        if (event->button() == Qt::LeftButton) {
            emit edgeClicked(this, event->localPos());
            event->accept();
        }
        else if (event->button() == Qt::RightButton) {
            emit edgeRightClicked(this, event->localPos());
            event->accept();
        }
    } else
        event->ignore();
}

void    EdgeItem::mouseMoveEvent(QMouseEvent* event)
{
    // Early exits
    if (getEdge() == nullptr ||
        getEdge()->getIsProtected() ||
        getEdge()->getLocked()) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }
    if (!getDraggable())
        return;
    if (event->buttons().testFlag(Qt::NoButton))
        return;

    const auto draggableCtrl = static_cast<EdgeDraggableCtrl*>(_draggableCtrl.get());
    if (draggableCtrl->handleMouseMoveEvent(event))
        event->accept();
    else
        event->ignore();
        // Note 20200531: Do not call base QQuickItem implementation, really.
}

void    EdgeItem::mouseReleaseEvent(QMouseEvent* event)
{
    const auto draggableCtrl = static_cast<EdgeDraggableCtrl*>(_draggableCtrl.get());
    draggableCtrl->handleMouseReleaseEvent(event);
}

qreal   EdgeItem::distanceFromLine(const QPointF& p, const QLineF& line) const noexcept
{
    // Inspired by DistancePointLine Unit Test, Copyright (c) 2002, All rights reserved
    // Damian Coventry  Tuesday, 16 July 2002
    static constexpr    qreal MinLength = 0.00001;
    const qreal lLength = line.length();
    if (lLength < MinLength)
        return -1.; // Protect generation of u from DIV0
    const qreal u  = ( ( ( p.x() - line.x1() ) * ( line.x2() - line.x1() ) ) +
                     ( ( p.y() - line.y1() ) * ( line.y2() - line.y1() ) ) ) / ( lLength * lLength );
    if (u < 0. || u > 1.)
        return -1.;
    const QPointF i {line.x1() + u * ( line.x2() - line.x1() ),
                     line.y1() + u * ( line.y2() - line.y1() )};
    return QLineF{p, i}.length();
}
//-----------------------------------------------------------------------------

/* Style and Properties Management *///----------------------------------------
void    EdgeItem::setStyle(EdgeStyle* style) noexcept
{
    if (style != _style) {
        if (_style != nullptr)  // Every style that is non default is disconnect from this node
            QObject::disconnect(_style, nullptr,
                                this,   nullptr);
        _style = style;
        if (_style) {
            connect(_style,    &QObject::destroyed,    // Monitor eventual style destruction
                    this,      &EdgeItem::styleDestroyed );
            // Note 20170909: _style.styleModified() signal is _not_ binded to updateItem() slot, since
            // it would be very unefficient to update edge for properties change affecting only
            // edge visual item (for example, _stye.lineWidth modification is watched directly
            // from edge delegate). Since arrowSize affect concrete edge geometry, bind it manually to
            // updateItem().
            connect(_style,    &qan::EdgeStyle::arrowSizeChanged,
                    this,      &EdgeItem::styleModified);
            connect(_style,    &qan::EdgeStyle::lineTypeChanged,
                    this,      &EdgeItem::styleModified);
            connect(_style,    &qan::EdgeStyle::srcShapeChanged,
                    this,      &EdgeItem::styleModified);
            connect(_style,    &qan::EdgeStyle::dstShapeChanged,
                    this,      &EdgeItem::styleModified);
        }
        emit styleChanged();
        updateItem();   // Force initial style settings
    }
}

void    EdgeItem::styleDestroyed(QObject* style) { Q_UNUSED(style); setStyle(nullptr); }

void    EdgeItem::styleModified()
{
    if (getStyle() != nullptr) {
        _arrowSize = getStyle()->getArrowSize();
        emit arrowSizeChanged();
        _srcShape = getStyle()->getSrcShape();
        emit srcShapeChanged();
        _dstShape = getStyle()->getDstShape();
        emit dstShapeChanged();
        updateItem();
    }
}
//-----------------------------------------------------------------------------

/* Edge drag management *///---------------------------------------------------
void    EdgeItem::setDraggable(bool draggable) noexcept
{
    if (draggable != _draggable) {
        _draggable = draggable;
        if (!draggable)
            setDragged(false);
        emit draggableChanged();
    }
}

void    EdgeItem::setDragged(bool dragged) noexcept
{
    if (dragged != _dragged) {
        _dragged = dragged;
        emit draggedChanged();
    }
}

qan::AbstractDraggableCtrl& EdgeItem::draggableCtrl() { Q_ASSERT(_draggableCtrl != nullptr); return *_draggableCtrl; }
//-----------------------------------------------------------------------------


/* Selection Management *///---------------------------------------------------
void    EdgeItem::onWidthChanged()
{
    configureSelectionItem();
}

void    EdgeItem::onHeightChanged()
{
    configureSelectionItem();
}
//-----------------------------------------------------------------------------


/* Drag'nDrop Management *///--------------------------------------------------
void    EdgeItem::setAcceptDrops(bool acceptDrops)
{
    _acceptDrops = acceptDrops;
    setFlag(QQuickItem::ItemAcceptsDrops, acceptDrops);
    emit acceptDropsChanged();
}

bool    EdgeItem::contains(const QPointF& point) const
{
    const auto lineType = _style ? _style->getLineType() : qan::EdgeStyle::LineType::Straight;
    bool r = false;
    qreal d = 0.;
    switch (lineType) {
    case qan::EdgeStyle::LineType::Undefined:  // [[fallthrough]]
    case qan::EdgeStyle::LineType::Straight:
        d = distanceFromLine(point, QLineF{_p1, _p2});
        r = (d > -0.001 && d < 6.001);
        break;
    case qan::EdgeStyle::LineType::Curved: {
        bezier::Bezier<3> cubicBezier({{static_cast<float>(_p1.x()), static_cast<float>(_p1.y())},
                                       {static_cast<float>(_c1.x()), static_cast<float>(_c1.y())},
                                       {static_cast<float>(_c2.x()), static_cast<float>(_c2.y())},
                                       {static_cast<float>(_p2.x()), static_cast<float>(_p2.y())}});
        // FIXME: check p in cubicBezier.tbb() (aka Tight bounding box)
        // FIXME: At least, adapt steps to tbb.length() / thresold (here 6)
        const auto steps = 25;
        const auto dStep = 1.0 / static_cast<float>(steps);
        for (int step = 0; step < steps; step++) {
            const auto pos = dStep * step;
            auto curveP = cubicBezier.valueAt(pos);
            const auto d = QLineF{point, QPointF{curveP.x, curveP.y}}.length();
            if (d > -0.001 && d < 6.001) {
                r = true;
                break;
            }
        }
    }
        break;
    case qan::EdgeStyle::LineType::Ortho:
        d = distanceFromLine(point, QLineF{_p1, _c1});
        r = (d > -0.001 && d < 6.001);
        if (!r) {
            const qreal d2 = distanceFromLine(point, QLineF{_p2, _c1});
            r = (d2 > -0.001 && d2 < 6.001);
        }
        break;
    }
    return r;
}

void    EdgeItem::dragEnterEvent(QDragEnterEvent* event)
{
    // Note 20160218: contains() is used so enter event is necessary generated "on the edge line"
    if (_acceptDrops) {
        if (event->source() == nullptr) {
            event->accept(); // This is propably a drag initated with type=Drag.Internal, for exemple a connector drop node trying to create an hyper edge, accept by default...
            QQuickItem::dragEnterEvent(event);
            return;
        }
        if (event->source() != nullptr) { // Get the source item from the quick drag attached object received
            QVariant source = event->source()->property("source");
            if (source.isValid()) {
                QQuickItem* sourceItem = source.value<QQuickItem*>();
                QVariant draggedStyle = sourceItem->property("draggedEdgeStyle"); // The source item (usually a style node or edge delegate must expose a draggedStyle property.
                if (draggedStyle.isValid()) {
                    event->accept();
                    return;
                }
            }
        }
        event->ignore();
        QQuickItem::dragEnterEvent(event);
    }
    QQuickItem::dragEnterEvent(event);
}

void	EdgeItem::dragMoveEvent(QDragMoveEvent* event)
{
    if (getAcceptDrops()) {
        qreal d = distanceFromLine(event->posF( ), QLineF{_p1, _p2});
        if (d > 0. && d < 5.)
            event->accept();
        else event->ignore();
    }
    QQuickItem::dragMoveEvent(event);
}

void	EdgeItem::dragLeaveEvent(QDragLeaveEvent* event)
{
    QQuickItem::dragLeaveEvent(event);
    if (getAcceptDrops())
        event->ignore();
}

void    EdgeItem::dropEvent( QDropEvent* event )
{
    if (getAcceptDrops() && event->source() != nullptr) { // Get the source item from the quick drag attached object received
        QVariant source = event->source()->property("source");
        if (source.isValid()) {
            QQuickItem* sourceItem = source.value<QQuickItem*>();
            QVariant draggedStyle = sourceItem->property("draggedEdgeStyle"); // The source item (usually a style node or edge delegate must expose a draggedStyle property.
            if (draggedStyle.isValid()) {
                qan::EdgeStyle* draggedEdgeStyle = draggedStyle.value< qan::EdgeStyle* >();
                if (draggedEdgeStyle != nullptr) {
                    setStyle(draggedEdgeStyle);
                    event->accept();
                }
            }
        }
    }
    QQuickItem::dropEvent(event);
}
//-----------------------------------------------------------------------------

} // ::qan
