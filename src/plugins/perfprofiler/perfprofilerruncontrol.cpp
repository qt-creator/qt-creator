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

        if ((m_perfRecordWorker = qobject_cast<ProcessRunner *>(runControl->createWorker("PerfRecorder")))) {
            m_perfParserWorker->addStartDependency(m_perfRecordWorker);
        } else {
            m_perfRecordWorker = new ProcessRunner(runControl);
            m_perfRecordWorker->suppressDefaultStdOutHandling();

            m_perfRecordWorker->setStartModifier([this, runControl] {
                const Store perfArgs = runControl->settingsData(PerfProfiler::Constants::PerfSettingsId);
                const QString recordArgs = perfArgs[Constants::PerfRecordArgsId].toString();

                CommandLine cmd({device()->filePath("perf"), {"record"}});
                cmd.addArgs(recordArgs, CommandLine::Raw);
                cmd.addArgs({"-o", "-", "--"});
                cmd.addCommandLineAsArgs(runControl->commandLine(), CommandLine::Raw);

                m_perfRecordWorker->setCommandLine(cmd);
                m_perfRecordWorker->setWorkingDirectory(runControl->workingDirectory());
                m_perfRecordWorker->setEnvironment(runControl->environment());
                appendMessage("Starting Perf: " + cmd.toUserOutput(), NormalMessageFormat);
            });

            m_perfRecordWorker->addStartDependency(m_perfParserWorker);

            // In the local case, the parser won't automatically stop when the recorder does. So we need
            // to mark the recorder as essential, too.
            m_perfRecordWorker->setEssential(true);
        }
        addStartDependency(m_perfRecordWorker);

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
        connect(m_perfRecordWorker, &ProcessRunner::stdOutData,
                this, [this, reader](const QByteArray &data) {
            if (!reader->feedParser(data))
                reportFailure(Tr::tr("Failed to transfer Perf data to perfparser."));
        });

        reportStarted();
    }

private:
    PerfParserWorker *m_perfParserWorker = nullptr;
    ProcessRunner *m_perfRecordWorker = nullptr;
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
