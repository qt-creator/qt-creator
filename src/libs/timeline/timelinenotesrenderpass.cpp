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

#include "timelinenotesrenderpass.h"
#include "timelinerenderstate.h"
#include "timelinenotesmodel.h"

#include <utils/theme/theme.h>

namespace Timeline {

struct Point2DWithDistanceFromTop {
    float x, y, d;
    void set(float nx, float ny, float nd);
};

class NotesMaterial : public QSGMaterial
{
public:
    QSGMaterialType *type() const;
    QSGMaterialShader *createShader() const;
};

struct NotesGeometry
{
    static const int maxNotes;
    static const QSGGeometry::AttributeSet &point2DWithDistanceFromTop();

    static QSGGeometry *createGeometry(QVector<int> &ids, const TimelineModel *model,
                                       const TimelineRenderState *parentState, bool collapsed);
};

const int NotesGeometry::maxNotes = 0xffff / 2;

class TimelineNotesRenderPassState : public TimelineRenderPass::State
{
public:
    TimelineNotesRenderPassState(int expandedRows);
    ~TimelineNotesRenderPassState();

    QSGNode *expandedRow(int row) const { return m_expandedRows[row]; }
    QSGNode *collapsedOverlay() const { return m_collapsedOverlay; }
    const QVector<QSGNode *> &expandedRows() const { return m_expandedRows; }

    QSGGeometry *nullGeometry() { return &m_nullGeometry; }
    NotesMaterial *material() { return &m_material; }

private:
    QSGGeometryNode *createNode();

