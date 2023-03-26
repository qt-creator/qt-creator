// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    const QVector<QSGNode *> &expandedRows() const final { return m_expandedRows; }
    const QVector<QSGNode *> &collapsedRows() const final { return m_collapsedRows; }
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
    // Vertex indices are 16bit
    static const int maxVerticesPerNode = 0xffff;

    enum VerticesPerEvent {
        NoVertices = 0,
        VerticesForSameHeight = 4,
        VerticesForDifferentHeight = 6
    };

    TimelineItemsGeometry();
    uint usedVertices;

    OpaqueColoredPoint2DWithSize prevNode;
    OpaqueColoredPoint2DWithSize currentNode;

    QSGGeometryNode *node;

    void initNodes();

    bool isEmpty() const;

    void allocate(QSGMaterial *material);
    VerticesPerEvent addVertices();
    void addEvent();

    void nextNode(float itemLeft, float itemTop, float itemWidth = 0, float selectionId = 0,
                  uchar red = 0, uchar green = 0, uchar blue = 0);
    void updateCurrentNode(float itemRight, float itemTop);
};

class NodeUpdater {
public:
    NodeUpdater(const TimelineModel *model, const TimelineRenderState *parentState,
                TimelineItemsRenderPassState *state, int indexFrom, int indexTo);
    void run();

private:
    static const int s_maxNumItems = 1 << 20;
    static const qint64 s_invalidTimestamp = 0xffffffffffffffffLL;

    struct ItemDescription
    {
        uchar red;
        uchar green;
        uchar blue;

        float width;
        float left;
        float right;

        float top;
        float selectionId;
    };

    void calculateDistances();
    int updateVertices(TimelineItemsGeometry &geometry, const QVarLengthArray<qint64> &distances,
                       qint64 minDistance, float itemTop, int i) const;
    void addEvent(TimelineItemsGeometry &geometry, const QVarLengthArray<qint64> &distances,
                  qint64 minDistance, const ItemDescription &item, int i) const;
    int updateNodes(const int from, const int to) const;

    const TimelineModel *m_model;
    const TimelineRenderState *m_parentState;
    const int m_indexFrom;
    const int m_indexTo;

    TimelineItemsRenderPassState *m_state;
    QVarLengthArray<qint64> m_collapsedDistances;
    QVarLengthArray<qint64> m_expandedDistances;
    qint64 m_minCollapsedDistance;
    qint64 m_minExpandedDistance;
};

void OpaqueColoredPoint2DWithSize::set(float nx, float ny, float nw, float nh, float nid,
                                       uchar nr, uchar ng, uchar nb, uchar d) {
    x = nx; y = ny; w = nw; h = nh; id = nid;
    r = nr; g = ng, b = nb; a = d;
}

float OpaqueColoredPoint2DWithSize::top() const
{
    return id < 0 ? (y / -id) : y;
}

void OpaqueColoredPoint2DWithSize::update(float nr, float ny)
{
    if (a <= MaximumDirection) {
        a += MaximumDirection;
        id = -2;
    } else {
        --id;
    }

    y += ny;
    w = nr - x;
}

OpaqueColoredPoint2DWithSize::Direction OpaqueColoredPoint2DWithSize::direction() const
{
    return static_cast<Direction>(a > MaximumDirection ? a - MaximumDirection : a);
}

void OpaqueColoredPoint2DWithSize::setCommon(const OpaqueColoredPoint2DWithSize *master)
{
    a = 255;
    if (master->a > MaximumDirection) {
        id = std::numeric_limits<float>::lowest();
        r = g = b = 128;
    } else {
        id = master->id;
        r = master->r;
        g = master->g;
        b = master->b;
    }
}

void OpaqueColoredPoint2DWithSize::setLeft(const OpaqueColoredPoint2DWithSize *master)
{
    w = -master->w;
    x = master->x;
}

void OpaqueColoredPoint2DWithSize::setRight(const OpaqueColoredPoint2DWithSize *master)
{
    w = master->w;
    x = master->x + master->w;
}

