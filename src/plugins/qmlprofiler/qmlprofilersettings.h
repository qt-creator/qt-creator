// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/runconfiguration.h>

#include <QPointer>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerSettings : public ProjectExplorer::ISettingsAspect
{
    Q_OBJECT

public:
    QmlProfilerSettings();

    void writeGlobalSettings() const;

    Utils::BoolAspect flushEnabled;
    Utils::IntegerAspect flushInterval;
    Utils::StringAspect lastTraceFile;
    Utils::BoolAspect aggregateTraces;
};

class QmlProfilerOptionsPage final : public Core::IOptionsPage
{
public:
    QmlProfilerOptionsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QPointer<QWidget> m_widget;
};

} // Internal
} // QmlProfiler
