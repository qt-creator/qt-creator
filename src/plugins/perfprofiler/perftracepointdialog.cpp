// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilertr.h"
#include "perftracepointdialog.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <utils/layoutbuilder.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>

const char ELEVATE_METHOD_NA[] = "n.a";
const char ELEVATE_METHOD_PKEXEC[] = "pkexec";
const char ELEVATE_METHOD_SUDO[] = "sudo";

using namespace ProjectExplorer;
using namespace Utils;

namespace PerfProfiler {
namespace Internal {

PerfTracePointDialog::PerfTracePointDialog()
{
    resize(400, 300);
    m_label = new QLabel(Tr::tr("Run the following script as root to create trace points?"));
    m_textEdit = new QTextEdit;
    m_privilegesChooser = new QComboBox;
    m_privilegesChooser->addItems({ELEVATE_METHOD_NA, ELEVATE_METHOD_PKEXEC, ELEVATE_METHOD_SUDO});
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    using namespace Layouting;
    Column {
        m_label,
        m_textEdit,
        Form {
            Tr::tr("Elevate privileges using:"), m_privilegesChooser, br,
        },
        m_buttonBox,
    }.attachTo(this);

    if (const Target *target = ProjectManager::startupTarget()) {
        const Kit *kit = target->kit();
        QTC_ASSERT(kit, return);

        m_device = DeviceKitAspect::device(kit);
        if (!m_device) {
            m_textEdit->setPlainText(Tr::tr("Error: No device available for active target."));
            return;
        }
    }

    if (!m_device) {
        // There should at least be a desktop device.
        m_device = DeviceManager::defaultDesktopDevice();
        QTC_ASSERT(m_device, return);
    }

    QFile file(":/perfprofiler/tracepoints.sh");
    if (file.open(QIODevice::ReadOnly)) {
        m_textEdit->setPlainText(QString::fromUtf8(file.readAll()));
    } else {
        m_textEdit->setPlainText(Tr::tr("Error: Failed to load trace point script %1: %2.")
                                 .arg(file.fileName()).arg(file.errorString()));
    }

    m_privilegesChooser->setCurrentText(
                QLatin1String(m_device->type() == Constants::DESKTOP_DEVICE_TYPE
                              ? ELEVATE_METHOD_PKEXEC : ELEVATE_METHOD_NA));

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &PerfTracePointDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &PerfTracePointDialog::reject);
}

PerfTracePointDialog::~PerfTracePointDialog() = default;

void PerfTracePointDialog::runScript()
{
    m_label->setText(Tr::tr("Executing script..."));
    m_textEdit->setReadOnly(true);
    m_privilegesChooser->setEnabled(false);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    m_process.reset(new Process(this));
    m_process->setWriteData(m_textEdit->toPlainText().toUtf8());
    m_textEdit->clear();

    const QString elevate = m_privilegesChooser->currentText();
    if (elevate != QLatin1String(ELEVATE_METHOD_NA))
        m_process->setCommand({m_device->filePath(elevate), {"sh"}});
    else
        m_process->setCommand({m_device->filePath("sh"), {}});

    connect(m_process.get(), &Process::done, this, &PerfTracePointDialog::handleProcessDone);
    m_process->start();
}

void PerfTracePointDialog::handleProcessDone()
{
    const QProcess::ProcessError error = m_process->error();
    QString message;
    if (error == QProcess::FailedToStart) {
        message = Tr::tr("Failed to run trace point script: %1").arg(error);
    } else if ((m_process->exitStatus() == QProcess::CrashExit) || (m_process->exitCode() != 0)) {
        message = Tr::tr("Failed to create trace points.");
    } else {
        message = Tr::tr("Created trace points for: %1").arg(
            m_process->readAllStandardOutput().trimmed().replace('\n', ", "));
    }
    m_label->setText(message);
    m_textEdit->setHtml(m_process->readAllStandardError());
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    m_buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
}

void PerfTracePointDialog::accept()
{
    if (m_process) {
        QTC_CHECK(m_process->state() == QProcess::NotRunning);
        QDialog::accept();
    } else {
        runScript();
    }
}

void PerfTracePointDialog::reject()
{
    if (m_process)
        m_process->kill();
    else
        QDialog::reject();
}

} // namespace Internal
} // namespace PerfProfiler
