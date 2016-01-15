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

#include "timelineitemsrenderpass.h"
#include "timelinerenderstate.h"
#include <QSGSimpleRectNode>
#include <QSGVertexColorMaterial>
#include <QtAlgorithms>

namespace Timeline {

class TimelineItemsRenderPassState : public TimelineRenderPass::State {
public:
    TimelineItemsRenderPassState(const TimelineModel *model);
    ~TimelineItemsRenderPassState();

    QSGNode *expandedRow(int row) const { return m_expandedRows[row]; }
    QSGNode *collapsedRow(int row) const { return m_collapsedRows[row]; }

    const QVector<QSGNode *> &expandedRows() const { return m_expandedRows; }
    const QVector<QSGNode *> &collapsedRows() const { return m_collapsedRows; }
    TimelineItemsMaterial *collapsedRowMaterial() { return &m_collapsedRowMaterial; }

    int indexFrom() const { return m_indexFrom; }
    int indexTo() const { return m_indexTo; }
    void updateIndexes(int from, int to);
    void updateCollapsedRowMaterial(float xScale, int selectedItem, QColor selectionColor);

private:
    int m_indexFrom;
    int m_indexTo;
    TimelineItemsMaterial m_collapsedRowMaterial;

    QVector<QSGNode *> m_expandedRows;
    QVector<QSGNode *> m_collapsedRows;
};

struct TimelineItemsGeometry {
    // Alternating nodes with with 7 and 4 vertices; and vertex indices are 16bit
    static const int maxEventsPerNode = 0xffff * 2 / (7 + 4);

    TimelineItemsGeometry() : allocatedVertices(0), usedVertices(0), node(0) {
        prevNode.set(0, TimelineModel::defaultRowHeight(), 0, 0, 0, 0, 0, 0);
    }

    uint allocatedVertices;
    uint usedVertices;

    OpaqueColoredPoint2DWithSize prevNode;

    QSGGeometryNode *node;


    void allocate(QSGMaterial *material);
    void addVertices(float itemTop);
    void addEvent(float itemLeft, float itemTop, float itemWidth, float selectionId, uchar red,
                  uchar green, uchar blue);
};

void OpaqueColoredPoint2DWithSize::set(float nx, float ny, float nw, float nh, float nid,
                                       uchar nr, uchar ng, uchar nb) {
    x = nx; y = ny; w = nw; h = nh; id = nid;
    r = nr; g = ng, b = nb; a = 255;
}

float OpaqueColoredPoint2DWithSize::top() const
{
    return y;
}

void OpaqueColoredPoint2DWithSize::setTop(float top)
{
    y = top;
}

void TimelineItemsGeometry::addEvent(float itemLeft, float itemTop, float itemWidth,
                                                    float selectionId, uchar red, uchar green,
                                                    uchar blue)
{
    float rowHeight = TimelineModel::defaultRowHeight();
    float itemHeight = rowHeight - itemTop;
    OpaqueColoredPoint2DWithSize *v = OpaqueColoredPoint2DWithSize::fromVertexData(
                node->geometry());
    if (prevNode.top() == rowHeight) {
        // "Z" form, bottom to top
        v[usedVertices++].set(itemLeft, rowHeight, -itemWidth, -itemHeight, selectionId, red, green,
                              blue);
        v[usedVertices++].set(itemLeft + itemWidth, rowHeight, itemWidth, -itemHeight, selectionId,
                              red, green, blue);
        v[usedVertices++].set(itemLeft, itemTop, -itemWidth, itemHeight, selectionId, red, green,
                              blue);
        v[usedVertices++].set(itemLeft + itemWidth, itemTop, itemWidth, itemHeight, selectionId,
                              red, green, blue);
        prevNode = v[usedVertices - 1];
    } else {
        if (prevNode.top() != itemTop) {
            // 2 extra vertices to degenerate the surplus triangles
            v[usedVertices++] = prevNode;
            v[usedVertices++].set(itemLeft, itemTop, -itemWidth, itemHeight, selectionId, red,
                                  green, blue);
        }
        // "Z" form, top to bottom
        v[usedVertices++].set(itemLeft, itemTop, -itemWidth, itemHeight, selectionId, red, green,
                              blue);
        v[usedVertices++].set(itemLeft + itemWidth, itemTop, itemWidth, itemHeight, selectionId,
                              red, green, blue);
        v[usedVertices++].set(itemLeft, rowHeight, - itemWidth, -itemHeight, selectionId, red,
                              green, blue);
        v[usedVertices++].set(itemLeft + itemWidth, rowHeight, itemWidth, -itemHeight, selectionId,
                              red, green, blue);
        prevNode = v[usedVertices - 1];
    }


}

OpaqueColoredPoint2DWithSize *OpaqueColoredPoint2DWithSize::fromVertexData(QSGGeometry *geometry)
{
    Q_ASSERT(geometry->attributeCount() == 4);
    Q_ASSERT(geometry->sizeOfVertex() == sizeof(OpaqueColoredPoint2DWithSize));
    const QSGGeometry::Attribute *attributes = geometry->attributes();
    Q_ASSERT(attributes[0].position == 0);
    Q_ASSERT(attributes[0].tupleSize == 2);
    Q_ASSERT(attributes[0].type == GL_FLOAT);
    Q_ASSERT(attributes[1].position == 1);
    Q_ASSERT(attributes[1].tupleSize == 2);
    Q_ASSERT(attributes[1].type == GL_FLOAT);
    Q_ASSERT(attributes[2].position == 2);
    Q_ASSERT(attributes[2].tupleSize == 1);
    Q_ASSERT(attributes[2].type == GL_FLOAT);
    Q_ASSERT(attributes[3].position == 3);
    Q_ASSERT(attributes[3].tupleSize == 4);
    Q_ASSERT(attributes[3].type == GL_UNSIGNED_BYTE);
    Q_UNUSED(attributes);
    return static_cast<OpaqueColoredPoint2DWithSize *>(geometry->vertexData());
}

void TimelineItemsGeometry::allocate(QSGMaterial *material)
{
    QSGGeometry *geometry = new QSGGeometry(OpaqueColoredPoint2DWithSize::attributes(),
                                            usedVertices);
    geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
    geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
    node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry, true);
    node->setMaterial(material);
    allocatedVertices = usedVertices;
    usedVertices = 0;
    prevNode.set(0, TimelineModel::defaultRowHeight(), 0, 0, 0, 0, 0, 0);
}

