// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfprofilerruncontrol.h"

#include "perfdatareader.h"
#include "perfprofilertool.h"
#include "perfprofilertr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/qtcprocess.h>

#include <QAction>
#include <QMessageBox>
#include <QTcpServer>

using namespace ProjectExplorer;
using namespace Utils;

namespace PerfProfiler::Internal {

class PerfParserWorker final : public RunWorker
{
public:
    explicit PerfParserWorker(RunControl *runControl)
        : RunWorker(runControl)
    {
        setId("PerfParser");

        auto tool = PerfProfilerTool::instance();
        m_reader.setTraceManager(&traceManager());
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

    void start() final
    {
        CommandLine cmd{findPerfParser()};
        m_reader.addTargetArguments(&cmd, runControl());
        if (usesPerfChannel()) { // The channel is only used with qdb currently.
            const QUrl url = perfChannel();
            QTC_CHECK(url.isValid());
            cmd.addArgs({"--host", url.host(), "--port", QString::number(url.port())});
        }
        appendMessage("PerfParser args: " + cmd.arguments(), NormalMessageFormat);
        m_reader.createParser(cmd);
        m_reader.startParser();
    }

    void stop() final
    {
        m_reader.stopParser();
    }

    PerfDataReader *reader() { return &m_reader;}

private:
    PerfDataReader m_reader;
};

class LocalPerfRecordWorker final : public RunWorker
{
    Q_OBJECT

public:
    LocalPerfRecordWorker(RunControl *runControl)
        : RunWorker(runControl)
    {
        setId("LocalPerfRecordWorker");
    }

    void start() final
    {
        m_process = new Process(this);
        runControl()->setProperty("PerfProcess", QVariant::fromValue(m_process.get()));

        connect(m_process, &Process::started, this, &RunWorker::reportStarted);
        connect(m_process, &Process::done, this, [this] {
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
                appendMessage(m_process->cleanedStdErr(), StdErrFormat);
            reportStopped();
        });

        const Store perfArgs = runControl()->settingsData(PerfProfiler::Constants::PerfSettingsId);
        const QString recordArgs = perfArgs[Constants::PerfRecordArgsId].toString();

        CommandLine cmd({device()->filePath("perf"), {"record"}});
        cmd.addArgs(recordArgs, CommandLine::Raw);
        cmd.addArgs({"-o", "-", "--"});
        cmd.addCommandLineAsArgs(runControl()->commandLine(), CommandLine::Raw);

        m_process->setCommand(cmd);
        m_process->setWorkingDirectory(runControl()->workingDirectory());
        m_process->setEnvironment(runControl()->environment());
        appendMessage("Starting Perf: " + cmd.toUserOutput(), NormalMessageFormat);
        m_process->start();
    }

    void stop() final
    {
        if (m_process)
            m_process->terminate();
    }

private:
    QPointer<Process> m_process;
};

class PerfProfilerRunner final : public RunWorker
{
public:
    explicit PerfProfilerRunner(RunControl *runControl)
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

    void start() final
    {
        auto tool = PerfProfilerTool::instance();
        connect(tool->stopAction(), &QAction::triggered, runControl(), &RunControl::initiateStop);
        connect(runControl(), &RunControl::started, PerfProfilerTool::instance(),
                &PerfProfilerTool::onRunControlStarted);
        connect(runControl(), &RunControl::stopped, PerfProfilerTool::instance(),
                &PerfProfilerTool::onRunControlFinished);

        PerfDataReader *reader = m_perfParserWorker->reader();
        Process *perfProcess = runControl()->property("PerfProcess").value<Process *>();

        if (perfProcess) {
            connect(perfProcess, &Process::readyReadStandardError, this, [this, perfProcess] {
                appendMessage(QString::fromLocal8Bit(perfProcess->readAllRawStandardError()),
                              StdErrFormat);
            });
            connect(perfProcess, &Process::readyReadStandardOutput, this, [this, reader, perfProcess] {
                if (!reader->feedParser(perfProcess->readAllRawStandardOutput()))
                    reportFailure(Tr::tr("Failed to transfer Perf data to perfparser."));
            });
        }

        reportStarted();
    }

private:
    PerfParserWorker *m_perfParserWorker = nullptr;
    RunWorker *m_perfRecordWorker = nullptr;
};

// PerfProfilerRunWorkerFactory

class PerfProfilerRunWorkerFactory final : public RunWorkerFactory
{
public:
    PerfProfilerRunWorkerFactory()
    {
        setProduct<PerfProfilerRunner>();
        addSupportedRunMode(ProjectExplorer::Constants::PERFPROFILER_RUN_MODE);
    }
};

void setupPerfProfilerRunWorker()
{
    static PerfProfilerRunWorkerFactory thePerfProfilerRunWorkerFactory;
}

} // PerfProfiler::Internal

#include "perfprofilerruncontrol.moc"
