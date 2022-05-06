/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "dockerapi.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>

#include <QLoggingCategory>

#include <thread>

Q_LOGGING_CATEGORY(dockerApiLog, "qtc.docker.api", QtDebugMsg);

namespace Docker {
namespace Internal {

using namespace Utils;

DockerApi *s_instance{nullptr};

DockerApi::DockerApi()
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
    FilePath dockerExe = findDockerClient();
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

        m_dockerDaemonAvailable = nullopt;
        dockerDaemonAvailableChanged();

        auto future = Utils::runAsync([lk = std::move(lk), this] {
            m_dockerDaemonAvailable = canConnect();
            dockerDaemonAvailableChanged();
        });

        Core::ProgressManager::addTask(future, tr("Checking docker daemon"), "DockerPlugin");
        return;
    }

    std::unique_lock lk(m_daemonCheckGuard);
    bool isAvailable = canConnect();
    if (!m_dockerDaemonAvailable.has_value() || isAvailable != m_dockerDaemonAvailable) {
        m_dockerDaemonAvailable = isAvailable;
        dockerDaemonAvailableChanged();
    }
}

void DockerApi::recheckDockerDaemon()
{
    QTC_ASSERT(s_instance, return );
    s_instance->checkCanConnect();
}

Utils::optional<bool> DockerApi::dockerDaemonAvailable(bool async)
{
    if (!m_dockerDaemonAvailable.has_value())
        checkCanConnect(async);
    return m_dockerDaemonAvailable;
}

Utils::optional<bool> DockerApi::isDockerDaemonAvailable(bool async)
{
    QTC_ASSERT(s_instance, return nullopt);
    return s_instance->dockerDaemonAvailable(async);
}

FilePath DockerApi::findDockerClient()
{
    if (m_dockerExecutable.isEmpty() || m_dockerExecutable.isExecutableFile())
        m_dockerExecutable = FilePath::fromString("docker").searchInPath();
    return m_dockerExecutable;
}

} // namespace Internal
} // namespace Docker
