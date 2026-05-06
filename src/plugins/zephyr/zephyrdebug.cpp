// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zephyrdebug.h"

#include "zephyrconstants.h"
#include "zephyrsettings.h"
#include "zephyrtr.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/qtcprocess.h>

#include <QtTaskTree/QBarrier>

#include <QRegularExpression>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Zephyr::Internal {

struct RunnerConfig
{
    FilePath elfFile;
    int gdbPort = 1234;
};

static RunnerConfig readRunnerConfig(const FilePath &buildDir)
{
    RunnerConfig result;
    result.elfFile = buildDir / "zephyr/zephyr.elf";

    const FilePath yamlFile = buildDir / "zephyr/runners.yaml";
    const QString yaml = QString::fromUtf8(yamlFile.fileContents().value_or(QByteArray{}));

    static const QRegularExpression debugRunnerRe(R"(^debug-runner:\s*(\S+))",
                                           QRegularExpression::MultilineOption);
    const QString runner = debugRunnerRe.match(yaml).captured(1);

    static const QRegularExpression elfRe(R"(^\s+elf_file:\s*(\S+))",
                                          QRegularExpression::MultilineOption);

    const QString elfName = elfRe.match(yaml).captured(1);
    if (!elfName.isEmpty())
        result.elfFile = buildDir / "zephyr" / elfName;

    if (runner == "qemu")
        result.gdbPort = 1234;
    else if (runner == "jlink" || runner == "nrfjprog" || runner == "nrfutil")
        result.gdbPort = 2331;
    else
        result.gdbPort = 3333; // openocd, pyocd, linkserver, ...

    return result;
}

static void killQemuIfRunning(const FilePath &buildDir)
{
    const FilePath pidFile = buildDir / "qemu.pid";
    if (!pidFile.isReadableFile())
        return;
    bool ok = false;
    const qint64 pid = pidFile.fileContents().value_or(QByteArray()).trimmed().toLongLong(&ok);
    if (ok && pid > 0) {
        Process kill;
        if (buildDir.osType() == OsTypeWindows) {
            kill.setCommand({buildDir.withNewPath("C:/Windows/System32/taskkill.exe"),
                             {"/PID", QString::number(pid), "/F"}});
        } else {
            kill.setCommand({buildDir.withNewPath("/bin/kill"), {QString::number(pid)}});
        }
        kill.runBlocking();
    }
    pidFile.removeFile();
}

class ZephyrDebugWorkerFactory final : public RunWorkerFactory
{
public:
    ZephyrDebugWorkerFactory()
    {
        setRecipeProducer([](RunControl *runControl) -> Group {
            const FilePath buildDir = runControl->buildDirectory();
            const RunnerConfig rc = readRunnerConfig(buildDir);

            DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
            rp.setStartMode(AttachToRemoteServer);
            rp.setCloseMode(KillAtClose);
            rp.setRemoteChannel("localhost:" + QString::number(rc.gdbPort));
            rp.setSymbolFile(rc.elfFile);
            rp.setUseContinueInsteadOfRun(true);
            rp.setSkipDebugServer(true);

            const auto modifier = [buildDir](Process &process) {
                CommandLine cmd{settings().westFilePath()};
                cmd.addArg("build");
                cmd.addArg("-d");
                cmd.addArg(buildDir.nativePath());
                cmd.addArg("-t");
                cmd.addArg("debugserver");
                process.setCommand(cmd);
                process.setWorkingDirectory(settings().workspaceDir());
            };

            return Group {
                onGroupSetup([buildDir] {
                    killQemuIfRunning(buildDir);
                    return SetupResult::Continue;
                }),
                When(runControl->processTaskWithModifier(modifier), &Process::started) >> Do {
                    debuggerRecipe(runControl, rp)
                }
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(Constants::ZEPHYR_RUN_CONFIG_ID);
    }
};

void setupZephyrDebug()
{
    static ZephyrDebugWorkerFactory theDebugWorkerFactory;
}

} // namespace Zephyr::Internal
