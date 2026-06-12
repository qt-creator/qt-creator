// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"

#include <utils/filepath.h>

#include <QObject>
#include <QWidgetList>

#include <chrono>

namespace QmlProfiler::Internal {

// Hosts the Instruments-like views for traces recorded by the macOS profile
// recorder: a CPU usage timeline and the merged call-stack tree. Counterpart
// to the plain view managers in the qmlprofiler plugin, but specific to the
// standalone viewer's sampler trace format.
class QMLPROFILER_EXPORT SamplerViewManager : public QObject
{
    Q_OBJECT

public:
    explicit SamplerViewManager(QObject *parent = nullptr);
    ~SamplerViewManager() override;

    // True if `dir` contains a sampler trace (cheap: reads only the metadata).
    static bool isSamplerTrace(const Utils::FilePath &dir);

    QWidgetList views(QWidget *parent);
    QWidget *rangeDetailsWidget() const;
    void load(const Utils::FilePath &dir);
    void clear();
    std::chrono::milliseconds traceDuration() const;

signals:
    void error(const QString &error);
    void loadFinished();

private:
    class SamplerViewManagerPrivate *d;
};

} // namespace QmlProfiler::Internal
