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

#include <remotelinux/remotelinux_constants.h>

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
        if (runControl()->usesPerfChannel()) { // The channel is only used with qdb currently.
            const QUrl url = runControl()->perfChannel();
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

// Factories

class PerfRecordWorkerFactory final : public RunWorkerFactory
{
public:
    PerfRecordWorkerFactory()
    {
        setProducer([](RunControl *runControl) {
            auto runner = new ProcessRunner(runControl);
            runner->setStartModifier([runner, runControl] {
                const Store perfArgs = runControl->settingsData(PerfProfiler::Constants::PerfSettingsId);
                const QString recordArgs = perfArgs[Constants::PerfRecordArgsId].toString();

                CommandLine cmd({runControl->device()->filePath("perf"), {"record"}});
                cmd.addArgs(recordArgs, CommandLine::Raw);
                cmd.addArgs({"-o", "-", "--"});
                cmd.addCommandLineAsArgs(runControl->commandLine(), CommandLine::Raw);

                runner->setCommandLine(cmd);
                runner->setWorkingDirectory(runControl->workingDirectory());
                runner->setEnvironment(runControl->environment());
                runControl->appendMessage("Starting Perf: " + cmd.toUserOutput(), NormalMessageFormat);
            });

            // In the local case, the parser won't automatically stop when the recorder does. So we need
            // to mark the recorder as essential, too.
            runner->setEssential(true);
            return runner;
        });

        addSupportedRunMode("PerfRecorder");
        addSupportForLocalRunConfigs();
        addSupportedDeviceType(RemoteLinux::Constants::GenericLinuxOsType);
        addSupportedDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
        addSupportedDeviceType("DockerDeviceType");
    }
};

class PerfProfilerRunWorkerFactory final : public RunWorkerFactory
{
public:
    PerfProfilerRunWorkerFactory()
    {
        setProducer([](RunControl *runControl) -> RunWorker * {
            PerfParserWorker *perfParserWorker = new PerfParserWorker(runControl);

            // There are currently two RunWorkerFactories reacting to that:
            // PerfRecordRunnerFactory above for the generic case and
            // QdbPerfProfilerWorkerFactory in boot2qt.
            ProcessRunner *perfRecordWorker
                = qobject_cast<ProcessRunner *>(runControl->createWorker("PerfRecorder"));

            QTC_ASSERT(perfRecordWorker, return nullptr);

            perfRecordWorker->suppressDefaultStdOutHandling();

            perfParserWorker->addStartDependency(perfRecordWorker);
            perfParserWorker->addStopDependency(perfRecordWorker);
            PerfProfilerTool::instance()->onWorkerCreation(runControl);

            auto tool = PerfProfilerTool::instance();
            QObject::connect(tool->stopAction(), &QAction::triggered,
                             runControl, &RunControl::initiateStop);
            QObject::connect(runControl, &RunControl::started, PerfProfilerTool::instance(),
                             &PerfProfilerTool::onRunControlStarted);
            QObject::connect(runControl, &RunControl::stopped, PerfProfilerTool::instance(),
                             &PerfProfilerTool::onRunControlFinished);

            PerfDataReader *reader = perfParserWorker->reader();
            QObject::connect(perfRecordWorker, &ProcessRunner::stdOutData,
                             perfParserWorker, [perfParserWorker, reader](const QByteArray &data) {
                if (reader->feedParser(data))
                    return;

                perfParserWorker->appendMessage(Tr::tr("Failed to transfer Perf data to perfparser."),
                                                ErrorMessageFormat);
                perfParserWorker->initiateStop();
            });
            return perfParserWorker;
        });
        addSupportedRunMode(ProjectExplorer::Constants::PERFPROFILER_RUN_MODE);
    }
};

void setupPerfProfilerRunWorker()
{
    static PerfProfilerRunWorkerFactory thePerfProfilerRunWorkerFactory;
    static PerfRecordWorkerFactory thePerfRecordWorkerFactory;
}

} // PerfProfiler::Internal