    NotesMaterial m_material;
    QSGGeometry m_nullGeometry;
    QSGGeometryNode *m_collapsedOverlay;
    QVector<QSGNode *> m_expandedRows;
};

const QSGGeometry::AttributeSet &NotesGeometry::point2DWithDistanceFromTop()
{
    static QSGGeometry::Attribute data[] = {
        QSGGeometry::Attribute::create(0, 2, GL_FLOAT, true),
        QSGGeometry::Attribute::create(1, 1, GL_FLOAT),
    };
    static QSGGeometry::AttributeSet attrs = {
        2,
        sizeof(Point2DWithDistanceFromTop),
        data
    };
    return attrs;
}

const TimelineNotesRenderPass *TimelineNotesRenderPass::instance()
{
    static const TimelineNotesRenderPass pass;
    return &pass;
}

TimelineNotesRenderPass::TimelineNotesRenderPass()
{
}

TimelineRenderPass::State *TimelineNotesRenderPass::update(const TimelineAbstractRenderer *renderer,
                                                           const TimelineRenderState *parentState,
                                                           State *oldState, int firstIndex,
                                                           int lastIndex, bool stateChanged,
                                                           qreal spacing) const
{
    Q_UNUSED(firstIndex);
    Q_UNUSED(lastIndex);
    Q_UNUSED(spacing);

    const TimelineNotesModel *notes = renderer->notes();
    const TimelineModel *model = renderer->model();

    if (!model || !notes)
        return oldState;

    TimelineNotesRenderPassState *state;
    if (oldState == 0) {
        state = new TimelineNotesRenderPassState(model->expandedRowCount());
    } else {
        if (!stateChanged && !renderer->notesDirty())
            return oldState;
        state = static_cast<TimelineNotesRenderPassState *>(oldState);
    }

    QVector<QVector<int> > expanded(model->expandedRowCount());
    QVector<int> collapsed;

    for (int i = 0; i < qMin(notes->count(), NotesGeometry::maxNotes); ++i) {
        if (notes->timelineModel(i) != model->modelId())
            continue;
        int timelineIndex = notes->timelineIndex(i);
        if (model->startTime(timelineIndex) > parentState->end() ||
                model->endTime(timelineIndex) < parentState->start())
            continue;
        expanded[model->expandedRow(timelineIndex)] << timelineIndex;
        collapsed << timelineIndex;
    }

    QSGGeometryNode *collapsedNode = static_cast<QSGGeometryNode *>(state->collapsedOverlay());

    if (collapsed.count() > 0) {
        collapsedNode->setGeometry(NotesGeometry::createGeometry(collapsed, model, parentState,
                                                                 true));
        collapsedNode->setFlag(QSGGeometryNode::OwnsGeometry, true);
    } else {
        collapsedNode->setGeometry(state->nullGeometry());
        collapsedNode->setFlag(QSGGeometryNode::OwnsGeometry, false);
    }

    for (int row = 0; row < model->expandedRowCount(); ++row) {
        QSGGeometryNode *rowNode = static_cast<QSGGeometryNode *>(state->expandedRow(row));
        if (expanded[row].isEmpty()) {
            rowNode->setGeometry(state->nullGeometry());
            rowNode->setFlag(QSGGeometryNode::OwnsGeometry, false);
        } else {
            rowNode->setGeometry(NotesGeometry::createGeometry(expanded[row], model, parentState,
                                                               false));
            collapsedNode->setFlag(QSGGeometryNode::OwnsGeometry, true);
        }
    }

    return state;
}

TimelineNotesRenderPassState::TimelineNotesRenderPassState(int numExpandedRows) :
    m_nullGeometry(NotesGeometry::point2DWithDistanceFromTop(), 0)
{
    m_material.setFlag(QSGMaterial::Blending, true);
    m_expandedRows.reserve(numExpandedRows);
    for (int i = 0; i < numExpandedRows; ++i)
        m_expandedRows << createNode();
    m_collapsedOverlay = createNode();
}

TimelineNotesRenderPassState::~TimelineNotesRenderPassState()
{
    qDeleteAll(m_expandedRows);
    delete m_collapsedOverlay;
}

QSGGeometryNode *TimelineNotesRenderPassState::createNode()
{
    QSGGeometryNode *node = new QSGGeometryNode;
    node->setGeometry(&m_nullGeometry);
    node->setMaterial(&m_material);
    node->setFlag(QSGNode::OwnedByParent, false);
    return node;
}

QSGGeometry *NotesGeometry::createGeometry(QVector<int> &ids, const TimelineModel *model,
                                           const TimelineRenderState *parentState, bool collapsed)
{
    float rowHeight = TimelineModel::defaultRowHeight();
    QSGGeometry *geometry = new QSGGeometry(point2DWithDistanceFromTop(),
                                            ids.count() * 2);
    Q_ASSERT(geometry->vertexData());
    geometry->setDrawingMode(GL_LINES);
    geometry->setLineWidth(3);
    Point2DWithDistanceFromTop *v =
            static_cast<Point2DWithDistanceFromTop *>(geometry->vertexData());
    for (int i = 0; i < ids.count(); ++i) {
        int timelineIndex = ids[i];
        float horizontalCenter = ((model->startTime(timelineIndex) +
                                   model->endTime(timelineIndex)) / (qint64)2 -
                                  parentState->start()) * parentState->scale();
        float verticalStart = (collapsed ? (model->collapsedRow(timelineIndex) + 0.1) : 0.1) *
                rowHeight;
        float verticalEnd = verticalStart + 0.8 * rowHeight;
        v[i * 2].set(horizontalCenter, verticalStart, 0);
        v[i * 2 + 1].set(horizontalCenter, verticalEnd, 1);
    }
    return geometry;
}

class NotesMaterialShader : public QSGMaterialShader
{
public:
    NotesMaterialShader();

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect,
                             QSGMaterial *oldEffect);
    virtual char const *const *attributeNames() const;

private:
    virtual void initialize();

    int m_matrix_id;
    int m_z_range_id;
    int m_color_id;
};

NotesMaterialShader::NotesMaterialShader()
    : QSGMaterialShader()
{
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/timeline/notes.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/timeline/notes.frag"));
}

void NotesMaterialShader::updateState(const RenderState &state, QSGMaterial *, QSGMaterial *)
{
    if (state.isMatrixDirty()) {
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
        program()->setUniformValue(m_z_range_id, GLfloat(1.0));
        const QColor notesColor = Utils::creatorTheme()
                ? Utils::creatorTheme()->color(Utils::Theme::Timeline_HighlightColor)
                : QColor(255, 165, 0);
        program()->setUniformValue(m_color_id, notesColor);
    }
}

char const *const *NotesMaterialShader::attributeNames() const
{
    static const char *const attr[] = {"vertexCoord", "distanceFromTop", 0};
    return attr;
}

void NotesMaterialShader::initialize()
{
    m_matrix_id = program()->uniformLocation("matrix");
    m_z_range_id = program()->uniformLocation("_qt_zRange");
    m_color_id = program()->uniformLocation("notesColor");
}

QSGMaterialType *NotesMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *NotesMaterial::createShader() const
{
    return new NotesMaterialShader;
}

void Point2DWithDistanceFromTop::set(float nx, float ny, float nd)
{
    x = nx; y = ny; d = nd;
}

} // namespace Timeline
