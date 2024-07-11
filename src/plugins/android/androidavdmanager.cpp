// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidavdmanager.h"
#include "androidconfigurations.h"
#include "androidtr.h"

#include <coreplugin/icore.h>

#include <solutions/tasking/conditional.h>
#include <solutions/tasking/tcpsocket.h>

#include <utils/async.h>
#include <utils/qtcprocess.h>

#include <QLoggingCategory>
#include <QMessageBox>

using namespace Tasking;
using namespace Utils;
using namespace std::chrono_literals;

namespace Android::Internal::AndroidAvdManager {

static Q_LOGGING_CATEGORY(avdManagerLog, "qtc.android.avdManager", QtWarningMsg)

static void startAvdDetached(QPromise<void> &promise, const CommandLine &avdCommand)
{
    qCDebug(avdManagerLog).noquote() << "Running command (startAvdDetached):" << avdCommand.toUserOutput();
    if (!Process::startDetached(avdCommand, {}, DetachedChannelMode::Discard))
        promise.future().cancel();
}

static CommandLine avdCommand(const QString &avdName, bool is32BitUserSpace)
{
    CommandLine cmd(AndroidConfig::emulatorToolPath());
    if (is32BitUserSpace)
        cmd.addArg("-force-32bit");
    cmd.addArgs(AndroidConfig::emulatorArgs(), CommandLine::Raw);
    cmd.addArgs({"-avd", avdName});
    return cmd;
}

static ExecutableItem startAvdAsyncRecipe(const QString &avdName)
{
    const Storage<bool> is32Storage;

    const auto onSetup = [] {
        const FilePath emulatorPath = AndroidConfig::emulatorToolPath();
        if (emulatorPath.exists())
            return SetupResult::Continue;

        QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("Emulator Tool Is Missing"),
                              Tr::tr("Install the missing emulator tool (%1) to the "
                                     "installed Android SDK.").arg(emulatorPath.displayName()));
        return SetupResult::StopWithError;
    };

    const auto onGetConfSetup = [](Process &process) {
        if (!HostOsInfo::isLinuxHost() || QSysInfo::WordSize != 32)
            return SetupResult::StopWithSuccess; // is64

        process.setCommand({"getconf", {"LONG_BIT"}});
        return SetupResult::Continue;
    };
    const auto onGetConfDone = [is32Storage](const Process &process, DoneWith result) {
        if (result == DoneWith::Success)
            *is32Storage = process.allOutput().trimmed() == "32";
        else
            *is32Storage = true;
        return true;
    };

    const auto onAvdSetup = [avdName, is32Storage](Async<void> &async) {
        async.setConcurrentCallData(startAvdDetached, avdCommand(avdName, *is32Storage));
    };
    const auto onAvdDone = [avdName] {
        QMessageBox::critical(Core::ICore::dialogParent(), Tr::tr("AVD Start Error"),
                              Tr::tr("Failed to start AVD emulator for \"%1\" device.").arg(avdName));
    };

    return Group {
        is32Storage,
        onGroupSetup(onSetup),
        ProcessTask(onGetConfSetup, onGetConfDone),
        AsyncTask<void>(onAvdSetup, onAvdDone, CallDoneIf::Error)
    };
}

