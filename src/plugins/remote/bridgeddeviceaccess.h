// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/devicesupport/idevice.h>

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/result.h>
#include <utils/synchronizedvalue.h>

#include <QDeadlineTimer>
#include <QFuture>
#include <QList>
#include <QObject>

#include <functional>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE
class QLoggingCategory;
QT_END_NAMESPACE

namespace CmdBridge { class FileAccess; }

namespace Utils { class WrappedProcessInterface; }

namespace Remote::Internal {

class BridgedDeviceAccess final : public QObject
{
public:
    struct Hooks
    {
        std::function<Utils::FilePath()> rootPath;
        // The environment the bridge is started with and answers
        // deviceEnvironment() with.
        std::function<Utils::Result<Utils::Environment>()> bridgeEnvironment;
        // A bridge the device can run where it is, tried before deploying one.
        std::function<Utils::Result<Utils::FilePath>()> bridgeInPlace;
        std::function<std::unique_ptr<CmdBridge::FileAccess>(
            const std::function<void()> &errorExitHandler)>
            createBridge;
        std::function<Utils::DeviceFileAccessPtr()> createFallback;
        // Forgets what was detected about the device. Called by drop() only.
        std::function<void()> resetCaches;
        std::function<void()> dropped;
    };

    using LogCategory = const QLoggingCategory &(*)();

    BridgedDeviceAccess(LogCategory log, const Hooks &hooks);
    ~BridgedDeviceAccess() final;

    Utils::DeviceFileAccessPtr cachedFileAccess() const;
    Utils::DeviceFileAccessPtr fileAccess();

    void drop();
    void dropStale() const;

private:
    using FileAccessLock = Utils::SynchronizedValue<Utils::DeviceFileAccessPtr>::unique_lock;

    Utils::Result<Utils::DeviceFileAccessPtr> createBridgeFileAccess(
        FileAccessLock &fileAccess,
        const std::optional<Utils::Result<Utils::FilePath>> &bridgeInPlace,
        const Utils::Environment &bridgeEnvironment);
    Utils::DeviceFileAccessPtr bringUp(int generation);
    Utils::DeviceFileAccessPtr fallBack(FileAccessLock &fileAccess, const QString &reason);
    bool shouldRetryBridge() const { return m_retryBridge && m_bridgeRetryTimer.hasExpired(); }
    std::function<void()> bridgeExitHandler();

    const LogCategory m_log;
    const Hooks m_hooks;

    mutable Utils::SynchronizedValue<Utils::DeviceFileAccessPtr> m_fileAccess;
    QList<QFuture<void>> m_bringUps;

    // Guarded by m_fileAccess.
    bool m_retryBridge = false;
    int m_bridgeAttempts = 0;
    QDeadlineTimer m_bridgeRetryTimer;
    bool m_creatingFileAccess = false;
    mutable int m_generation = 0;
};

// Runs a command line in the shell on the device, without the bridge.
class BridgeFallbackFileAccess : public Utils::UnixDeviceFileAccess
{
public:
    explicit BridgeFallbackFileAccess(const Utils::FilePath &rootPath)
        : m_rootPath(rootPath)
    {}

    Utils::Result<Utils::RunResult> runInShellImpl(
        const Utils::CommandLine &cmdLine, const QByteArray &stdInData) const override;

private:
    const Utils::FilePath m_rootPath;
};

using DeviceCommandWrapper = std::function<Utils::Result<Utils::CommandLine>(
    const QString &markerTemplate,
    const Utils::CommandLine &cmd,
    const std::optional<Utils::Environment> &env,
    const std::optional<Utils::FilePath> &workDir,
    bool withMarker)>;

using OpenTerminal = std::function<void(
    const Utils::Environment &, const Utils::FilePath &, const Utils::Continuation<> &)>;

Utils::WrappedProcessInterface *createBridgedProcessInterface(
    const ProjectExplorer::IDevice::ConstPtr &device, const DeviceCommandWrapper &wrapCommand);

OpenTerminal openTerminalWithDefaultShell(
    const std::function<Utils::FilePath()> &rootPath);

QtTaskTree::ExecutableItem bridgedSignalOperationRecipe(
    const Utils::FilePath &rootPath,
    const ProjectExplorer::SignalOperationData &data,
    const QtTaskTree::Storage<Utils::Result<>> &resultStorage);

} // namespace Remote::Internal
