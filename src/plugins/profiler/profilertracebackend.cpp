// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilertracebackend.h"

#include "profilertr.h"

#include <tracing/timelineformatdata.h>

#include <QLabel>

using namespace std::chrono;

namespace Profiler::Internal {

ProfilerTraceBackend::ProfilerTraceBackend(QObject *parent)
    : QObject(parent)
{}

ProfilerTraceBackend::~ProfilerTraceBackend() = default;

Utils::Result<> ProfilerTraceBackend::save(const Utils::FilePath &path)
{
    Q_UNUSED(path)
    return Utils::ResultError(Tr::tr("This trace format cannot be saved."));
}

// Reserve the width of the widest expected text, so that the toolbar does not
// re-layout each time the text changes.
void ProfilerTraceBackend::setTimeLabelText(QLabel &label, const QString &format,
                                            nanoseconds time)
{
    const QString widest = format.arg("99m 59.9s");
    label.setMinimumWidth(label.fontMetrics().horizontalAdvance(widest) + label.indent());
    label.setText(format.arg(time > 0ns ? Timeline::formatTime(time) : QString("0 s")));
}

} // namespace Profiler::Internal
