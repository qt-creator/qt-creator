/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "timelineselectionrenderpass.h"
#include "timelineitemsrenderpass.h"
#include <QtMath>
#include <QSGSimpleRectNode>

namespace Timeline {

QSGGeometryNode *createSelectionNode(QSGMaterial *material)
{
    QSGGeometryNode *selectionNode = new QSGGeometryNode;
    selectionNode->setMaterial(material);
    selectionNode->setFlag(QSGNode::OwnsMaterial, false);
    QSGGeometry *geometry = new QSGGeometry(OpaqueColoredPoint2DWithSize::attributes(), 4);
    geometry->setDrawingMode(GL_TRIANGLE_STRIP);
    OpaqueColoredPoint2DWithSize *v = OpaqueColoredPoint2DWithSize::fromVertexData(geometry);
    for (int i = 0; i < 4; ++i)
        v[i].set(0, 0, 0, 0, 0, 0, 0, 0);
    selectionNode->setGeometry(geometry);
    selectionNode->setFlag(QSGNode::OwnsGeometry, true);
    selectionNode->setFlag(QSGNode::OwnedByParent, false);
    return selectionNode;
}

class TimelineSelectionRenderPassState : public TimelineRenderPass::State {
public:
    TimelineSelectionRenderPassState();
    ~TimelineSelectionRenderPassState();

    QSGNode *expandedOverlay() const { return m_expandedOverlay; }
    QSGNode *collapsedOverlay() const { return m_collapsedOverlay; }
    TimelineItemsMaterial *material() { return &m_material; }

private:
    QSGNode *m_expandedOverlay;
    QSGNode *m_collapsedOverlay;
    TimelineItemsMaterial m_material;
};

TimelineRenderPass::State *TimelineSelectionRenderPass::update(
        const TimelineAbstractRenderer *renderer, const TimelineRenderState *parentState,
        State *oldState, int firstIndex, int lastIndex, bool stateChanged, qreal spacing) const
{
    Q_UNUSED(stateChanged);

    const TimelineModel *model = renderer->model();
    if (!model || model->isEmpty())
        return oldState;

    TimelineSelectionRenderPassState *state;

    if (oldState == 0)
        state = new TimelineSelectionRenderPassState;
    else
        state = static_cast<TimelineSelectionRenderPassState *>(oldState);

    int selectedItem = renderer->selectedItem();
    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(model->expanded() ?
                                                               state->expandedOverlay() :
                                                               state->collapsedOverlay());

    if (selectedItem != -1 && selectedItem >= firstIndex && selectedItem < lastIndex) {
        qreal top = 0;
        qreal height = 0;
        if (model->expanded()) {
            int row = model->expandedRow(selectedItem);
            int rowHeight = model->expandedRowHeight(row);
            height = rowHeight * model->relativeHeight(selectedItem);
            top = (model->expandedRowOffset(row) + rowHeight) - height;
        } else {
            int row = model->collapsedRow(selectedItem);
            height = TimelineModel::defaultRowHeight() * model->relativeHeight(selectedItem);
            top = TimelineModel::defaultRowHeight() * (row + 1) - height;
        }

        qint64 startTime = model->startTime(selectedItem);
        qint64 left = qMax(startTime - parentState->start(), (qint64)0);
        qint64 width = qMin(parentState->end() - startTime, model->duration(selectedItem));

        // Construct from upper left and lower right for better precision. When constructing from
        // left and width the error on the left border is inherited by the right border. Like this
        // they're independent.

        QRectF position(left * parentState->scale(), top, width * parentState->scale(),  height);

        QColor itemColor = model->color(selectedItem);
        uchar red = itemColor.red();
        uchar green = itemColor.green();
        uchar blue = itemColor.blue();
        int selectionId = model->selectionId(selectedItem);

        OpaqueColoredPoint2DWithSize *v = OpaqueColoredPoint2DWithSize::fromVertexData(
                    node->geometry());
        v[0].set(position.left(), position.bottom(), -position.width(), -position.height(),
                 selectionId, red, green, blue);
        v[1].set(position.right(), position.bottom(), position.width(), -position.height(),
                 selectionId, red, green, blue);
        v[2].set(position.left(), position.top(), -position.width(), position.height(),
                 selectionId, red, green, blue);
        v[3].set(position.right(), position.top(), position.width(), position.height(),
                 selectionId, red, green, blue);
        state->material()->setSelectionColor(renderer->selectionLocked() ? QColor(96,0,255) :
                                                                           Qt::blue);
        state->material()->setSelectedItem(selectionId);
        state->material()->setScale(QVector2D(spacing / parentState->scale(), 1));
        node->markDirty(QSGNode::DirtyMaterial | QSGNode::DirtyGeometry);
    } else {
        OpaqueColoredPoint2DWithSize *v = OpaqueColoredPoint2DWithSize::fromVertexData(
                    node->geometry());
        for (int i = 0; i < 4; ++i)
            v[i].set(0, 0, 0, 0, 0, 0, 0, 0);
        node->markDirty(QSGNode::DirtyGeometry);
    }
    return state;
}

const TimelineSelectionRenderPass *TimelineSelectionRenderPass::instance()
{
    static const TimelineSelectionRenderPass pass;
    return &pass;
}

TimelineSelectionRenderPass::TimelineSelectionRenderPass()
{
}

TimelineSelectionRenderPassState::TimelineSelectionRenderPassState() :
    m_expandedOverlay(0), m_collapsedOverlay(0)
{
    m_expandedOverlay = createSelectionNode(&m_material);
    m_collapsedOverlay = createSelectionNode(&m_material);
}

TimelineSelectionRenderPassState::~TimelineSelectionRenderPassState()
{
    delete m_collapsedOverlay;
    delete m_expandedOverlay;
}

} // namespace Timeline