void OpaqueColoredPoint2DWithSize::setTop(const OpaqueColoredPoint2DWithSize *master)
{
    y = master->id < 0 ? master->y / -master->id : master->y;
    h = TimelineModel::defaultRowHeight() - y;
}

void OpaqueColoredPoint2DWithSize::setBottom(const OpaqueColoredPoint2DWithSize *master)
{
    y = TimelineModel::defaultRowHeight();
    h = (master->id < 0 ? master->y / -master->id : master->y) - TimelineModel::defaultRowHeight();
}

void OpaqueColoredPoint2DWithSize::setBottomLeft(const OpaqueColoredPoint2DWithSize *master)
{
    setCommon(master);
    setLeft(master);
    setBottom(master);
}

void OpaqueColoredPoint2DWithSize::setBottomRight(const OpaqueColoredPoint2DWithSize *master)
{
    setCommon(master);
    setRight(master);
    setBottom(master);
}

void OpaqueColoredPoint2DWithSize::setTopLeft(const OpaqueColoredPoint2DWithSize *master)
{
    setCommon(master);
    setLeft(master);
    setTop(master);
}

void OpaqueColoredPoint2DWithSize::setTopRight(const OpaqueColoredPoint2DWithSize *master)
{
    setCommon(master);
    setRight(master);
    setTop(master);
}

void TimelineItemsGeometry::addEvent()
{
    OpaqueColoredPoint2DWithSize *v =
            OpaqueColoredPoint2DWithSize::fromVertexData(node->geometry());
    switch (currentNode.direction()) {
    case OpaqueColoredPoint2DWithSize::BottomToTop:
        v[usedVertices++].setBottomLeft(&currentNode);
        v[usedVertices++].setBottomRight(&currentNode);
        v[usedVertices++].setTopLeft(&currentNode);
        v[usedVertices++].setTopRight(&currentNode);
        break;
    case OpaqueColoredPoint2DWithSize::TopToBottom:
        if (prevNode.top() != currentNode.top()) {
            v[usedVertices++].setTopRight(&prevNode);
            v[usedVertices++].setTopLeft(&currentNode);
        }
        v[usedVertices++].setTopLeft((&currentNode));
        v[usedVertices++].setTopRight((&currentNode));
        v[usedVertices++].setBottomLeft((&currentNode));
        v[usedVertices++].setBottomRight((&currentNode));
        break;
    default:
        break;
    }
}

OpaqueColoredPoint2DWithSize *OpaqueColoredPoint2DWithSize::fromVertexData(QSGGeometry *geometry)
{
    Q_ASSERT(geometry->attributeCount() == 4);
    Q_ASSERT(geometry->sizeOfVertex() == sizeof(OpaqueColoredPoint2DWithSize));
    const QSGGeometry::Attribute *attributes = geometry->attributes();
    Q_ASSERT(attributes[0].position == 0);
    Q_ASSERT(attributes[0].tupleSize == 2);
    Q_ASSERT(attributes[0].type == QSGGeometry::FloatType);
    Q_ASSERT(attributes[1].position == 1);
    Q_ASSERT(attributes[1].tupleSize == 2);
    Q_ASSERT(attributes[1].type == QSGGeometry::FloatType);
    Q_ASSERT(attributes[2].position == 2);
    Q_ASSERT(attributes[2].tupleSize == 1);
    Q_ASSERT(attributes[2].type == QSGGeometry::FloatType);
    Q_ASSERT(attributes[3].position == 3);
    Q_ASSERT(attributes[3].tupleSize == 4);
    Q_ASSERT(attributes[3].type == QSGGeometry::UnsignedByteType);
    Q_UNUSED(attributes)
    return static_cast<OpaqueColoredPoint2DWithSize *>(geometry->vertexData());
}

TimelineItemsGeometry::TimelineItemsGeometry() : usedVertices(0), node(nullptr)
{
    initNodes();
}

