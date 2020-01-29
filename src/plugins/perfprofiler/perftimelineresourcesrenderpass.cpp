/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perftimelineresourcesrenderpass.h"
#include "perftimelinemodel.h"

#include <utils/theme/theme.h>

#include <tracing/timelineabstractrenderer.h>
#include <tracing/timelinerenderstate.h>

#include <QSGFlatColorMaterial>

namespace PerfProfiler {
namespace Internal {

class ResourcesRenderPassState final : public Timeline::TimelineRenderPass::State
{
public:
    ResourcesRenderPassState();
    ~ResourcesRenderPassState() final;

    QSGFlatColorMaterial *material() { return &m_material; }

    int indexFrom() const { return m_indexFrom; }
    int indexTo() const { return m_indexTo; }

    void updateIndexes(int from, int to);

    const QVector<QSGNode *> &expandedRows() const final { return m_expandedRows; }
    const QVector<QSGNode *> &collapsedRows() const final { return m_collapsedRows; }

    void addGeometry(QSGGeometry *geometry);

private:
    QSGFlatColorMaterial m_material;
    QVector<QSGNode *> m_collapsedRows;
    QVector<QSGNode *> m_expandedRows;
    QVector<QSGGeometry *> m_geometries;

    int m_indexFrom;
    int m_indexTo;
};

struct ResourcesGeometry {
    static const int maxEventsPerNode = 0xffff;

    int allocatedVertices = 0;
    int usedVertices = 0;

    QSGGeometry *geometry = nullptr;
    QSGGeometryNode *collapsedNode = nullptr;
    QSGGeometryNode *expandedNode = nullptr;
    QSGGeometry::Point2D *vertexData();

    void allocate(QSGMaterial *material);
    void addEvent(float x, float y);

    QSGGeometryNode *createNode(QSGMaterial *material);
    void clear();
};


const PerfTimelineResourcesRenderPass *PerfTimelineResourcesRenderPass::instance()
{
    static const PerfTimelineResourcesRenderPass pass = PerfTimelineResourcesRenderPass();
    return &pass;
}

static void insertNode(ResourcesGeometry *geometry, const PerfTimelineModel *model,
                       int from, int to, const Timeline::TimelineRenderState *parentState,
                       ResourcesRenderPassState *state)
{
    QSGNode *expandedSamplesRow = state->expandedRows().at(PerfTimelineModel::SamplesRow);
    QSGNode *collapsedSamplesRow = state->collapsedRows().at(PerfTimelineModel::SamplesRow);
    if (geometry->usedVertices > 0) {
        geometry->allocate(state->material());
        collapsedSamplesRow->appendChildNode(geometry->collapsedNode);
        expandedSamplesRow->appendChildNode(geometry->expandedNode);
        state->addGeometry(geometry->geometry);
    }

    const int rowHeight = Timeline::TimelineModel::defaultRowHeight();
    for (int i = from; i < to; ++i) {
        if (!model->isResourceTracePoint(i))
            continue;

        qint64 center = qMax(parentState->start(), qMin(parentState->end(), model->startTime(i)));
        float itemCenter = (center - parentState->start()) * parentState->scale();

        geometry->addEvent(itemCenter, (1.0f - model->resourceUsage(i)) * rowHeight);
    }
}

static void updateNodes(const PerfTimelineModel *model, int from, int to,
                        const Timeline::TimelineRenderState *parentState,
                        ResourcesRenderPassState *state)
{
    ResourcesGeometry geometry;

    for (int i = from; i < to; ++i) {
        if (model->isResourceTracePoint(i)
                && ++geometry.usedVertices == ResourcesGeometry::maxEventsPerNode) {
            insertNode(&geometry, model, from, i + 1, parentState, state);
            geometry.clear();
            geometry.usedVertices = 1;
            from = i;
        }
    }

    if (geometry.usedVertices > 0)
        insertNode(&geometry, model, from, to, parentState, state);
}

Timeline::TimelineRenderPass::State *PerfTimelineResourcesRenderPass::update(
        const Timeline::TimelineAbstractRenderer *renderer,
        const Timeline::TimelineRenderState *parentState,
        Timeline::TimelineRenderPass::State *oldState, int indexFrom, int indexTo,
        bool stateChanged, float spacing) const
{
    Q_UNUSED(stateChanged)
    Q_UNUSED(spacing)

    const PerfTimelineModel *model = qobject_cast<const PerfTimelineModel *>(renderer->model());

    if (!model || indexFrom < 0 || indexTo > model->count() || indexFrom >= indexTo)
        return oldState;

    for (int i = indexFrom; i >= 0; --i) {
        if (model->isResourceTracePoint(i)) {
            indexFrom = i;
            break;
        }
    }

    for (int i = indexTo, end = model->count(); i < end; ++i) {
        if (model->isResourceTracePoint(i)) {
            indexTo = i + 1;
            break;
        }
    }

    ResourcesRenderPassState *state = oldState ? static_cast<ResourcesRenderPassState *>(oldState)
                                               : new ResourcesRenderPassState;

    if (state->indexFrom() < state->indexTo()) {
        if (indexFrom < state->indexFrom())
            updateNodes(model, indexFrom, state->indexFrom() + 1, parentState, state);

        if (indexTo > state->indexTo())
            updateNodes(model, state->indexTo() - 1, indexTo, parentState, state);
    } else {
        updateNodes(model, indexFrom, indexTo, parentState, state);
    }

    state->updateIndexes(indexFrom, indexTo);
    return state;
}

QSGGeometry::Point2D *ResourcesGeometry::vertexData()
{
    return geometry->vertexDataAsPoint2D();
}

void ResourcesGeometry::allocate(QSGMaterial *material)
{
    geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), usedVertices);
    geometry->setLineWidth(3.0);
    Q_ASSERT(geometry->vertexData());
    geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
    geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
    geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
    collapsedNode = createNode(material);
    expandedNode = createNode(material);
    allocatedVertices = usedVertices;
    usedVertices = 0;
}

