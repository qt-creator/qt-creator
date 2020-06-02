/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "perfdatareader.h"
#include "perfprofilerconstants.h"
#include "perfprofilerruncontrol.h"
#include "perfprofilertool.h"
#include "perfrunconfigurationaspect.h"
#include "perfsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/devicesupport/deviceprocess.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <ssh/sshconnection.h>
#include <utils/qtcprocess.h>

#include <QAction>
#include <QMessageBox>
#include <QTcpServer>

using namespace ProjectExplorer;
using namespace Utils;

namespace PerfProfiler {
namespace Internal {

class PerfParserWorker : public RunWorker
{
    Q_OBJECT

public:
    PerfParserWorker(RunControl *runControl)
        : RunWorker(runControl)
    {
        setId("PerfParser");

        auto tool = PerfProfilerTool::instance();
        m_reader.setTraceManager(tool->traceManager());
        m_reader.triggerRecordingStateChange(tool->isRecording());

        connect(tool, &PerfProfilerTool::recordingChanged,
                &m_reader, &PerfDataReader::triggerRecordingStateChange);

        connect(&m_reader, &PerfDataReader::updateTimestamps,
                tool, &PerfProfilerTool::updateTime);
        connect(&m_reader, &PerfDataReader::starting,
                tool, &PerfProfilerTool::startLoading);
        connect(&m_reader, &PerfDataReader::started, tool, &PerfProfilerTool::onReaderStarted);
        connect(&m_reader, &PerfDataReader::finishing, this, [tool] {
            // Temporarily disable buttons.
            tool->setToolActionsEnabled(false);
        });
        connect(&m_reader, &PerfDataReader::finished, tool, &PerfProfilerTool::onReaderFinished);

        connect(&m_reader, &PerfDataReader::processStarted, this, &RunWorker::reportStarted);
        connect(&m_reader, &PerfDataReader::processFinished, this, &RunWorker::reportStopped);
        connect(&m_reader, &PerfDataReader::processFailed, this, &RunWorker::reportFailure);
    }

    void start() override
    {
        QStringList args = m_reader.findTargetArguments(runControl());
        QUrl url = runControl()->property("PerfConnection").toUrl();
        if (url.isValid()) {
            args.append(QStringList{"--host", url.host(), "--port", QString::number(url.port())});
        }
        appendMessage("PerfParser args: " + args.join(' '), Utils::DebugFormat);
        m_reader.createParser(args);
        m_reader.startParser();
    }

    void stop() override
    {
        m_reader.stopParser();
    }

    PerfDataReader *reader() { return &m_reader;}

private:
    PerfDataReader m_reader;
};


class LocalPerfRecordWorker : public RunWorker
{
    Q_OBJECT

public:
    LocalPerfRecordWorker(RunControl *runControl)
        : RunWorker(runControl)
    {
        setId("LocalPerfRecordWorker");

        auto perfAspect = static_cast<PerfRunConfigurationAspect *>(runControl->aspect(Constants::PerfSettingsId));
        QTC_ASSERT(perfAspect, return);
        PerfSettings *settings = static_cast<PerfSettings *>(perfAspect->currentSettings());
        QTC_ASSERT(settings, return);
        m_perfRecordArguments = settings->perfRecordArguments();
    }

    void start() override
    {
        m_process = device()->createProcess(this);
        if (!m_process) {
            reportFailure(tr("Could not start device process."));
            return;
        }

        connect(m_process, &DeviceProcess::started, this, &RunWorker::reportStarted);
        connect(m_process, &DeviceProcess::finished, this, &RunWorker::reportStopped);
        connect(m_process, &DeviceProcess::error, [this](QProcess::ProcessError e) {
            // The terminate() below will frequently lead to QProcess::Crashed. We're not interested
            // in that. FailedToStart is the only actual failure.
            if (e == QProcess::FailedToStart) {
                QString msg = tr("Perf Process Failed to Start");
                QMessageBox::warning(Core::ICore::dialogParent(),
                                     msg, tr("Make sure that you are running a recent Linux kernel and "
                                             "that the \"perf\" utility is available."));
                reportFailure(msg);
            }
        });

        Runnable perfRunnable = runnable();

        QStringList arguments;
        arguments << "record";
        arguments += m_perfRecordArguments;
        arguments << "-o" << "-" << "--" << perfRunnable.executable.toString()
                  << Utils::QtcProcess::splitArgs(perfRunnable.commandLineArguments,
                                                  Utils::OsTypeLinux);

        perfRunnable.executable = FilePath::fromString("perf");
        perfRunnable.commandLineArguments = Utils::QtcProcess::joinArgs(arguments,
                                                                        Utils::OsTypeLinux);
        m_process->start(perfRunnable);
    }

    void stop() override
    {
        if (m_process)
            m_process->terminate();
    }

    DeviceProcess *recorder() { return m_process; }

private:
    QPointer<DeviceProcess> m_process;
    QStringList m_perfRecordArguments;
};


PerfProfilerRunner::PerfProfilerRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("PerfProfilerRunner");

    m_perfParserWorker = new PerfParserWorker(runControl);
    addStopDependency(m_perfParserWorker);

    // If the parser is gone, there is no point in going on.
    m_perfParserWorker->setEssential(true);

    if ((m_perfRecordWorker = runControl->createWorker("PerfRecorder"))) {
        m_perfParserWorker->addStartDependency(m_perfRecordWorker);
        addStartDependency(m_perfParserWorker);

    } else {
        m_perfRecordWorker = new LocalPerfRecordWorker(runControl);

        m_perfRecordWorker->addStartDependency(m_perfParserWorker);
        addStartDependency(m_perfRecordWorker);

        // In the local case, the parser won't automatically stop when the recorder does. So we need
        // to mark the recorder as essential, too.
        m_perfRecordWorker->setEssential(true);
    }

    m_perfParserWorker->addStopDependency(m_perfRecordWorker);
    PerfProfilerTool::instance()->onWorkerCreation(runControl);
}

void PerfProfilerRunner::start()
{
    auto tool = PerfProfilerTool::instance();
    connect(tool->stopAction(), &QAction::triggered, runControl(), &RunControl::initiateStop);
    connect(runControl(), &RunControl::started, PerfProfilerTool::instance(),
            &PerfProfilerTool::onRunControlStarted);
    connect(runControl(), &RunControl::stopped, PerfProfilerTool::instance(),
            &PerfProfilerTool::onRunControlFinished);
    connect(runControl(), &RunControl::finished, PerfProfilerTool::instance(),
            &PerfProfilerTool::onRunControlFinished);

    PerfDataReader *reader = m_perfParserWorker->reader();
    if (auto prw = qobject_cast<LocalPerfRecordWorker *>(m_perfRecordWorker)) {
        // That's the local case.
        DeviceProcess *recorder = prw->recorder();
        connect(recorder, &DeviceProcess::readyReadStandardError, this, [this, recorder] {
            appendMessage(QString::fromLocal8Bit(recorder->readAllStandardError()),
                          Utils::StdErrFormat);
        });
        connect(recorder, &DeviceProcess::readyReadStandardOutput, this, [this, reader, recorder] {
            if (!reader->feedParser(recorder->readAllStandardOutput()))
                reportFailure(tr("Failed to transfer Perf data to perfparser."));
        });
    }

    reportStarted();
}

} // namespace Internal
} // namespace PerfProfiler

#include "perfprofilerruncontrol.moc"