void TimelineItemsGeometry::addVertices(float itemTop)
{
    if (prevNode.top() == TimelineModel::defaultRowHeight()) {
        usedVertices += 4;
        prevNode.setTop(itemTop);
    } else {
        usedVertices += (prevNode.top() != itemTop ? 6 : 4);
        prevNode.setTop(TimelineModel::defaultRowHeight());
    }
}

class TimelineExpandedRowNode : public QSGNode {
public:
    TimelineItemsMaterial material;
    virtual ~TimelineExpandedRowNode() {}
};

static void updateNodes(int from, int to, const TimelineModel *model,
                        const TimelineRenderState *parentState, TimelineItemsRenderPassState *state)
{
    float defaultRowHeight = TimelineModel::defaultRowHeight();

    QVector<TimelineItemsGeometry> expandedPerRow(model->expandedRowCount());
    QVector<TimelineItemsGeometry> collapsedPerRow(model->collapsedRowCount());

    for (int i = from; i < to; ++i) {
        qint64 start = qMax(parentState->start(), model->startTime(i));
        qint64 end = qMin(parentState->end(), model->startTime(i) + model->duration(i));
        if (start > end)
            continue;

        float itemTop = (1.0 - model->relativeHeight(i)) * defaultRowHeight;
        expandedPerRow[model->expandedRow(i)].addVertices(itemTop);
        collapsedPerRow[model->collapsedRow(i)].addVertices(itemTop);
    }

    for (int i = 0; i < model->expandedRowCount(); ++i) {
        TimelineItemsGeometry &row = expandedPerRow[i];
        if (row.usedVertices > 0) {
            row.allocate(&static_cast<TimelineExpandedRowNode *>(
                             state->expandedRow(i))->material);
            state->expandedRow(i)->appendChildNode(row.node);
        }
    }

    for (int i = 0; i < model->collapsedRowCount(); ++i) {
        TimelineItemsGeometry &row = collapsedPerRow[i];
        if (row.usedVertices > 0) {
            row.allocate(state->collapsedRowMaterial());
            state->collapsedRow(i)->appendChildNode(row.node);
        }
    }

    for (int i = from; i < to; ++i) {
        qint64 start = qMax(parentState->start(), model->startTime(i));
        qint64 end = qMin(parentState->end(), model->startTime(i) + model->duration(i));
        if (start > end)
            continue;

        QColor color = model->color(i);
        uchar red = color.red();
        uchar green = color.green();
        uchar blue = color.blue();

        float itemWidth = end > start ? (end - start) * parentState->scale() :
                                        std::numeric_limits<float>::min();
        float itemLeft = (start - parentState->start()) * parentState->scale();

        // This has to be the exact same expression as above, to guarantee determinism.
        float itemTop = (1.0 - model->relativeHeight(i)) * defaultRowHeight;
        float selectionId = model->selectionId(i);

        expandedPerRow[model->expandedRow(i)].addEvent(itemLeft, itemTop, itemWidth, selectionId,
                                                       red, green, blue);
        collapsedPerRow[model->collapsedRow(i)].addEvent(itemLeft, itemTop, itemWidth, selectionId,
                                                         red, green, blue);
    }
}