QSGGeometryNode *ResourcesGeometry::createNode(QSGMaterial *material)
{
    QSGGeometryNode *node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry, false);
    node->setMaterial(material);
    return node;
}

void ResourcesGeometry::clear()
{
    allocatedVertices = usedVertices = 0;
    geometry = nullptr;
    collapsedNode = expandedNode = nullptr;
}

void ResourcesGeometry::addEvent(float x, float y)
{
    QSGGeometry::Point2D *v = vertexData() + usedVertices;
    v->set(x, y);
    ++usedVertices;
}

ResourcesRenderPassState::ResourcesRenderPassState() :
    m_indexFrom(std::numeric_limits<int>::max()), m_indexTo(-1)
{
    m_collapsedRows.fill(nullptr, PerfTimelineModel::SamplesRow);
    QSGNode *node = new QSGNode;
    node->setFlag(QSGNode::OwnedByParent, false);
    m_collapsedRows.append(node);
    m_expandedRows.fill(nullptr, PerfTimelineModel::SamplesRow);
    node = new QSGNode;
    node->setFlag(QSGNode::OwnedByParent, false);
    m_expandedRows.append(node);
    m_material.setColor(Utils::creatorTheme()->color(Utils::Theme::Timeline_HighlightColor));

    // Disable blending
    m_material.setFlag(QSGMaterial::Blending, false);
}

ResourcesRenderPassState::~ResourcesRenderPassState()
{
    qDeleteAll(m_collapsedRows);
    qDeleteAll(m_expandedRows);
    qDeleteAll(m_geometries);
}

void ResourcesRenderPassState::updateIndexes(int from, int to)
{
    if (from < m_indexFrom)
        m_indexFrom = from;
    if (to > m_indexTo)
        m_indexTo = to;
}

void ResourcesRenderPassState::addGeometry(QSGGeometry *geometry)
{
    m_geometries.append(geometry);
}

} // namespace Internal
} // namespace PerfProfiler
