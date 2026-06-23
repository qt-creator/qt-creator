// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampler.h"

namespace QmlProfiler::Internal {

// Records a trace by periodically suspending the target process and walking
// every thread's call stack, then symbolizing the samples (see macsampler.cpp).
// macOS only.
class QMLPROFILER_EXPORT CallStackSampler : public Sampler
{
public:
    QString displayName() const override;
    bool isAvailable(QString *error = nullptr) const override;
    QtTaskTree::ExecutableItem recordRecipe(
        const std::shared_ptr<RecordingSession> &session) const override;
};

} // namespace QmlProfiler::Internal