const QSGGeometry::AttributeSet &OpaqueColoredPoint2DWithSize::attributes()
{
    static QSGGeometry::Attribute data[] = {
        QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),
        QSGGeometry::Attribute::create(1, 2, GL_FLOAT),
        QSGGeometry::Attribute::create(2, 1, GL_FLOAT),
        QSGGeometry::Attribute::create(3, 4, GL_UNSIGNED_BYTE)
    };
    static QSGGeometry::AttributeSet attrs = {
        4,
        sizeof(OpaqueColoredPoint2DWithSize),
        data
    };
    return attrs;
}

const TimelineItemsRenderPass *TimelineItemsRenderPass::instance()
{
    static const TimelineItemsRenderPass pass;
    return &pass;
}

TimelineRenderPass::State *TimelineItemsRenderPass::update(const TimelineAbstractRenderer *renderer,
                                                           const TimelineRenderState *parentState,
                                                           State *oldState, int indexFrom,
                                                           int indexTo, bool stateChanged,
                                                           qreal spacing) const
{
    Q_UNUSED(stateChanged);
    const TimelineModel *model = renderer->model();
    if (!model || indexFrom < 0 || indexTo > model->count() || indexFrom >= indexTo)
        return oldState;

    QColor selectionColor = (renderer->selectionLocked() ? QColor(96,0,255) :
                                                           QColor(Qt::blue)).lighter(130);

    TimelineItemsRenderPassState *state;
    if (oldState == 0)
        state = new TimelineItemsRenderPassState(model);
    else
        state = static_cast<TimelineItemsRenderPassState *>(oldState);


    int selectedItem = renderer->selectedItem() == -1 ? -1 :
            model->selectionId(renderer->selectedItem());

    state->updateCollapsedRowMaterial(spacing / parentState->scale(), selectedItem, selectionColor);

    if (state->indexFrom() < state->indexTo()) {
        if (indexFrom < state->indexFrom()) {
            for (int i = indexFrom; i < state->indexFrom();
                 i+= TimelineItemsGeometry::maxEventsPerNode)
                updateNodes(i, qMin(i + TimelineItemsGeometry::maxEventsPerNode,
                                    state->indexFrom()), model, parentState, state);
        }
        if (indexTo > state->indexTo()) {
            for (int i = state->indexTo(); i < indexTo; i+= TimelineItemsGeometry::maxEventsPerNode)
                updateNodes(i, qMin(i + TimelineItemsGeometry::maxEventsPerNode, indexTo), model,
                            parentState, state);
        }
    } else {
        for (int i = indexFrom; i < indexTo; i+= TimelineItemsGeometry::maxEventsPerNode)
            updateNodes(i, qMin(i + TimelineItemsGeometry::maxEventsPerNode, indexTo), model,
                        parentState, state);
    }

    if (model->expanded()) {
        for (int row = 0; row < model->expandedRowCount(); ++row) {
            TimelineExpandedRowNode *rowNode = static_cast<TimelineExpandedRowNode *>(
                        state->expandedRow(row));
            rowNode->material.setScale(
                        QVector2D(spacing / parentState->scale(),
                                  static_cast<qreal>(model->expandedRowHeight(row))) /
                                  static_cast<qreal>(TimelineModel::defaultRowHeight()));
            rowNode->material.setSelectedItem(selectedItem);
            rowNode->material.setSelectionColor(selectionColor);
        }
    }

    state->updateIndexes(indexFrom, indexTo);
    return state;
}

