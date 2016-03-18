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

#pragma once

#include <QSGNode>
#include "timelinerenderpass.h"
#include "timelinemodel.h"

namespace Timeline {

class TIMELINE_EXPORT TimelineRenderState {
public:
    TimelineRenderState(qint64 start, qint64 end, qreal scale, int numPasses);
    ~TimelineRenderState();

    qint64 start() const;
    qint64 end() const;
    qreal scale() const;

    TimelineRenderPass::State *passState(int i);
    const TimelineRenderPass::State *passState(int i) const;
    void setPassState(int i, TimelineRenderPass::State *state);

    const QSGNode *expandedRowRoot() const;
    const QSGNode *collapsedRowRoot() const;
    const QSGNode *expandedOverlayRoot() const;
    const QSGNode *collapsedOverlayRoot() const;

    QSGNode *expandedRowRoot();
    QSGNode *collapsedRowRoot();
    QSGNode *expandedOverlayRoot();
    QSGNode *collapsedOverlayRoot();

    bool isEmpty() const;
    void assembleNodeTree(const TimelineModel *model, int defaultRowHeight, int defaultRowOffset);
    void updateExpandedRowHeights(const TimelineModel *model, int defaultRowHeight,
                                  int defaultRowOffset);
    QSGTransformNode *finalize(QSGNode *oldNode, bool expanded, const QMatrix4x4 &transform);

private:
    class TimelineRenderStatePrivate;
    TimelineRenderStatePrivate *d_ptr;
    Q_DECLARE_PRIVATE(TimelineRenderState)
};

} // namespace Timeline
