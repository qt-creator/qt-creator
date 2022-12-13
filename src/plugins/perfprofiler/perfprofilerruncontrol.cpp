// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "perfprofilerruncontrol.h"

#include "perfdatareader.h"
#include "perfprofilertool.h"
#include "perfrunconfigurationaspect.h"
#include "perfsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
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
        CommandLine cmd{findPerfParser()};
        m_reader.addTargetArguments(&cmd, runControl());
        QUrl url = runControl()->property("PerfConnection").toUrl();
        if (url.isValid()) {
            cmd.addArgs({"--host", url.host(), "--port", QString::number(url.port())});
        }
        appendMessage("PerfParser args: " + cmd.arguments(), NormalMessageFormat);
        m_reader.createParser(cmd);
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

        auto perfAspect = runControl->aspect<PerfRunConfigurationAspect>();
        QTC_ASSERT(perfAspect, return);
        PerfSettings *settings = static_cast<PerfSettings *>(perfAspect->currentSettings);
        QTC_ASSERT(settings, return);
        m_perfRecordArguments = settings->perfRecordArguments();
    }

    void start() override
    {
        m_process = new QtcProcess(this);

        connect(m_process, &QtcProcess::started, this, &RunWorker::reportStarted);
        connect(m_process, &QtcProcess::done, this, [this] {
            // The terminate() below will frequently lead to QProcess::Crashed. We're not interested
            // in that. FailedToStart is the only actual failure.
            if (m_process->error() == QProcess::FailedToStart) {
                const QString msg = Tr::tr("Perf Process Failed to Start");
                QMessageBox::warning(Core::ICore::dialogParent(), msg,
                                     Tr::tr("Make sure that you are running a recent Linux kernel "
                                            "and that the \"perf\" utility is available."));
                reportFailure(msg);
                return;
            }
            if (!m_process->cleanedStdErr().isEmpty())
                appendMessage(m_process->cleanedStdErr(), Utils::StdErrFormat);
            reportStopped();
        });

        CommandLine cmd({device()->filePath("perf"), {"record"}});
        cmd.addArgs(m_perfRecordArguments);
        cmd.addArgs({"-o", "-", "--"});
        cmd.addCommandLineAsArgs(runControl()->commandLine(), CommandLine::Raw);

        m_process->setCommand(cmd);
        m_process->setWorkingDirectory(runControl()->workingDirectory());
        appendMessage("Starting Perf: " + cmd.toUserOutput(), Utils::NormalMessageFormat);
        m_process->start();
    }

    void stop() override
    {
        if (m_process)
            m_process->terminate();
    }

    QtcProcess *recorder() { return m_process; }

private:
    QPointer<QtcProcess> m_process;
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
        QtcProcess *recorder = prw->recorder();
        connect(recorder, &QtcProcess::readyReadStandardError, this, [this, recorder] {
            appendMessage(QString::fromLocal8Bit(recorder->readAllStandardError()),
                          Utils::StdErrFormat);
        });
        connect(recorder, &QtcProcess::readyReadStandardOutput, this, [this, reader, recorder] {
            if (!reader->feedParser(recorder->readAllStandardOutput()))
                reportFailure(Tr::tr("Failed to transfer Perf data to perfparser."));
        });
    }

    reportStarted();
}

} // namespace Internal
} // namespace PerfProfiler

#include "perfprofilerruncontrol.moc"
