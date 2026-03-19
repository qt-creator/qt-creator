// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelineabstractrenderer.h"
#include "timelinerenderstate.h"

#include <QtQml/qqml.h>

namespace Timeline {

class TRACING_EXPORT TimelineRenderer : public TimelineAbstractRenderer
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TimelineRenderer(QQuickItem *parent = nullptr);
    ~TimelineRenderer() override;

    Q_INVOKABLE void selectNextFromSelectionId(int selectionId);
    Q_INVOKABLE void selectPrevFromSelectionId(int selectionId);
    Q_INVOKABLE void clearData();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    enum MatchResult { NoMatch, Cutoff, ApproximateMatch, ExactMatch };

    struct MatchParameters {
        qint64 startTime;
        qint64 endTime;
        qint64 exactTime;
        qint64 bestOffset;
    };

    static const int SafeFloatMax = 1 << 12;

    void clear();
    void resetCurrentSelection();
    int rowFromPosition(int y) const;
    TimelineRenderState *findRenderState();
    void findCurrentSelection(int mouseX, int mouseY, int width);
    MatchResult checkMatch(MatchParameters *params, int index, qint64 itemStart, qint64 itemEnd);
    MatchResult matchForward(MatchParameters *params, int index);
    MatchResult matchBackward(MatchParameters *params, int index);

    int m_currentEventIndex = -1;
    int m_currentRow = -1;
    QList<QHash<qint64, TimelineRenderState *>> m_renderStates;
    TimelineRenderState *m_lastState = nullptr;
};

} // namespace Timeline

QML_DECLARE_TYPE(Timeline::TimelineRenderer)
