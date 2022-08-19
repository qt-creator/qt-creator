// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>

#include <QDialog>
#include <QProcess>

namespace Utils { class QtcProcess; }

namespace PerfProfiler {
namespace Internal {

namespace Ui { class PerfTracePointDialog; }

class PerfTracePointDialog : public QDialog
{
    Q_OBJECT

public:
    PerfTracePointDialog();
    ~PerfTracePointDialog();

private:
    void runScript();
    void handleProcessDone();
    void finish();

    Ui::PerfTracePointDialog *m_ui;
    ProjectExplorer::IDeviceConstPtr m_device;
    std::unique_ptr<Utils::QtcProcess> m_process;

    void accept() final;
    void reject() final;
};

} // namespace Internal
} // namespace PerfProfiler