static ExecutableItem serialNumberRecipe(const QString &avdName, const Storage<QString> &serialNumberStorage)
{
    const Storage<QStringList> outputStorage;
    const Storage<QString> currentSerialNumberStorage;
    const LoopUntil iterator([outputStorage](int iteration) { return iteration < outputStorage->size(); });

    const auto onSocketSetup = [iterator, outputStorage, currentSerialNumberStorage](TcpSocket &socket) {
        const QString line = outputStorage->at(iterator.iteration());
        if (line.startsWith("* daemon"))
            return SetupResult::StopWithError;

        const QString serialNumber = line.left(line.indexOf('\t')).trimmed();
        if (!serialNumber.startsWith("emulator"))
            return SetupResult::StopWithError;

        const int index = serialNumber.indexOf(QLatin1String("-"));
        if (index == -1)
            return SetupResult::StopWithError;

        bool ok;
        const int port = serialNumber.mid(index + 1).toInt(&ok);
        if (!ok)
            return SetupResult::StopWithError;

        *currentSerialNumberStorage = serialNumber;

        socket.setAddress(QHostAddress(QHostAddress::LocalHost));
        socket.setPort(port);
        socket.setWriteData("avd name\nexit\n");
        return SetupResult::Continue;
    };
    const auto onSocketDone = [avdName, currentSerialNumberStorage, serialNumberStorage](const TcpSocket &socket) {
        const QByteArrayList response = socket.socket()->readAll().split('\n');
        // The input "avd name" might not be echoed as-is, but contain ASCII control sequences.
        for (int i = response.size() - 1; i > 1; --i) {
            if (!response.at(i).startsWith("OK"))
                continue;

            const QString currentAvdName = QString::fromLatin1(response.at(i - 1)).trimmed();
            if (avdName != currentAvdName)
                break;

            *serialNumberStorage = *currentSerialNumberStorage;
            return DoneResult::Success;
        }
        return DoneResult::Error;
    };

    return Group {
        outputStorage,
        AndroidConfig::devicesCommandOutputRecipe(outputStorage),
        For {
            iterator,
            parallel,
            stopOnSuccess,
            Group {
                currentSerialNumberStorage,
                TcpSocketTask(onSocketSetup, onSocketDone)
            }
        }
    };
}

static ExecutableItem isAvdBootedRecipe(const Storage<QString> &serialNumberStorage)
{
    const auto onSetup = [serialNumberStorage](Process &process) {
        const CommandLine cmd{AndroidConfig::adbToolPath(),
                              {AndroidDeviceInfo::adbSelector(*serialNumberStorage),
                               "shell", "getprop", "init.svc.bootanim"}};
        qCDebug(avdManagerLog).noquote() << "Running command (isAvdBooted):" << cmd.toUserOutput();
        process.setCommand(cmd);
    };
    const auto onDone = [](const Process &process, DoneWith result) {
        return result == DoneWith::Success && process.allOutput().trimmed() == "stopped";
    };
    return ProcessTask(onSetup, onDone);
}

static ExecutableItem waitForAvdRecipe(const QString &avdName, const Storage<QString> &serialNumberStorage)
{
    const Storage<QStringList> outputStorage;
    const Storage<bool> stopStorage;

    const auto onIsConnectedDone = [stopStorage, outputStorage, serialNumberStorage] {
        const QString serialNumber = *serialNumberStorage;
        for (const QString &line : *outputStorage) {
            // skip the daemon logs
            if (!line.startsWith("* daemon") && line.left(line.indexOf('\t')).trimmed() == serialNumber)
                return DoneResult::Error;
        }
        serialNumberStorage->clear();
        *stopStorage = true;
        return DoneResult::Success;
    };

    const auto onWaitForBootedDone = [stopStorage] { return !*stopStorage; };

    return Group {
        Forever {
            stopOnSuccess,
            serialNumberRecipe(avdName, serialNumberStorage),
            TimeoutTask([](std::chrono::milliseconds &timeout) { timeout = 1s; }, DoneResult::Error)
        }.withTimeout(120s),
        Forever {
            stopStorage,
            stopOnSuccess,
            isAvdBootedRecipe(serialNumberStorage),
            TimeoutTask([](std::chrono::milliseconds &timeout) { timeout = 1s; }, DoneResult::Error),
            Group {
                outputStorage,
                AndroidConfig::devicesCommandOutputRecipe(outputStorage),
                onGroupDone(onIsConnectedDone, CallDoneIf::Success)
            },
            onGroupDone(onWaitForBootedDone)
        }.withTimeout(120s)
    };
}

ExecutableItem startAvdRecipe(const QString &avdName, const Storage<QString> &serialNumberStorage)
{
    return Group {
        If (serialNumberRecipe(avdName, serialNumberStorage) || startAvdAsyncRecipe(avdName)) >> Then {
            waitForAvdRecipe(avdName, serialNumberStorage)
        } >> Else {
            errorItem
        }
    };
}

} // namespace Android::Internal::AndroidAvdManager
