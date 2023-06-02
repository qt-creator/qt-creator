// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevicefwd.h>

#include <QDialog>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QTextEdit;
QT_END_NAMESPACE

namespace Utils { class Process; }

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

    QLabel *m_label;
    QTextEdit *m_textEdit;
    QComboBox *m_privilegesChooser;
    QDialogButtonBox *m_buttonBox;
    ProjectExplorer::IDeviceConstPtr m_device;
    std::unique_ptr<Utils::Process> m_process;

    void accept() final;
    void reject() final;
};

} // namespace Internal
} // namespace PerfProfiler
