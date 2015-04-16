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
#include <QtMath>
#include <QSGSimpleRectNode>

namespace Timeline {

QSGSimpleRectNode *createSelectionNode()
{
    QSGSimpleRectNode *selectionNode = new QSGSimpleRectNode;
    selectionNode->material()->setFlag(QSGMaterial::Blending, false);
    selectionNode->setRect(0, 0, 0, 0);
    selectionNode->setFlag(QSGNode::OwnedByParent, false);
    QSGSimpleRectNode *selectionChild = new QSGSimpleRectNode;
    selectionChild->material()->setFlag(QSGMaterial::Blending, false);
    selectionChild->setRect(0, 0, 0, 0);
    selectionNode->appendChildNode(selectionChild);
    return selectionNode;
}

class TimelineSelectionRenderPassState : public TimelineRenderPass::State {
public:
    TimelineSelectionRenderPassState();
    ~TimelineSelectionRenderPassState();

    QSGNode *expandedOverlay() const { return m_expandedOverlay; }
    QSGNode *collapsedOverlay() const { return m_collapsedOverlay; }
private:
    QSGSimpleRectNode *m_expandedOverlay;
    QSGSimpleRectNode *m_collapsedOverlay;
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

    QSGSimpleRectNode *selectionNode = static_cast<QSGSimpleRectNode *>(model->expanded() ?
                                                                        state->expandedOverlay() :
                                                                        state->collapsedOverlay());

    QSGSimpleRectNode *child = static_cast<QSGSimpleRectNode *>(selectionNode->firstChild());
    int selectedItem = renderer->selectedItem();
    if (selectedItem != -1 && selectedItem >= firstIndex && selectedItem < lastIndex) {
        qreal top = 0;
        qreal height = 0;
        if (model->expanded()) {
            int row = model->expandedRow(selectedItem);
            int rowHeight = model->expandedRowHeight(row);
            height = rowHeight * model->relativeHeight(selectedItem);
            top = model->expandedRowOffset(row) - height + rowHeight;
        } else {
            int row = model->collapsedRow(selectedItem);
            height = TimelineModel::defaultRowHeight() * model->relativeHeight(selectedItem);
            top = TimelineModel::defaultRowHeight() * (row + 1) - height;
        }

        qreal left = qMax(model->startTime(selectedItem) - parentState->start(), (qint64)0);
        qreal right = qMin(parentState->end() - parentState->start(),
                           model->endTime(selectedItem) - parentState->start());

        // Construct from upper left and lower right for better precision. When constructing from
        // left and width the error on the left border is inherited by the right border. Like this
        // they're independent.

        QRectF outer(QPointF(left * parentState->scale(), top),
                     QPointF(right * parentState->scale(), top + height));

        float scaleConversion = parentState->scale() / spacing;
        float missing = 3.0 - outer.width() / scaleConversion;
        if (missing > 0.0) {
            outer.setLeft(outer.left() - missing * scaleConversion / 2.0);
            outer.setRight(outer.right() + missing * scaleConversion / 2.0);
        }
        missing = 3.0 - outer.height();
        if (missing > 0.0)
            outer.setTop(outer.top() - missing);

        selectionNode->setRect(outer);
        selectionNode->setColor(renderer->selectionLocked() ? QColor(96,0,255) : Qt::blue);

        float childWidthThreshold = 6.0 * scaleConversion;
        if (outer.width() > childWidthThreshold && outer.height() > 6.0) {
            // Construct from upper left and lower right for better precision
            child->setRect(QRectF(QPointF(outer.left() + childWidthThreshold / 2.0,
                                          outer.top() + 3.0),
                                  QPointF(outer.right() - childWidthThreshold / 2.0,
                                          outer.bottom() - 3.0)));
            child->setColor(model->color(selectedItem));
        } else {
            child->setRect(0, 0, 0, 0);
        }
    } else {
        selectionNode->setRect(0, 0, 0, 0);
        child->setRect(0, 0, 0, 0);
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
    m_expandedOverlay(createSelectionNode()), m_collapsedOverlay(createSelectionNode())
{
}

TimelineSelectionRenderPassState::~TimelineSelectionRenderPassState()
{
    delete m_collapsedOverlay;
    delete m_expandedOverlay;
}

} // namespace Timeline