void TimelineItemsGeometry::initNodes()
{
    currentNode.set(0, TimelineModel::defaultRowHeight(), 0, 0, 0, 0, 0, 0,
                    OpaqueColoredPoint2DWithSize::InvalidDirection);
    prevNode.set(0, TimelineModel::defaultRowHeight(), 0, 0, 0, 0, 0, 0,
                 OpaqueColoredPoint2DWithSize::InvalidDirection);
}

bool TimelineItemsGeometry::isEmpty() const
{
    return usedVertices == 0 &&
            currentNode.direction() == OpaqueColoredPoint2DWithSize::InvalidDirection;
}

void TimelineItemsGeometry::allocate(QSGMaterial *material)
{
    QSGGeometry *geometry = new QSGGeometry(OpaqueColoredPoint2DWithSize::attributes(),
                                            usedVertices);
    Q_ASSERT(geometry->vertexData());
    geometry->setIndexDataPattern(QSGGeometry::StaticPattern);
    geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
    node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry, true);
    node->setMaterial(material);
    usedVertices = 0;
    initNodes();
}

TimelineItemsGeometry::VerticesPerEvent TimelineItemsGeometry::addVertices()
{
    switch (currentNode.direction()) {
    case OpaqueColoredPoint2DWithSize::BottomToTop:
        usedVertices += VerticesForSameHeight;
        return VerticesForSameHeight;
    case OpaqueColoredPoint2DWithSize::TopToBottom: {
        VerticesPerEvent vertices = (prevNode.top() != currentNode.top() ?
                    VerticesForDifferentHeight : VerticesForSameHeight);
        usedVertices += vertices;
        return vertices;
    } default:
        return NoVertices;
    }
}

void TimelineItemsGeometry::nextNode(float itemLeft, float itemTop, float itemWidth,
                                     float selectionId, uchar red, uchar green, uchar blue)
{
    prevNode = currentNode;
    currentNode.set(itemLeft, itemTop, itemWidth, TimelineModel::defaultRowHeight() - itemTop,
                    selectionId, red, green, blue,
                    currentNode.direction() == OpaqueColoredPoint2DWithSize::BottomToTop ?
                        OpaqueColoredPoint2DWithSize::TopToBottom :
                        OpaqueColoredPoint2DWithSize::BottomToTop);
}

void TimelineItemsGeometry::updateCurrentNode(float itemRight, float itemTop)
{
    currentNode.update(itemRight, itemTop);
}

class TimelineExpandedRowNode : public QSGNode {
public:
    TimelineItemsMaterial material;
    ~TimelineExpandedRowNode() override {}
};

static qint64 startTime(const TimelineModel *model, const TimelineRenderState *parentState, int i)
{
    return qMax(parentState->start(), model->startTime(i));
}

static qint64 endTime(const TimelineModel *model, const TimelineRenderState *parentState, int i)
{
    return qMin(parentState->end(), model->startTime(i) + model->duration(i));
}

