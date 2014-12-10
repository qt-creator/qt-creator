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

#ifndef TIMELINERENDERER_H
#define TIMELINERENDERER_H

#include "timelinezoomcontrol.h"
#include "timelinemodel.h"
#include "timelinenotesmodel.h"
#include "timelineabstractrenderer.h"

#include <QSGTransformNode>
#include <QQuickItem>

namespace Timeline {

class TimelineRenderPass;
class TimelineRenderState;

class TIMELINE_EXPORT TimelineRenderer : public TimelineAbstractRenderer
{
    Q_OBJECT

public:
    explicit TimelineRenderer(QQuickItem *parent = 0);

    Q_INVOKABLE void selectNextFromSelectionId(int selectionId);
    Q_INVOKABLE void selectPrevFromSelectionId(int selectionId);

    // TODO: We could add some Q_INVOKABLE functions to enable or disable render passes when the the
    // need arises.

signals:
    void itemPressed(int pressedItem);

public slots:
    void clearData();

protected:
    virtual QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void hoverMoveEvent(QHoverEvent *event);

private:
    class TimelineRendererPrivate;
    Q_DECLARE_PRIVATE(TimelineRenderer)
};

} // namespace Timeline

QML_DECLARE_TYPE(Timeline::TimelineRenderer)

#endif // TIMELINERENDERER_H
