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

#include <QtTaskTree/QBarrier>

#include <utils/qtcprocess.h>

#include <QAction>
#include <QMessageBox>
#include <QTcpServer>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace PerfProfiler::Internal {

static Group perfParserRecipe(RunControl *runControl)
{
    const Storage<std::unique_ptr<PerfDataReader>> storage;

    const auto onSetup = [storage, runControl](QBarrier &barrier) {
        storage->reset(new PerfDataReader);
        PerfDataReader *reader = storage->get();
        auto tool = PerfProfilerTool::instance();

        reader->setTraceManager(&traceManager());
        reader->triggerRecordingStateChange(tool->isRecording());

        QObject::connect(tool, &PerfProfilerTool::recordingChanged,
                reader, &PerfDataReader::triggerRecordingStateChange);

        QObject::connect(reader, &PerfDataReader::updateTimestamps, tool, &PerfProfilerTool::updateTime);
        QObject::connect(reader, &PerfDataReader::starting, tool, &PerfProfilerTool::startLoading);
        QObject::connect(reader, &PerfDataReader::started, tool, &PerfProfilerTool::onReaderStarted);
        QObject::connect(reader, &PerfDataReader::finishing, tool, [tool] {
            // Temporarily disable buttons.
            tool->setToolActionsEnabled(false);
        });
        QObject::connect(reader, &PerfDataReader::finished, tool, &PerfProfilerTool::onReaderFinished);
        QObject::connect(reader, &PerfDataReader::processStarted, runControl, &RunControl::reportStarted);
        QObject::connect(reader, &PerfDataReader::processFinished, &barrier,
                         [barrier = &barrier, storagePtr = storage.activeStorage()] {
            storagePtr->release()->deleteLater();
            barrier->advance();
        });
        QObject::connect(reader, &PerfDataReader::processFailed, &barrier,
                         [barrier = &barrier, storagePtr = storage.activeStorage()] {
            storagePtr->release()->deleteLater();
            barrier->stopWithResult(DoneResult::Error);
        });

        QObject::connect(runControl, &RunControl::stdOutData, &barrier,
                         [reader, runControl, barrier = &barrier](const QByteArray &data) {
            if (reader->feedParser(data))
                return;
            runControl->postMessage(Tr::tr("Failed to transfer Perf data to perfparser."),
                                    ErrorMessageFormat);
            barrier->stopWithResult(DoneResult::Error);
        });

        QObject::connect(runControl, &RunControl::canceled, reader, &PerfDataReader::stopParser);

        CommandLine cmd{findPerfParser()};
        reader->addTargetArguments(&cmd, runControl);
        if (runControl->usesPerfChannel()) { // The channel is only used with qdb currently.
            const QUrl url = runControl->perfChannel();
            QTC_CHECK(url.isValid());
            cmd.addArgs({"--host", url.host(), "--port", QString::number(url.port())});
        }
        runControl->postMessage("PerfParser args: " + cmd.arguments(), NormalMessageFormat);
        reader->createParser(cmd);
        reader->startParser();
    };

    return {
        storage,
        QBarrierTask(onSetup)
    };
}

// Factories

class PerfRecordWorkerFactory final : public RunWorkerFactory
{
public:
    PerfRecordWorkerFactory()
    {
        setId("PerfRecordWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            const auto modifier = [runControl](Process &process) {
                const Store perfArgs = runControl->settingsData(PerfProfiler::Constants::PerfSettingsId);
                const QString recordArgs = perfArgs[Constants::PerfRecordArgsId].toString();

                CommandLine cmd({runControl->device()->filePath("perf"), {"record"}});
                cmd.addArgs(recordArgs, CommandLine::Raw);
                cmd.addArgs({"-o", "-", "--"});
                cmd.addCommandLineAsArgs(runControl->commandLine(), CommandLine::Raw);

                process.setCommand(cmd);
                process.setWorkingDirectory(runControl->workingDirectory());
                process.setEnvironment(runControl->environment());
                runControl->appendMessage("Starting Perf: " + cmd.toUserOutput(), NormalMessageFormat);
            };
            return runControl->processRecipe(modifier, {.suppressDefaultStdOutHandling = true});
        });

        addSupportedRunMode(ProjectExplorer::Constants::PERFPROFILER_RUNNER);
        addSupportForLocalRunConfigs();
        addSupportedRunConfig(RemoteLinux::Constants::RunConfigId);
        addSupportedRunConfig(RemoteLinux::Constants::CustomRunConfigId);
        setExecutionType(ProjectExplorer::Constants::STDPROCESS_EXECUTION_TYPE_ID);
    }
};

class PerfProfilerRunWorkerFactory final : public RunWorkerFactory
{
public:
    PerfProfilerRunWorkerFactory()
    {
        setId("PerfProfilerRunWorkerFactory");
        setRecipeProducer([](RunControl *runControl) {
            // The following RunWorkerFactories react to that:
            // 1. AppManagerPerfProfilerWorkerFactory
            // 2. PerfRecordWorkerFactory
            // 3. QdbPerfProfilerWorkerFactory

            PerfProfilerTool::instance()->onWorkerCreation(runControl);
            auto tool = PerfProfilerTool::instance();
            QObject::connect(tool->stopAction(), &QAction::triggered,
                             runControl, &RunControl::initiateStop);
            QObject::connect(runControl, &RunControl::started, PerfProfilerTool::instance(),
                             &PerfProfilerTool::onRunControlStarted);
            QObject::connect(runControl, &RunControl::stopped, PerfProfilerTool::instance(),
                             &PerfProfilerTool::onRunControlFinished);

            return Group {
                parallel,
                Group {
                    runControl->createRecipe(ProjectExplorer::Constants::PERFPROFILER_RUNNER),
                    onGroupDone([runControl] { runControl->initiateStop(); })
                },
                perfParserRecipe(runControl)
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::PERFPROFILER_RUN_MODE);
        addSupportForLocalRunConfigs();
        addSupportedRunConfig(RemoteLinux::Constants::RunConfigId);
        addSupportedRunConfig(RemoteLinux::Constants::CustomRunConfigId);
        setExecutionType(ProjectExplorer::Constants::STDPROCESS_EXECUTION_TYPE_ID);
    }
};

void setupPerfProfilerRunWorker()
{
    static PerfProfilerRunWorkerFactory thePerfProfilerRunWorkerFactory;
    static PerfRecordWorkerFactory thePerfRecordWorkerFactory;
}

} // PerfProfiler::Internal