const QSGGeometry::AttributeSet &OpaqueColoredPoint2DWithSize::attributes()
{
    static const QSGGeometry::Attribute data[] = {
        // vec4 vertexCoord
        QSGGeometry::Attribute::createWithAttributeType(0, 2, QSGGeometry::FloatType,
                                                        QSGGeometry::PositionAttribute),
        // vec2 rectSize
        QSGGeometry::Attribute::createWithAttributeType(1, 2, QSGGeometry::FloatType,
                                                        QSGGeometry::UnknownAttribute),
        // float selectionId
        QSGGeometry::Attribute::createWithAttributeType(2, 1, QSGGeometry::FloatType,
                                                        QSGGeometry::UnknownAttribute),
        // vec4 vertexColor
        QSGGeometry::Attribute::createWithAttributeType(3, 4, QSGGeometry::UnsignedByteType,
                                                        QSGGeometry::ColorAttribute),
    };
    static const QSGGeometry::AttributeSet attrs = {
        sizeof(data) / sizeof(data[0]),
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
                                                           float spacing) const
{
    Q_UNUSED(stateChanged)
    const TimelineModel *model = renderer->model();
    if (!model || indexFrom < 0 || indexTo > model->count() || indexFrom >= indexTo)
        return oldState;

    QColor selectionColor = (renderer->selectionLocked() ? QColor(96,0,255) :
                                                           QColor(Qt::blue)).lighter(130);

    TimelineItemsRenderPassState *state;
    if (oldState == nullptr)
        state = new TimelineItemsRenderPassState(model);
    else
        state = static_cast<TimelineItemsRenderPassState *>(oldState);


    int selectedItem = renderer->selectedItem() == -1 ? -1 :
            model->selectionId(renderer->selectedItem());

    state->updateCollapsedRowMaterial(spacing / parentState->scale(), selectedItem, selectionColor);

    if (state->indexFrom() < state->indexTo()) {
        if (indexFrom < state->indexFrom() || indexTo > state->indexTo())
            NodeUpdater(model, parentState, state, indexFrom, indexTo).run();
    } else if (indexFrom < indexTo) {
        NodeUpdater(model, parentState, state, indexFrom, indexTo).run();
    }

    if (model->expanded()) {
        for (int row = 0; row < model->expandedRowCount(); ++row) {
            TimelineExpandedRowNode *rowNode = static_cast<TimelineExpandedRowNode *>(
                        state->expandedRow(row));
            rowNode->material.setScale(
                        QVector2D(spacing / parentState->scale(),
                                  static_cast<float>(model->expandedRowHeight(row))) /
                                  static_cast<float>(TimelineModel::defaultRowHeight()));
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

    bool updateUniformData(RenderState &state, QSGMaterial *newEffect,
                           QSGMaterial *oldEffect) override;
};

TimelineItemsMaterialShader::TimelineItemsMaterialShader()
    : QSGMaterialShader()
{
    setShaderFileName(VertexStage, ":/qt/qml/QtCreator/Tracing/timelineitems_qt6.vert.qsb");
    setShaderFileName(FragmentStage, ":/qt/qml/QtCreator/Tracing/timelineitems_qt6.frag.qsb");
}

bool TimelineItemsMaterialShader::updateUniformData(RenderState &state,
                                                    QSGMaterial *newMaterial, QSGMaterial *)
{
    QByteArray *buf = state.uniformData();
    auto material = static_cast<const TimelineItemsMaterial *>(newMaterial);

    // mat4 matrix
    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
    }

    // vec2 scale
    const QVector2D scale = material->scale();
    memcpy(buf->data() + 64, &scale, 8);

    // vec4 selectionColor
    const QColor color = material->selectionColor();
    const float colorArray[] = { color.redF(), color.greenF(), color.blueF(), color.alphaF() };
    memcpy(buf->data() + 80, colorArray, 16);

    // float selectedItem
    const float selectedItem = material->selectedItem();
    memcpy(buf->data() + 96, &selectedItem, 4);

    return true;
}

TimelineItemsMaterial::TimelineItemsMaterial() : m_selectedItem(-1)
{
    setFlag(QSGMaterial::Blending, false);
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    setFlag(QSGMaterial::NoBatching, true);
#else
    setFlag(QSGMaterial::CustomCompileStep, true);
#endif // >= Qt 6.3/6.0
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

QSGMaterialShader *TimelineItemsMaterial::createShader(QSGRendererInterface::RenderMode) const
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

NodeUpdater::NodeUpdater(const TimelineModel *model, const TimelineRenderState *parentState,
                         TimelineItemsRenderPassState *state, int indexFrom, int indexTo) :
    m_model(model), m_parentState(parentState), m_indexFrom(indexFrom), m_indexTo(indexTo),
    m_state(state), m_minCollapsedDistance(0), m_minExpandedDistance(0)
{
}

void NodeUpdater::calculateDistances()
{
    int numItems = m_indexTo - m_indexFrom;

    m_collapsedDistances.resize(numItems);
    m_expandedDistances.resize(numItems);
    QVarLengthArray<qint64> startsPerExpandedRow(m_model->expandedRowCount());
    QVarLengthArray<qint64> startsPerCollapsedRow(m_model->collapsedRowCount());
    memset(startsPerCollapsedRow.data(), 0xff, startsPerCollapsedRow.size());
    memset(startsPerExpandedRow.data(), 0xff, startsPerExpandedRow.size());
    for (int i = m_indexFrom; i < m_indexTo; ++i) {
        // Add some "random" factor. Distances below 256ns cannot be properly displayed
        // anyway and if all events have the same distance from one another, then we'd merge
        // them all together otherwise.
        qint64 start = startTime(m_model, m_parentState, i) + (i % 256);
        qint64 end = endTime(m_model, m_parentState, i) + (i % 256);
        if (start > end) {
            m_collapsedDistances[i - m_indexFrom] = m_expandedDistances[i - m_indexFrom] = 0;
            continue;
        }

        qint64 &collapsedStart = startsPerCollapsedRow[m_model->collapsedRow(i)];
        m_collapsedDistances[i - m_indexFrom] = (collapsedStart != s_invalidTimestamp) ?
                    end - collapsedStart : std::numeric_limits<qint64>::max();
        collapsedStart = start;

        qint64 &expandedStart = startsPerExpandedRow[m_model->expandedRow(i)];
        m_expandedDistances[i - m_indexFrom] = (expandedStart != s_invalidTimestamp) ?
                    end - expandedStart : std::numeric_limits<qint64>::max();
        expandedStart = start;
    }

    QVarLengthArray<qint64> sorted = m_collapsedDistances;
    std::sort(sorted.begin(), sorted.end());
    m_minCollapsedDistance = sorted[numItems - s_maxNumItems];
    sorted = m_expandedDistances;
    std::sort(sorted.begin(), sorted.end());
    m_minExpandedDistance = sorted[numItems - s_maxNumItems];
}

int NodeUpdater::updateVertices(TimelineItemsGeometry &geometry,
                                const QVarLengthArray<qint64> &distances, qint64 minDistance,
                                float itemTop, int i) const
{
    int vertices = 0;
    if (geometry.isEmpty()) {
        // We'll run another addVertices() on each row with content after the loop.
        // Reserve some space for that.
        vertices = TimelineItemsGeometry::VerticesForDifferentHeight;
        geometry.nextNode(0, itemTop);
    } else if (distances.isEmpty() || distances[i - m_indexFrom] > minDistance) {
        vertices = geometry.addVertices();
        geometry.nextNode(0, itemTop);
    } else {
        geometry.updateCurrentNode(0, itemTop);
    }
    return vertices;
}

void NodeUpdater::addEvent(TimelineItemsGeometry &geometry,
                           const QVarLengthArray<qint64> &distances, qint64 minDistance,
                           const NodeUpdater::ItemDescription &item, int i) const
{
    if (geometry.isEmpty()) {
        geometry.nextNode(item.left, item.top, item.width, item.selectionId, item.red,
                          item.green, item.blue);
    } else if (distances.isEmpty() || distances[i - m_indexFrom] > minDistance) {
        geometry.addEvent();
        geometry.nextNode(item.left, item.top, item.width, item.selectionId, item.red,
                          item.green, item.blue);
    } else {
        geometry.updateCurrentNode(item.right, item.top);
    }
}

int NodeUpdater::updateNodes(const int from, const int to) const
{
    float defaultRowHeight = TimelineModel::defaultRowHeight();

    QVector<TimelineItemsGeometry> expandedPerRow(m_model->expandedRowCount());
    QVector<TimelineItemsGeometry> collapsedPerRow(m_model->collapsedRowCount());

    int collapsedVertices = 0;
    int expandedVertices = 0;
    int lastEvent = from;
    for (;lastEvent < to
         && collapsedVertices < TimelineItemsGeometry::maxVerticesPerNode
         && expandedVertices < TimelineItemsGeometry::maxVerticesPerNode;
         ++lastEvent) {
        qint64 start = startTime(m_model, m_parentState, lastEvent);
        qint64 end = endTime(m_model, m_parentState, lastEvent);
        if (start > end)
            continue;

        float itemTop = (1.0 - m_model->relativeHeight(lastEvent)) * defaultRowHeight;

        expandedVertices += updateVertices(expandedPerRow[m_model->expandedRow(lastEvent)],
                m_expandedDistances, m_minExpandedDistance, itemTop, lastEvent);
        collapsedVertices += updateVertices(collapsedPerRow[m_model->collapsedRow(lastEvent)],
                m_collapsedDistances, m_minCollapsedDistance, itemTop, lastEvent);
    }

    for (int i = 0, end = m_model->expandedRowCount(); i < end; ++i) {
        TimelineItemsGeometry &row = expandedPerRow[i];
        if (row.currentNode.direction() != OpaqueColoredPoint2DWithSize::InvalidDirection)
            row.addVertices();
        if (row.usedVertices > 0) {
            row.allocate(&static_cast<TimelineExpandedRowNode *>(
                             m_state->expandedRow(i))->material);
            m_state->expandedRow(i)->appendChildNode(row.node);
        }
    }

    for (int i = 0; i < m_model->collapsedRowCount(); ++i) {
        TimelineItemsGeometry &row = collapsedPerRow[i];
        if (row.currentNode.direction() != OpaqueColoredPoint2DWithSize::InvalidDirection)
            row.addVertices();
        if (row.usedVertices > 0) {
            row.allocate(m_state->collapsedRowMaterial());
            m_state->collapsedRow(i)->appendChildNode(row.node);
        }
    }

    ItemDescription item;
    for (int i = from; i < lastEvent; ++i) {
        qint64 start = startTime(m_model, m_parentState, i);
        qint64 end = endTime(m_model, m_parentState, i);
        if (start > end)
            continue;

        QRgb color = m_model->color(i);
        item.red = qRed(color);
        item.green = qGreen(color);
        item.blue = qBlue(color);

        item.width = end > start ? (end - start) * m_parentState->scale() :
                                   std::numeric_limits<float>::min();
        item.left = (start - m_parentState->start()) * m_parentState->scale();
        item.right = (end - m_parentState->start()) * m_parentState->scale();

        // This has to be the exact same expression as above, to guarantee determinism.
        item.top = (1.0 - m_model->relativeHeight(i)) * defaultRowHeight;
        item.selectionId = m_model->selectionId(i);

        addEvent(expandedPerRow[m_model->expandedRow(i)], m_expandedDistances,
                m_minExpandedDistance, item, i);
        addEvent(collapsedPerRow[m_model->collapsedRow(i)], m_collapsedDistances,
                m_minCollapsedDistance, item, i);
    }

    for (int i = 0, end = m_model->expandedRowCount(); i < end; ++i) {
        TimelineItemsGeometry &row = expandedPerRow[i];
        if (row.currentNode.direction() != OpaqueColoredPoint2DWithSize::InvalidDirection)
            row.addEvent();
    }

    for (int i = 0, end = m_model->collapsedRowCount(); i < end; ++i) {
        TimelineItemsGeometry &row = collapsedPerRow[i];
        if (row.currentNode.direction() != OpaqueColoredPoint2DWithSize::InvalidDirection)
            row.addEvent();
    }

    return lastEvent;
}

void NodeUpdater::run()
{
    if (m_indexTo - m_indexFrom > s_maxNumItems)
        calculateDistances();

    if (m_state->indexFrom() < m_state->indexTo()) {
        if (m_indexFrom < m_state->indexFrom()) {
            for (int i = m_indexFrom; i < m_state->indexFrom();)
                i = updateNodes(i, m_state->indexFrom());
        }
        if (m_indexTo > m_state->indexTo()) {
            for (int i = m_state->indexTo(); i < m_indexTo;)
                i = updateNodes(i, m_indexTo);
        }
    } else {
        for (int i = m_indexFrom; i < m_indexTo;)
            i = updateNodes(i, m_indexTo);
    }
}

} // namespace Timeline