TimelineItemsRenderPass::TimelineItemsRenderPass()
{
}

class TimelineItemsMaterialShader : public QSGMaterialShader
{
public:
    TimelineItemsMaterialShader();

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect,
                             QSGMaterial *oldEffect);
    virtual char const *const *attributeNames() const;

private:
    virtual void initialize();

    int m_matrix_id;
    int m_scale_id;
    int m_selection_color_id;
    int m_selected_item_id;
    int m_z_range_id;
};

TimelineItemsMaterialShader::TimelineItemsMaterialShader()
    : QSGMaterialShader()
{
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/timeline/timelineitems.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/timeline/timelineitems.frag"));
}

void TimelineItemsMaterialShader::updateState(const RenderState &state, QSGMaterial *newMaterial,
                                              QSGMaterial *)
{
    if (state.isMatrixDirty()) {
        TimelineItemsMaterial *material = static_cast<TimelineItemsMaterial *>(newMaterial);
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
        program()->setUniformValue(m_scale_id, material->scale());
        program()->setUniformValue(m_selection_color_id, material->selectionColor());
        program()->setUniformValue(m_selected_item_id, material->selectedItem());
        program()->setUniformValue(m_z_range_id, GLfloat(1.0));
    }
}

char const *const *TimelineItemsMaterialShader::attributeNames() const
{
    static const char *const attr[] = {"vertexCoord", "rectSize", "selectionId", "vertexColor", 0};
    return attr;
}

void TimelineItemsMaterialShader::initialize()
{
    m_matrix_id = program()->uniformLocation("matrix");
    m_scale_id = program()->uniformLocation("scale");
    m_selection_color_id = program()->uniformLocation("selectionColor");
    m_selected_item_id = program()->uniformLocation("selectedItem");
    m_z_range_id = program()->uniformLocation("_qt_zRange");
}


TimelineItemsMaterial::TimelineItemsMaterial() : m_selectedItem(-1)
{
    setFlag(QSGMaterial::Blending, false);
}

QVector2D TimelineItemsMaterial::scale() const
{
    return m_scale;
}

void TimelineItemsMaterial::setScale(QVector2D scale)
{
    m_scale = scale;
}

float TimelineItemsMaterial::selectedItem() const
{
    return m_selectedItem;
}

void TimelineItemsMaterial::setSelectedItem(float selectedItem)
{
    m_selectedItem = selectedItem;
}

QColor TimelineItemsMaterial::selectionColor() const
{
    return m_selectionColor;
}

void TimelineItemsMaterial::setSelectionColor(QColor selectionColor)
{
    m_selectionColor = selectionColor;
}

QSGMaterialType *TimelineItemsMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *TimelineItemsMaterial::createShader() const
{
    return new TimelineItemsMaterialShader;
}

TimelineItemsRenderPassState::TimelineItemsRenderPassState(const TimelineModel *model) :
    m_indexFrom(std::numeric_limits<int>::max()), m_indexTo(-1)
{
    m_expandedRows.reserve(model->expandedRowCount());
    m_collapsedRows.reserve(model->collapsedRowCount());
    for (int i = 0; i < model->expandedRowCount(); ++i) {
        TimelineExpandedRowNode *node = new TimelineExpandedRowNode;
        node->setFlag(QSGNode::OwnedByParent, false);
        m_expandedRows << node;
    }
    for (int i = 0; i < model->collapsedRowCount(); ++i) {
        QSGNode *node = new QSGNode;
        node->setFlag(QSGNode::OwnedByParent, false);
        m_collapsedRows << node;
    }
}

TimelineItemsRenderPassState::~TimelineItemsRenderPassState()
{
    qDeleteAll(m_collapsedRows);
    qDeleteAll(m_expandedRows);
}

void TimelineItemsRenderPassState::updateIndexes(int from, int to)
{
    if (from < m_indexFrom)
        m_indexFrom = from;
    if (to > m_indexTo)
        m_indexTo = to;
}

void TimelineItemsRenderPassState::updateCollapsedRowMaterial(float xScale, int selectedItem,
                                                              QColor selectionColor)
{
    m_collapsedRowMaterial.setScale(QVector2D(xScale, 1));
    m_collapsedRowMaterial.setSelectedItem(selectedItem);
    m_collapsedRowMaterial.setSelectionColor(selectionColor);
}

} // namespace Timeline
