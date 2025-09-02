// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinerenderpass.h"

namespace Timeline {

const QList<QSGNode *> &TimelineRenderPass::State::expandedRows() const
{
    static const QList<QSGNode *> empty;
    return empty;
}

const QList<QSGNode *> &TimelineRenderPass::State::collapsedRows() const
{
    static const QList<QSGNode *> empty;
    return empty;
}

QSGNode *TimelineRenderPass::State::expandedOverlay() const
{
    return nullptr;
}

QSGNode *TimelineRenderPass::State::collapsedOverlay() const
{
    return nullptr;
}

TimelineRenderPass::State::~State()
{
}

TimelineRenderPass::~TimelineRenderPass() {}

} // namespace Timeline
