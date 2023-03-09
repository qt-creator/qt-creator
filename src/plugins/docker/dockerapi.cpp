// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerapi.h"

#include "dockertr.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/asynctask.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QLoggingCategory>

#include <thread>

Q_LOGGING_CATEGORY(dockerApiLog, "qtc.docker.api", QtWarningMsg);

using namespace Utils;

namespace Docker::Internal {

DockerApi *s_instance{nullptr};

DockerApi::DockerApi(DockerSettings *settings)
    : m_settings(settings)
{
    s_instance = this;
}

DockerApi *DockerApi::instance()
{
    return s_instance;
}

bool DockerApi::canConnect()
{
    QtcProcess process;
    FilePath dockerExe = dockerClient();
    if (dockerExe.isEmpty() || !dockerExe.isExecutableFile())
        return false;

    bool result = false;

    process.setCommand(CommandLine(dockerExe, QStringList{"info"}));
    connect(&process, &QtcProcess::done, [&process, &result] {
        qCInfo(dockerApiLog) << "'docker info' result:\n" << qPrintable(process.allOutput());
        if (process.result() == ProcessResult::FinishedWithSuccess)
            result = true;
    });

    process.start();
    process.waitForFinished();

    return result;
}

void DockerApi::checkCanConnect(bool async)
{
    if (async) {
        std::unique_lock lk(m_daemonCheckGuard, std::try_to_lock);
        if (!lk.owns_lock())
            return;

        m_dockerDaemonAvailable = std::nullopt;
        emit dockerDaemonAvailableChanged();

        auto future = Utils::asyncRun([lk = std::move(lk), this] {
            m_dockerDaemonAvailable = canConnect();
            emit dockerDaemonAvailableChanged();
        });

        Core::ProgressManager::addTask(future, Tr::tr("Checking docker daemon"), "DockerPlugin");
        return;
    }

    std::unique_lock lk(m_daemonCheckGuard);
    bool isAvailable = canConnect();
    if (!m_dockerDaemonAvailable.has_value() || isAvailable != m_dockerDaemonAvailable) {
        m_dockerDaemonAvailable = isAvailable;
        emit dockerDaemonAvailableChanged();
    }
}

void DockerApi::recheckDockerDaemon()
{
    QTC_ASSERT(s_instance, return );
    s_instance->checkCanConnect();
}

std::optional<bool> DockerApi::dockerDaemonAvailable(bool async)
{
    if (!m_dockerDaemonAvailable.has_value())
        checkCanConnect(async);
    return m_dockerDaemonAvailable;
}

std::optional<bool> DockerApi::isDockerDaemonAvailable(bool async)
{
    QTC_ASSERT(s_instance, return std::nullopt);
    return s_instance->dockerDaemonAvailable(async);
}

FilePath DockerApi::dockerClient()
{
    return FilePath::fromString(m_settings->dockerBinaryPath.value());
}

} // Docker::Internal
