// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerapi.h"

#include "dockerconstants.h"
#include "dockertr.h"

#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/async.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QHash>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLoggingCategory>

#include <thread>

Q_LOGGING_CATEGORY(dockerApiLog, "qtc.docker.api", QtWarningMsg);

using namespace Utils;

namespace Docker::Internal {

static QHash<Utils::Id, DockerApi *> s_instances;

DockerApi::DockerApi(ContainerToolSettings *settings)
    : m_settings(settings)
{
    Q_ASSERT(QThread::isMainThread());
    *m_dockerClientBinary.writeLocked() = m_settings->binaryPath.effectiveBinary();

    connect(&m_settings->binaryPath, &FilePathAspect::changed, this, [this]() {
        *m_dockerClientBinary.writeLocked() = m_settings->binaryPath.effectiveBinary();
        refreshNetworks();
    });

    s_instances.insert(m_settings->typeId(), this);

    refreshNetworks();
}

DockerApi::~DockerApi()
{
    s_instances.remove(m_settings->typeId());
}

DockerApi *DockerApi::instance()
{
    return s_instances.value(Docker::Constants::DOCKER_DEVICE_TYPE);
}

DockerApi *DockerApi::instance(Utils::Id typeId)
{
    return s_instances.value(typeId);
}

bool DockerApi::canConnect()
{
    Process process;
    FilePath dockerExe = dockerClient();
    if (dockerExe.isEmpty())
        return false;

    process.setCommand({dockerExe, {"info"}});
    process.runBlocking();

    const bool success = process.result() == ProcessResult::FinishedWithSuccess;
    if (!success) {
        qCWarning(dockerApiLog) << "Failed to connect to daemon:"
                                << process.verboseExitMessage();
    } else {
        qCInfo(dockerApiLog) << "'" + m_settings->displayType() + " info' result:\n"
                             << qPrintable(process.allOutput());
    }

    return process.result() == ProcessResult::FinishedWithSuccess;
}

bool DockerApi::isContainerRunning(const QString &containerId)
{
    Process process;
    FilePath dockerExe = dockerClient();
    if (dockerExe.isEmpty() || !dockerExe.isExecutableFile())
        return false;

    process.setCommand(
        CommandLine(dockerExe, QStringList{"inspect", "--format", "{{.State.Running}}", containerId}));
    process.runBlocking();

    if (process.result() == ProcessResult::FinishedWithSuccess) {
        QString output = process.readAllStandardOutput().trimmed();
        if (output == "true")
            return true;
    }

    return false;
}

bool DockerApi::imageExists(const QString &imageId)
{
    Process process;
    FilePath dockerExe = dockerClient();
    if (dockerExe.isEmpty() || !dockerExe.isExecutableFile())
        return false;

    process.setCommand(
        CommandLine(dockerExe, QStringList{"image", "inspect", imageId, "--format", "{{.Id}}"}));
    process.runBlocking();

    if (process.exitCode() == 0)
        return true;

    qCDebug(dockerApiLog) << "Failed to inspect image" << imageId << ":"
                          << process.verboseExitMessage();

    return false;
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

        Core::ProgressManager::addTask(
            future,
            Tr::tr("Checking %1 daemon").arg(m_settings->displayType()),
            "DockerPlugin");
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
    DockerApi *inst = instance();
    QTC_ASSERT(inst, return);
    inst->checkCanConnect();
}

void DockerApi::recheckDaemon(Utils::Id typeId)
{
    DockerApi *inst = instance(typeId);
    QTC_ASSERT(inst, return);
    inst->checkCanConnect();
}

std::optional<bool> DockerApi::dockerDaemonAvailable(bool async)
{
    if (!m_dockerDaemonAvailable.has_value())
        checkCanConnect(async);
    return m_dockerDaemonAvailable;
}

std::optional<bool> DockerApi::isDockerDaemonAvailable(bool async)
{
    return isDockerDaemonAvailable(Docker::Constants::DOCKER_DEVICE_TYPE, async);
}

std::optional<bool> DockerApi::isDockerDaemonAvailable(Utils::Id typeId, bool async)
{
    DockerApi *inst = instance(typeId);
    QTC_ASSERT(inst, return std::nullopt);
    return inst->dockerDaemonAvailable(async);
}

FilePath DockerApi::dockerClient()
{
    return m_dockerClientBinary.get();
}

QString DockerApi::displayType() const
{
    return m_settings->displayType();
}

QString Network::toString() const
{
    return QString(R"(ID: "%1"
Name: "%2"
Driver: "%3"
Scope: "%4"
Internal: "%5"
IPv6: "%6"
CreatedAt: "%7"
Labels: "%8"
    )")
        .arg(id)
        .arg(name)
        .arg(driver)
        .arg(scope)
        .arg(internal)
        .arg(ipv6)
        .arg(createdAt.toString(Qt::ISODate))
        .arg(labels);
}

void DockerApi::refreshNetworks()
{
    FilePath dockerExe = dockerClient();
    if (dockerExe.isEmpty() || !dockerExe.isExecutableFile()) {
        qCWarning(dockerApiLog)
            << Tr::tr("%1 executable not found").arg(m_settings->displayType());
        return;
    }

    const auto setupNetworkFetch = [dockerExe](Process &process) {
        process.setCommand({dockerExe, {"network", "ls", "--format", "{{json .}}"}});
    };

    const auto onDone = [this](const Process &process) {
        if (process.exitCode() != 0) {
            m_networks = Utils::ResultError(
                Tr::tr("Failed to list networks: %1").arg(process.verboseExitMessage()));
            emit networksChanged();
            return;
        }

        QList<Network> result;
        for (const auto &line : process.cleanedStdOut().split('\n')) {
            if (line.isEmpty())
                continue;

            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &error);

            if (error.error != QJsonParseError::NoError) {
                m_networks = Utils::ResultError(
                    Tr::tr("Failed to parse network info: %1").arg(error.errorString()));
                emit networksChanged();
                return;
            }

            Network network;
            network.id = doc["ID"].toString();
            network.name = doc["Name"].toString();
            network.driver = doc["Driver"].toString();
            network.scope = doc["Scope"].toString();
            network.internal = doc["Internal"].toString() == "true";
            network.ipv6 = doc["IPv6"].toString() == "true";
            network.createdAt = QDateTime::fromString(doc["CreatedAt"].toString(), Qt::ISODate);
            network.labels = doc["Labels"].toString();

            result.append(network);
        }

        m_networks = result;
        emit networksChanged();
    };

    m_taskTreeRunner.start({ProcessTask(setupNetworkFetch, onDone)});
}

} // namespace Docker::Internal
