// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "perfprofiler_global.h"

#include <projectexplorer/runconfiguration.h>

#include <QObject>

namespace PerfProfiler {

class PERFPROFILER_EXPORT PerfSettings final : public ProjectExplorer::ISettingsAspect
{
    Q_OBJECT
    Q_PROPERTY(QStringList perfRecordArguments READ perfRecordArguments NOTIFY changed)

public:
    explicit PerfSettings(ProjectExplorer::Target *target = nullptr);
    ~PerfSettings() final;

    void readGlobalSettings();
    void writeGlobalSettings() const;

    QStringList perfRecordArguments() const;

    void resetToDefault();

    Utils::IntegerAspect period;
    Utils::IntegerAspect stackSize;
    Utils::SelectionAspect sampleMode;
    Utils::SelectionAspect callgraphMode;
    Utils::StringListAspect events;
    Utils::StringAspect extraArguments;

signals:
    void changed();
};

} // namespace PerfProfiler
