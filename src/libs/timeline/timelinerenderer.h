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

#include "timelinezoomcontrol.h"
#include "timelinemodel.h"
#include "timelinenotesmodel.h"
#include "timelineabstractrenderer.h"

#include <QSGTransformNode>
#include <QQuickItem>

namespace Timeline {


class TIMELINE_EXPORT TimelineRenderer : public TimelineAbstractRenderer
{
    Q_OBJECT

public:
    explicit TimelineRenderer(QQuickItem *parent = 0);

    Q_INVOKABLE void selectNextFromSelectionId(int selectionId);
    Q_INVOKABLE void selectPrevFromSelectionId(int selectionId);
    Q_INVOKABLE void clearData();

protected:
    virtual QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void hoverMoveEvent(QHoverEvent *event);
    virtual void wheelEvent(QWheelEvent *event);

private:
    class TimelineRendererPrivate;
    Q_DECLARE_PRIVATE(TimelineRenderer)
};

} // namespace Timeline

QML_DECLARE_TYPE(Timeline::TimelineRenderer)
