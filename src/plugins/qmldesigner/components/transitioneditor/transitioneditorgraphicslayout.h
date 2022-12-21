// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineitem.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsLinearLayout)

namespace QmlDesigner {

class TimelineItem;
class TimelineRulerSectionItem;
class TimelinePlaceholder;

class ModelNode;

class TransitionEditorGraphicsLayout : public TimelineItem
{
    Q_OBJECT

signals:
    void rulerClicked(const QPointF &pos);

    void zoomChanged(int factor);

public:
    TransitionEditorGraphicsLayout(QGraphicsScene *scene, TimelineItem *parent = nullptr);

    ~TransitionEditorGraphicsLayout() override;

public:
    int zoom() const;

    double rulerWidth() const;

    double rulerScaling() const;

    double rulerDuration() const;

    double endFrame() const;

    void setWidth(int width);

    void setTransition(const ModelNode &transition);

    void setDuration(qreal duration);

    void setZoom(int factor);

    void invalidate();

    int maximumScrollValue() const;

    void activate();

    TimelineRulerSectionItem *ruler() const;

private:
    QGraphicsLinearLayout *m_layout = nullptr;

    TimelineRulerSectionItem *m_rulerItem = nullptr;

    TimelinePlaceholder *m_placeholder1 = nullptr;

    TimelinePlaceholder *m_placeholder2 = nullptr;
};

} // namespace QmlDesigner
