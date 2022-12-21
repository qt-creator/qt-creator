// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QSGNode)
namespace Timeline {

class TimelineAbstractRenderer;
class TimelineRenderState;

class TRACING_EXPORT TimelineRenderPass {
public:
    class TRACING_EXPORT State {
    public:
        virtual const QVector<QSGNode *> &expandedRows() const;
        virtual const QVector<QSGNode *> &collapsedRows() const;
        virtual QSGNode *expandedOverlay() const;
        virtual QSGNode *collapsedOverlay() const;
        virtual ~State();
    };

    virtual ~TimelineRenderPass();
    virtual State *update(const TimelineAbstractRenderer *renderer,
                          const TimelineRenderState *parentState,
                          State *state, int indexFrom, int indexTo, bool stateChanged,
                          float spacing) const = 0;
};

} // namespace Timeline
