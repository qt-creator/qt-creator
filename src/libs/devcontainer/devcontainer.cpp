// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainer.h"

#include "devcontainertr.h"

#include <tasking/barrier.h>
#include <tasking/conditional.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/overloaded.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QCryptographicHash>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(devcontainerlog, "devcontainer", QtDebugMsg)

using namespace Utils;

namespace DevContainer {

struct InstancePrivate
{
    Config config;
    InstanceConfig instanceConfig;
    Tasking::TaskTree taskTree;
};

// Generates a unique ID for the devcontainer instance based on the workspace folder
// and config file path.
QString InstanceConfig::devContainerId() const
{
    const QByteArray workspace = workspaceFolder.toUrlishString().toUtf8();
    const QByteArray config = configFilePath.toUrlishString().toUtf8();
    QString id = QString::fromLatin1(
        QCryptographicHash::hash(workspace, QCryptographicHash::Sha256).toHex());
    return id;
}

QString InstanceConfig::jsonToString(const QJsonValue &value) const
{
    QString str = value.toString();
    QRegularExpression re("\\$\\{([^}]+)\\}");
    QRegularExpressionMatchIterator it = re.globalMatch(str);

    if (it.hasNext()) {
        const QMap<QString, std::function<QString(QStringList)>> replacers = {
            {"localWorkspaceFolder", [this](const QStringList &) { return workspaceFolder.path(); }},
            {"localWorkspaceFolderBasename",
             [this](const QStringList &) { return workspaceFolder.fileName(); }},
            {"containerWorkspaceFolder",
             [this](const QStringList &) { return containerWorkspaceFolder.path(); }},
            {"containerWorkspaceFolderBasename",
             [this](const QStringList &) { return containerWorkspaceFolder.fileName(); }},
            {"devcontainerId", [this](const QStringList &) { return devContainerId(); }},
            {"localEnv",
             [this](const QStringList &parts) {
                 if (parts.isEmpty())
                     return QString();
                 const QString varname = parts.first();
                 const QString defaultValue = parts.mid(1).join(':');
                 return localEnvironment.value_or(varname, defaultValue);
             }},
        };

        struct Replace
        {
            qsizetype start;
            qsizetype length;
            QString replacement;
        };
        QList<Replace> replacements;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString varName = match.captured(1);
            QStringList parts = varName.split(':');
            QString variableName = parts.takeFirst();

            auto itReplacer = replacers.find(variableName);
            if (itReplacer != replacers.end()) {
                QString replacement = itReplacer.value()(parts);
                replacements.append({match.capturedStart(), match.capturedLength(), replacement});
            } else if (variableName != "containerEnv") {
                // Container env is only supported for "remoteEnv", but since it might be valid,
                // we don't warn about it here.
                logFunction(Tr::tr("Unsupported variable in devcontainer config:") + variableName);
            }
        }

        // Apply replacements in reverse order to avoid messing up indices
        for (auto it = replacements.crbegin(); it != replacements.crend(); ++it)
            str.replace(it->start, it->length, std::move(it->replacement));
    }

    return str;
}

Instance::Instance(Config config, InstanceConfig instanceConfig)
    : d(std::make_unique<InstancePrivate>())
{
    d->config = std::move(config);
    d->instanceConfig = std::move(instanceConfig);
}

Result<std::unique_ptr<Instance>> Instance::fromFile(
    const FilePath &filePath, InstanceConfig instanceConfig)
{
    const Result<Config> config = configFromFile(filePath, instanceConfig);
    if (!config)
        return ResultError(config.error());

    return std::make_unique<Instance>(*config, instanceConfig);
}

Result<Config> Instance::configFromFile(
    const Utils::FilePath &filePath, InstanceConfig instanceConfig)
{
    const Result<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return ResultError(contents.error());

    const Result<Config> config
        = Config::fromJson(*contents, [instanceConfig](const QJsonValue &value) {
              return instanceConfig.jsonToString(value);
          });

    return config;
}

std::unique_ptr<Instance> Instance::fromConfig(const Config &config, InstanceConfig instanceConfig)
{
    return std::make_unique<Instance>(config, instanceConfig);
}

Instance::~Instance() {};

struct ContainerDetails
{
    QString Id;
    QString Created;
    QString Name;

    struct
    {
        QString Status;
        QString StartedAt;
        QString FinishedAt;
    } State;

    struct
    {
        QString Image;
        QString User;
        QMap<QString, QString> Env;
        std::optional<QMap<QString, QString>> Labels;
    } Config;

    struct Mount
    {
        QString Type;
        std::optional<QString> Name;
        QString Source;
        QString Destination;
    };
    QList<Mount> Mounts;

    struct NetworkSettings
    {
        struct PortBinding
        {
            QString HostIp;
            QString HostPort;
        };
        QMap<QString, std::optional<QList<PortBinding>>> Ports;
    } NetworkSettings;

    struct Port
    {
        QString IP;
        int PrivatePort;
        int PublicPort;
        QString Type;
    };
    QList<Port> Ports;
};

// QDebug stream operator for ContainerDetails
QDebug operator<<(QDebug debug, const ContainerDetails &details)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ContainerDetails(Id: " << details.Id << ", Created: " << details.Created
                    << ", Name: " << details.Name << ", State: { Status: " << details.State.Status
                    << ", StartedAt: " << details.State.StartedAt
                    << ", FinishedAt: " << details.State.FinishedAt
                    << " }, Config: { Image: " << details.Config.Image
                    << ", User: " << details.Config.User << ", Env: " << details.Config.Env
                    << ", Labels: " << details.Config.Labels.value_or(QMap<QString, QString>())
                    << " }, Mounts: [";

    for (const auto &mount : details.Mounts) {
        debug.nospace() << "{ Type: " << mount.Type << ", Name: " << mount.Name.value_or(QString())
                        << ", Source: " << mount.Source << ", Destination: " << mount.Destination
                        << " }, ";
    }
    debug.nospace() << "] NetworkSettings: { Ports: ";

    for (auto it = details.NetworkSettings.Ports.constBegin();
         it != details.NetworkSettings.Ports.constEnd();
         ++it) {
        debug.nospace() << it.key() << ": ";
        if (it.value()) {
            for (const auto &binding : *it.value()) {
                debug.nospace() << "{ HostIp: " << binding.HostIp
                                << ", HostPort: " << binding.HostPort << " }, ";
            }
        } else {
            debug.nospace() << "(null), ";
        }
    }

    debug.nospace() << "} Ports: [";
    for (const auto &port : details.Ports) {
        debug.nospace() << "{ IP: " << port.IP << ", PrivatePort: " << port.PrivatePort
                        << ", PublicPort: " << port.PublicPort << ", Type: " << port.Type << " }, ";
    }
    debug.nospace() << "]";
    return debug;
}

struct RunningContainerDetails
{
    QString userName;
    QString userShell;
    Environment probedUserEnvironment;
};

struct ImageDetails
{
    QString Id;
    QString Architecture;
    std::optional<QString> Variant;
    QString Os;
    struct
    {
        QString User;
        std::optional<QStringList> Env;
        std::optional<QMap<QString, QString>> Labels;
        std::optional<QStringList> Entrypoint;
        std::optional<QStringList> Cmd;
    } Config;
};

// QDebug stream operator for ImageDetails
QDebug operator<<(QDebug debug, const ImageDetails &details)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ImageDetails(Id: " << details.Id
                    << ", Architecture: " << details.Architecture
                    << ", Variant: " << details.Variant.value_or(QString())
                    << ", Os: " << details.Os << ", Config: { User: " << details.Config.User
                    << ", Env: " << details.Config.Env.value_or(QStringList())
                    << ", Labels: " << details.Config.Labels.value_or(QMap<QString, QString>())
                    << ", Entrypoint: " << details.Config.Entrypoint.value_or(QStringList())
                    << ", Cmd: " << details.Config.Cmd.value_or(QStringList()) << " })";
    return debug;
}

using namespace Tasking;

static void connectProcessToLog(
    Process &process, const InstanceConfig &instanceConfig, const QString &context)
{
    process.setTextChannelMode(Channel::Output, TextChannelMode::MultiLine);
    process.setTextChannelMode(Channel::Error, TextChannelMode::MultiLine);
    QObject::connect(
        &process, &Process::textOnStandardOutput, [instanceConfig, context](const QString &text) {
            for (const auto &line : text.trimmed().split('\n')) {
                if (context.isEmpty())
                    instanceConfig.logFunction(line.trimmed());
                else
                    instanceConfig.logFunction(QString("[%1] %2").arg(context).arg(line.trimmed()));
            }
        });

    QObject::connect(
        &process, &Process::textOnStandardError, [instanceConfig, context](const QString &text) {
            for (const auto &line : text.trimmed().split('\n')) {
                if (context.isEmpty())
                    instanceConfig.logFunction(line.trimmed());
                else
                    instanceConfig.logFunction(QString("[%1] %2").arg(context).arg(line.trimmed()));
            }
        });
}

static QString imageName(const InstanceConfig &instanceConfig)
{
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(
            instanceConfig.workspaceFolder.nativePath().toUtf8(), QCryptographicHash::Sha256)
            .toHex());
    return QString("qtc-devcontainer-%1").arg(hash);
}

static QString containerName(const InstanceConfig &instanceConfig)
{
    return imageName(instanceConfig) + "-container";
}

static QStringList toAppPortArg(int port)
{
    return {"-p", QString("127.0.0.1:%1:%1").arg(port)};
}

static QStringList toAppPortArg(const QString &port)
{
    return {"-p", port};
}

static QStringList toAppPortArg(const QList<std::variant<int, QString>> &ports)
{
    QStringList args;
    for (const auto &port : ports) {
        args += std::visit(
            overloaded{
                [](int p) { return toAppPortArg(p); },
                [](const QString &p) { return toAppPortArg(p); }},
            port);
    }
    return args;
}

QStringList createAppPortArgs(std::variant<int, QString, QList<std::variant<int, QString>>> appPort)
{
    return std::visit(
        overloaded{
            [](int port) { return toAppPortArg(port); },
            [](const QString &port) { return toAppPortArg(port); },
            [](const QList<std::variant<int, QString>> &ports) { return toAppPortArg(ports); }},
        appPort);
}

static ProcessTask inspectContainerTask(
    Storage<ContainerDetails> containerDetails, const InstanceConfig &instanceConfig)
{
    const auto setupInspectContainer = [containerDetails, instanceConfig](Process &process) {
        CommandLine inspectCmdLine{
            instanceConfig.dockerCli,
            {"inspect", {"--type", "container"}, containerName(instanceConfig)}};

        process.setCommand(inspectCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString(Tr::tr("Inspecting Container: %1")).arg(process.commandLine().toUserOutput()));
    };

    const auto doneInspectContainer = [containerDetails](const Process &process) -> DoneResult {
        const auto output = process.cleanedStdOut();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(devcontainerlog)
                << "Failed to parse JSON from Docker inspect:" << error.errorString();
            qCWarning(devcontainerlog).noquote() << output;
            return DoneResult::Error;
        }
        if (!doc.isArray() || doc.array().isEmpty()) {
            qCWarning(devcontainerlog)
                << "Expected JSON array with one entry from Docker inspect, got:" << doc.toJson();
            return DoneResult::Error;
        }
        // Parse into ContainerDetails struct
        QJsonObject json = doc.array()[0].toObject();
        ContainerDetails details;
        details.Id = json.value("Id").toString();
        details.Created = json.value("Created").toString();
        details.Name = json.value("Name").toString().mid(1); // Remove leading '/'

        QJsonObject stateObj = json.value("State").toObject();
        details.State.Status = stateObj.value("Status").toString();
        details.State.StartedAt = stateObj.value("StartedAt").toString();
        details.State.FinishedAt = stateObj.value("FinishedAt").toString();

        QJsonObject configObj = json.value("Config").toObject();
        details.Config.Image = configObj.value("Image").toString();
        details.Config.User = configObj.value("User").toString();

        if (configObj.contains("Env")) {
            QJsonArray envArray = configObj.value("Env").toArray();
            details.Config.Env.clear();
            for (const QJsonValue &envValue : envArray) {
                if (!envValue.isString()) {
                    qCWarning(devcontainerlog)
                        << "Expected string in Env array, found:" << envValue;
                    continue;
                }
                const QString envValueStr = envValue.toString();
                const auto [key, value] = Utils::splitAtFirst(envValueStr, QLatin1Char('='));
                details.Config.Env.insert(key.toString(), value.toString());
            }
        }

        if (configObj.contains("Labels")) {
            QJsonObject labelsObj = configObj.value("Labels").toObject();
            details.Config.Labels = QMap<QString, QString>();
            for (auto it = labelsObj.begin(); it != labelsObj.end(); ++it)
                details.Config.Labels->insert(it.key(), it.value().toString());
        }

        // Parse Mounts
        if (json.contains("Mounts") && json["Mounts"].isArray()) {
            QJsonArray mountsArray = json["Mounts"].toArray();
            for (const QJsonValue &mountValue : mountsArray) {
                QJsonObject mountObj = mountValue.toObject();
                ContainerDetails::Mount mount;
                mount.Type = mountObj.value("Type").toString();
                if (mountObj.contains("Name"))
                    mount.Name = mountObj.value("Name").toString();
                mount.Source = mountObj.value("Source").toString();
                mount.Destination = mountObj.value("Destination").toString();
                details.Mounts.append(mount);
            }
        }

        // Parse NetworkSettings
        if (json.contains("NetworkSettings")) {
            QJsonObject networkSettingsObj = json.value("NetworkSettings").toObject();
            if (networkSettingsObj.contains("Ports")) {
                QJsonObject portsObj = networkSettingsObj.value("Ports").toObject();
                for (auto it = portsObj.begin(); it != portsObj.end(); ++it) {
                    QJsonArray portBindingsArray = it.value().toArray();
                    QList<ContainerDetails::NetworkSettings::PortBinding> portBindings;
                    for (const QJsonValue &bindingValue : portBindingsArray) {
                        QJsonObject bindingObj = bindingValue.toObject();
                        ContainerDetails::NetworkSettings::PortBinding binding;
                        binding.HostIp = bindingObj.value("HostIp").toString();
                        binding.HostPort = bindingObj.value("HostPort").toString();
                        portBindings.append(binding);
                    }
                    details.NetworkSettings.Ports.insert(it.key(), portBindings);
                }
            }
        }

        details.Ports.clear();
        for (auto it = details.NetworkSettings.Ports.constBegin();
             it != details.NetworkSettings.Ports.constEnd();
             ++it) {
            const QStringList parts = it.key().split(QLatin1Char('/'));
            if (parts.size() == 2) {
                bool okPrivatePort = false;
                const int privatePort = parts.at(0).toInt(&okPrivatePort);
                const QString type = parts.at(1);

                if (it.value()) {
                    for (const ContainerDetails::NetworkSettings::PortBinding &binding :
                         *it.value()) {
                        bool okPublicPort = false;
                        const int publicPort = binding.HostPort.toInt(&okPublicPort);

                        ContainerDetails::Port p;
                        p.IP = binding.HostIp;
                        p.PrivatePort = okPrivatePort ? privatePort : 0;
                        p.PublicPort = okPublicPort ? publicPort : 0;
                        p.Type = type;
                        details.Ports.append(p);
                    }
                }
            }
        }

        *containerDetails = details;

        qCDebug(devcontainerlog) << "Container details:" << details;

        return DoneResult::Success;
    };

    return ProcessTask{setupInspectContainer, doneInspectContainer};
}

static ProcessTask inspectImageTask(
    Storage<ImageDetails> imageDetails,
    const InstanceConfig &instanceConfig,
    const QString &imageName)
{
    const auto setupInspectImage = [imageDetails, instanceConfig, imageName](Process &process) {
        CommandLine
            inspectCmdLine{instanceConfig.dockerCli, {"inspect", {"--type", "image"}, imageName}};

        process.setCommand(inspectCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString(Tr::tr("Inspecting Image: %1")).arg(process.commandLine().toUserOutput()));
    };

    const auto doneInspectImage =
        [imageDetails](const Process &process, DoneWith doneWith) -> DoneResult {
        if (doneWith != DoneWith::Success) {
            qCWarning(devcontainerlog) << "Docker inspect failed with result:" << doneWith
                                       << "Output:" << process.cleanedStdOut();
            return DoneResult::Error;
        }

        const auto output = process.cleanedStdOut();
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            qCWarning(devcontainerlog)
                << "Failed to parse JSON from Docker inspect:" << error.errorString();
            qCWarning(devcontainerlog).noquote() << output;
            return DoneResult::Error;
        }
        if (!doc.isArray() || doc.array().isEmpty()) {
            qCWarning(devcontainerlog)
                << "Expected JSON array with one entry from Docker inspect, got:" << doc.toJson();
            return DoneResult::Error;
        }
        // Parse into ImageDetails struct
        QJsonObject json = doc.array()[0].toObject();
        ImageDetails details;
        details.Id = json.value("Id").toString();
        details.Architecture = json.value("Architecture").toString();
        if (json.contains("Variant"))
            details.Variant = json.value("Variant").toString();
        details.Os = json.value("Os").toString();
        QJsonObject config = json.value("Config").toObject();
        details.Config.User = config.value("User").toString();
        if (config.contains("Env")) {
            QJsonArray envArray = config.value("Env").toArray();
            details.Config.Env = QStringList();
            for (const QJsonValue &envValue : envArray)
                details.Config.Env->append(envValue.toString());
        }
        if (config.contains("Labels")) {
            QJsonObject labelsObj = config.value("Labels").toObject();
            details.Config.Labels = QMap<QString, QString>();
            for (auto it = labelsObj.begin(); it != labelsObj.end(); ++it)
                details.Config.Labels->insert(it.key(), it.value().toString());
        }
        if (config.contains("Entrypoint")) {
            QJsonArray entrypointArray = config.value("Entrypoint").toArray();
            details.Config.Entrypoint = QStringList();
            for (const QJsonValue &entryValue : entrypointArray)
                details.Config.Entrypoint->append(entryValue.toString());
        }
        if (config.contains("Cmd")) {
            QJsonArray cmdArray = config.value("Cmd").toArray();
            details.Config.Cmd = QStringList();
            for (const QJsonValue &cmdValue : cmdArray)
                details.Config.Cmd->append(cmdValue.toString());
        }
        *imageDetails = details;
        qCDebug(devcontainerlog) << "Image details:" << details;

        return DoneResult::Success;
    };

    return ProcessTask{setupInspectImage, doneInspectImage};
}

static QStringList generateMountArgs(
    const InstanceConfig &instanceConfig, const DevContainerCommon &commonConfig)
{
    auto mountToString = [](const std::variant<Mount, QString> &mount) -> QString {
        return std::visit(
            overloaded{
                [](const Mount &m) {
                    const QString type = m.type == MountType::Bind ? QString("bind")
                                                                   : QString("volume");
                    const QString source = m.source ? ",source=" + *m.source : QString();
                    return QString("--mount=type=%1,target=%2%3").arg(type).arg(m.target).arg(source);
                },
                [](const QString &m) { return QString("--mount=%1").arg(m); }},
            mount);
    };

    return Utils::transform<QStringList>(commonConfig.mounts, mountToString)
           + Utils::transform<QStringList>(instanceConfig.mounts, mountToString);
}

template<typename C>
static void setupCreateContainerFromImage(
    const C &containerConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig,
    const ImageDetails &imageDetails,
    Process &process)
{
    connectProcessToLog(process, instanceConfig, Tr::tr("Create Container"));

    QStringList containerEnvArgs;

    for (auto &[key, value] : commonConfig.containerEnv)
        containerEnvArgs << "-e" << QString("%1=%2").arg(key, value);

    QStringList appPortArgs;

    if (containerConfig.appPort)
        appPortArgs = createAppPortArgs(*containerConfig.appPort);

    QStringList customEntryPoints = {}; // TODO: Get entry points from features.

    QStringList cmd
        = {"-c",
           QString(R"(echo Container started.
trap "exit 0" TERM
%1
exec "$@"
while sleep 1 & wait $!; do :; done
)")
               .arg(customEntryPoints.join('\n')),
           "-"};

    if (!containerConfig.overrideCommand) {
        cmd.append(imageDetails.Config.Entrypoint.value_or(QStringList()));
        cmd.append(imageDetails.Config.Cmd.value_or(QStringList()));
    }
    QStringList workspaceMountArgs;

    if (containerConfig.workspaceFolder && containerConfig.workspaceMount) {
        workspaceMountArgs = {"--mount", *containerConfig.workspaceMount};
    } else {
        workspaceMountArgs
            = {"--mount",
               QString("type=bind,source=%1,target=%2")
                   .arg(
                       instanceConfig.workspaceFolder.toUrlishString(),
                       instanceConfig.containerWorkspaceFolder.toUrlishString())};
    }

    CommandLine createCmdLine{
        instanceConfig.dockerCli,
        {"create",
         {"--name", containerName(instanceConfig)},
         containerEnvArgs,
         appPortArgs,
         workspaceMountArgs,
         generateMountArgs(instanceConfig, commonConfig),
         {"--entrypoint", "/bin/sh"},
         imageName(instanceConfig),
         cmd}};
    process.setCommand(createCmdLine);
    process.setWorkingDirectory(instanceConfig.workspaceFolder);

    instanceConfig.logFunction(
        QString(Tr::tr("Creating Container: %1")).arg(process.commandLine().toUserOutput()));
}

static ProcessTask eventMonitor(const QString &eventType, const InstanceConfig &instanceConfig)
{
    const auto monitorSetup = [instanceConfig, eventType](Process &process) {
        CommandLine eventsCmdLine
            = {instanceConfig.dockerCli,
               {"events",
                {"--filter", QString("event=%1").arg(eventType)},
                {"--filter", QString("container=%1").arg(containerName(instanceConfig))},
                {"--format", "{{json .}}"}}};

        process.setCommand(eventsCmdLine);

        instanceConfig.logFunction(
            QString(Tr::tr("Waiting for Container to Start: %1")).arg(eventsCmdLine.toUserOutput()));

        process.setTextChannelMode(Channel::Output, TextChannelMode::SingleLine);
        process.setTextChannelMode(Channel::Error, TextChannelMode::SingleLine);
        QObject::connect(
            &process,
            &Process::textOnStandardOutput,
            [&process, eventType, instanceConfig](const QString &text) {
                instanceConfig.logFunction(QString("[Event Monitor] %1").arg(text));

                QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
                if (doc.isNull() || !doc.isObject()) {
                    qCWarning(devcontainerlog)
                        << "Received invalid JSON from Docker events:" << text;
                    return;
                }
                QJsonObject event = doc.object();
                if (event.contains("status") && event["status"].toString() == eventType
                    && event.contains("id")) {
                    qCDebug(devcontainerlog) << "Container started:" << event["id"].toString();
                    process.stop();
                } else {
                    qCWarning(devcontainerlog) << "Unexpected Docker event:" << event;
                }
            });

        QObject::connect(
            &process, &Process::textOnStandardError, [instanceConfig](const QString &text) {
                instanceConfig.logFunction(QString("[Event Monitor] %1").arg(text));
                qCWarning(devcontainerlog) << "Docker events error:" << text;
            });
    };

    return ProcessTask(monitorSetup, DoneResult::Success);
}

static QString containerUser(const ContainerDetails &containerDetails)
{
    if (containerDetails.Config.User.isEmpty())
        return QString("root");

    static QRegularExpression nameGroupRegex("([^:]*)(:(.*))?");

    QRegularExpressionMatch match = nameGroupRegex.match(containerDetails.Config.User);
    if (!match.hasMatch()) {
        qCWarning(devcontainerlog)
            << "Failed to parse user from container details:" << containerDetails.Config.User;
        return QString("root");
    }

    if (match.captured(1).isEmpty())
        return QString("root");

    return match.captured(1);
}

static ExecutableItem execInContainerTask(
    const QString &logPrefix,
    const InstanceConfig &instanceConfig,
    const std::variant<std::function<QString()>, std::function<CommandLine()>, CommandLine, QString>
        &cmdLine,
    const ProcessTask::TaskDoneHandler &doneHandler)
{
    const auto setupExec = [instanceConfig, cmdLine, logPrefix](Process &process) {
        connectProcessToLog(process, instanceConfig, logPrefix);

        CommandLine execCmdLine{instanceConfig.dockerCli, {"exec", containerName(instanceConfig)}};
        if (std::holds_alternative<CommandLine>(cmdLine)) {
            execCmdLine.addCommandLineAsArgs(std::get<CommandLine>(cmdLine));
        } else if (std::holds_alternative<QString>(cmdLine)) {
            execCmdLine.addArgs({std::get<QString>(cmdLine)}, CommandLine::Raw);
        } else if (std::holds_alternative<std::function<QString()>>(cmdLine)) {
            const QString cmd = std::get<std::function<QString()>>(cmdLine)();
            if (cmd.isEmpty()) {
                qCWarning(devcontainerlog) << "Empty command provided for execInContainerTask.";
                return;
            }
            execCmdLine.addArgs({cmd}, CommandLine::Raw);
        } else if (std::holds_alternative<std::function<CommandLine()>>(cmdLine)) {
            const CommandLine cmd = std::get<std::function<CommandLine()>>(cmdLine)();
            if (cmd.isEmpty()) {
                qCWarning(devcontainerlog) << "Empty command provided for execInContainerTask.";
                return;
            }
            execCmdLine.addCommandLineAsArgs(cmd);
        } else {
            qCWarning(devcontainerlog) << "Unsupported command line type for execInContainerTask.";
            return;
        }

        process.setCommand(execCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString(Tr::tr("Executing in Container: %1")).arg(process.commandLine().toUserOutput()));
    };

    return ProcessTask{setupExec, doneHandler};
}

static ExecutableItem probeUserEnvTask(
    Storage<RunningContainerDetails> containerDetails,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    if (commonConfig.userEnvProbe == UserEnvProbe::None)
        return Group{};

    static const QMap<UserEnvProbe, QString> shellLoginMap{
        {UserEnvProbe::None, "-c"},
        {UserEnvProbe::InteractiveShell, "-ic"},
        {UserEnvProbe::LoginShell, "-lc"},
        {UserEnvProbe::LoginInteractiveShell, "-lic"}};

    const QString shellArg = shellLoginMap[commonConfig.userEnvProbe];

    return execInContainerTask(
        "Probe User Environment",
        instanceConfig,
        [containerDetails, shellArg]() -> CommandLine {
            return {FilePath::fromUserInput(containerDetails->userShell), {shellArg, "printenv"}};
        },
        [containerDetails,
         commonConfig,
         instanceConfig](const Process &process, DoneWith doneWith) -> DoneResult {
            if (doneWith == DoneWith::Error) {
                qCWarning(devcontainerlog)
                    << "Failed to probe user environment:" << process.verboseExitMessage();
                return DoneResult::Error;
            }

            const QString output = process.cleanedStdOut().trimmed();
            if (output.isEmpty()) {
                qCWarning(devcontainerlog) << "No output from user environment probe.";
                return DoneResult::Success;
            }

            Environment env(output.split('\n', Qt::SkipEmptyParts));

            // We don't want to capture the following environment variables:
            for (const char *key : {"_", "PWD"})
                env.unset(QLatin1StringView(key));

            containerDetails->probedUserEnvironment = env;

            return DoneResult::Success;
        });
}

struct UserFromPasswd
{
    QString name;
    QString uid;
    QString gid;
    QString home;
    QString shell;
};

Result<UserFromPasswd> parseUserFromPasswd(const QString &passwdLine)
{
    QStringList row = passwdLine.trimmed().split(QLatin1Char(':'));
    QTC_ASSERT(row.size() >= 7, return ResultError(Tr::tr("Invalid passwd line: %1").arg(passwdLine)));
    return UserFromPasswd{
        row.value(0),
        row.value(2),
        row.value(3),
        row.value(5),
        row.value(6),
    };
}

static ExecutableItem runningContainerDetailsTask(
    Storage<ContainerDetails> containerDetails,
    Storage<RunningContainerDetails> runningDetails,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const ExecutableItem idTask = execInContainerTask(
        "Get Running Container User",
        instanceConfig,
        CommandLine{"id", {"-un"}},
        [runningDetails](const Process &process, DoneWith doneWith) -> DoneResult {
            if (doneWith == DoneWith::Error) {
                qCWarning(devcontainerlog)
                    << "Failed to get running container user:" << process.verboseExitMessage();
                return DoneResult::Error;
            }

            const QString user = process.cleanedStdOut().trimmed();
            runningDetails->userName = user;
            qCDebug(devcontainerlog) << "Running container user:" << user;
            return DoneResult::Success;
        });

    const ExecutableItem shellTask = execInContainerTask(
        "Get Running Container User Shell",
        instanceConfig,
        [containerDetails, runningDetails]() -> CommandLine {
            const QString userName = containerUser(*containerDetails);
            QString userEscapedForShell = userName;
            userEscapedForShell.replace(QRegularExpression("(['\\\\])"), "\\\\1");
            QString userEscapedForGrep = userName;
            userEscapedForGrep.replace(QRegularExpression("([.*+?^${}()|[\\]\\\\])"), "\\\\1")
                .replace('\'', "\\'");

            CommandLine testGetEnt{
                "command",
                {"-v", "getent", {">/dev/null", CommandLine::Raw}, {"2>&1", CommandLine::Raw}}};
            const CommandLine getPasswdViaGetent{"getent", {"passwd", userName}};
            const CommandLine getPasswdViaGrep{
                "grep",
                {"-E", QString("^(%1|^[^:]*:[^:]*:%1:)").arg(userEscapedForGrep), "/etc/passwd"}};
            const CommandLine trueCmd{"true"};

            testGetEnt.addCommandLineWithAnd(getPasswdViaGetent);
            testGetEnt.addCommandLineWithOr(getPasswdViaGrep);
            testGetEnt.addCommandLineWithOr(trueCmd);

            CommandLine getShellCmd{"/bin/sh", {"-c"}};
            getShellCmd.addCommandLineAsSingleArg(testGetEnt);
            return getShellCmd;
        },
        [containerDetails, runningDetails](const Process &process, DoneWith doneWith) -> DoneResult {
            const QString output = process.cleanedStdOut().trimmed();

            runningDetails->userShell = containerDetails->Config.Env.value("SHELL", "/bin/sh");
            qCDebug(devcontainerlog)
                << "Running container user shell (default):" << runningDetails->userShell;

            if (output.isEmpty() || doneWith == DoneWith::Error) {
                qCWarning(devcontainerlog) << "Failed to get running container user shell:"
                                           << process.verboseExitMessage();
                return DoneResult::Success;
            }

            auto user = parseUserFromPasswd(output);
            if (!user) {
                qCWarning(devcontainerlog) << "Failed to parse user from passwd line:" << output;
                return DoneResult::Error;
            }

            qCDebug(devcontainerlog)
                << "Running container user:" << user->name << "UID:" << user->uid
                << "GID:" << user->gid << "Home:" << user->home << "Shell:" << user->shell;

            runningDetails->userShell = user->shell;

            return DoneResult::Success;
        });

    return Group{idTask, shellTask, probeUserEnvTask(runningDetails, commonConfig, instanceConfig)};
}

static ProcessTask lifecycleHookTask(
    const CommandLine &cmdLine, const InstanceConfig &instanceConfig, const QString &name)
{
    return ProcessTask([cmdLine, instanceConfig, name](Process &process) {
        connectProcessToLog(process, instanceConfig, name);
        process.setCommand(cmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);
    });
}

static ExecutableItem singleCommandLifecycleRecipe(
    const InstanceConfig &instanceConfig,
    const QString &command,
    std::optional<CommandLine> dockerExecCmd,
    const QString &name = {})
{
    if (command.isEmpty())
        return Group{};

    // If dockerExecCmd is provided, we execute the command in the container using a shell.
    // If not, we execute the command through a host shell.
    static const CommandLine hostShell = HostOsInfo::isWindowsHost()
                                             ? CommandLine{"cmd.exe", {"/c"}}
                                             : CommandLine{"/bin/sh", {"-c"}};

    // We either execute the command in a shell running on the host ...
    CommandLine cmdLine;
    if (dockerExecCmd) {
        cmdLine = *dockerExecCmd;
        cmdLine.addArgs({"/bin/sh", "-c"});
    } else {
        cmdLine = hostShell;
    }

    cmdLine.addArg(command);

    return lifecycleHookTask(cmdLine, instanceConfig, name);
}

static ExecutableItem singleCommandLifecycleRecipe(
    const InstanceConfig &instanceConfig,
    const QStringList &command,
    std::optional<CommandLine> dockerExecCmd,
    const QString &name = {})
{
    if (command.isEmpty())
        return Group{};

    const CommandLine cmdLineFromList
        = CommandLine(FilePath::fromUserInput(command[0]), command.mid(1));

    CommandLine cmdLine;
    if (dockerExecCmd) {
        cmdLine = *dockerExecCmd;
        cmdLine.addCommandLineAsArgs(cmdLineFromList);
    } else {
        cmdLine = cmdLineFromList;
    }

    return lifecycleHookTask(cmdLine, instanceConfig, name);
}

static ExecutableItem singleCommandLifecycleRecipe(
    const InstanceConfig &instanceConfig,
    std::variant<QString, QStringList> command,
    std::optional<CommandLine> dockerExecCmd,
    const QString &name = {})
{
    // https://containers.dev/implementors/json_reference/#formatting-string-vs-array-properties
    // * If the command is a QString, it is executed through a shell.
    // * If its a QStringList, it is executed directly without a shell.

    return std::visit(
        overloaded{
            [&](const QString &cmd) {
                return singleCommandLifecycleRecipe(instanceConfig, cmd, dockerExecCmd, name);
            },
            [&](const QStringList &cmd) {
                return singleCommandLifecycleRecipe(instanceConfig, cmd, dockerExecCmd, name);
            }},
        command);
}

static ExecutableItem lifecycleHookRecipe(
    const QString &hookName,
    const InstanceConfig &instanceConfig,
    std::optional<Command> command,
    std::optional<CommandLine> dockerExecCmd = std::nullopt)
{
    if (!command)
        return Group{};

    auto logExecution = Sync([instanceConfig, hookName] {
        instanceConfig.logFunction(QString("Executing the %1 hook from %2")
                                       .arg(hookName)
                                       .arg(instanceConfig.configFilePath.fileName()));
    });

    const QList<GroupItem> cmds = std::visit(
        overloaded{
            [&](const QString &cmd) -> GroupItems {
                return {singleCommandLifecycleRecipe(instanceConfig, cmd, dockerExecCmd, hookName)};
            },
            [&](const QStringList &cmd) -> GroupItems {
                return {singleCommandLifecycleRecipe(instanceConfig, cmd, dockerExecCmd, hookName)};
            },
            [&](const CommandMap &map) {
                GroupItems commands;
                for (const auto &[name, cmd] : map) {
                    commands.push_back(
                        singleCommandLifecycleRecipe(instanceConfig, cmd, dockerExecCmd, name));
                }
                return commands;
            }},
        *command);

    return Group{logExecution, Group{parallelIdealThreadCountLimit, cmds}};
}

static ExecutableItem runLifecycleHooksRecipe(
    DevContainerCommon commonConfig, const InstanceConfig &instanceConfig)
{
    const CommandLine dockerExecPrefix
        = CommandLine{instanceConfig.dockerCli, {"exec", containerName(instanceConfig)}};

    struct Cmd
    {
        const QString name;
        const std::optional<Command> command;
    };
    const QList<Cmd> lifecycleHooks
        = {{"onCreateCommand", commonConfig.onCreateCommand},
           {"updateContentCommand", commonConfig.updateContentCommand},
           {"postCreateCommand", commonConfig.postCreateCommand},
           {"postStartCommand", commonConfig.postStartCommand},
           {"postAttachCommand", commonConfig.postAttachCommand}};

    GroupItems remoteItems = Utils::transform(lifecycleHooks, [&](const Cmd &hook) -> GroupItem {
        return lifecycleHookRecipe(hook.name, instanceConfig, hook.command, dockerExecPrefix);
    });

    GroupItems localItems = {
        lifecycleHookRecipe("initializeCommand", instanceConfig, commonConfig.initializeCommand)};

    return Group(localItems + remoteItems);
}

static ExecutableItem containerDoesNotExistTask(const InstanceConfig &instanceConfig)
{
    return ProcessTask(
        [instanceConfig](Process &process) {
            CommandLine cmdLine{
                instanceConfig.dockerCli,
                {"container",
                 "ls",
                 "-a",
                 "--format",
                 "{{.Names}}",
                 "--filter",
                 QString("name=%1").arg(containerName(instanceConfig))}};
            process.setCommand(cmdLine);
            process.setWorkingDirectory(instanceConfig.workspaceFolder);
        },
        [instanceConfig](const Process &process, DoneWith doneWith) -> DoneResult {
            if (doneWith == DoneWith::Error) {
                qCWarning(devcontainerlog)
                    << "Failed to check if container exists:" << process.cleanedStdErr();
                return DoneResult::Error;
            }
            const QString output = process.cleanedStdOut().trimmed();
            if (output == containerName(instanceConfig)) {
                qCWarning(devcontainerlog) << "Container already exists:" << output;
                return DoneResult::Error;
            }

            return DoneResult::Success;
        });
}

static ExecutableItem containerState(const InstanceConfig &instanceConfig, Storage<QString> state)
{
    return ProcessTask(
        [instanceConfig](Process &process) {
            CommandLine cmdLine{
                instanceConfig.dockerCli,
                {"container",
                 "ls",
                 "-a",
                 "--format",
                 "{{.State}}",
                 "--filter",
                 QString("name=%1").arg(containerName(instanceConfig))}};

            process.setCommand(cmdLine);
            process.setWorkingDirectory(instanceConfig.workspaceFolder);
        },
        [instanceConfig, state](const Process &process, DoneWith doneWith) -> DoneResult {
            if (doneWith == DoneWith::Error) {
                qCWarning(devcontainerlog) << "Failed to check if container state"
                                           << ":" << process.cleanedStdErr();
                return DoneResult::Error;
            }
            *state = process.cleanedStdOut().trimmed();
            return DoneResult::Success;
        });
}

template<typename C>
static ExecutableItem createContainerRecipe(
    Storage<ImageDetails> imageDetails,
    const C &containerConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    auto createContainerSetup =
        [imageDetails, containerConfig, commonConfig, instanceConfig](Process &process) {
            setupCreateContainerFromImage(
                containerConfig, commonConfig, instanceConfig, *imageDetails, process);
        };

    // clang-format off
    return Group {
        If (containerDoesNotExistTask(instanceConfig)) >> Then {
            ProcessTask(createContainerSetup)
        }
    };
    // clang-format on
}

static ExecutableItem startContainerRecipe(const InstanceConfig &instanceConfig)
{
    const auto start = [instanceConfig] {
        return ProcessTask([instanceConfig](Process &process) {
            connectProcessToLog(process, instanceConfig, Tr::tr("Start Container"));

            CommandLine
                startCmdLine{instanceConfig.dockerCli, {"start", containerName(instanceConfig)}};
            process.setCommand(startCmdLine);
            process.setWorkingDirectory(instanceConfig.workspaceFolder);

            instanceConfig.logFunction(
                QString(Tr::tr("Starting Container: %1")).arg(process.commandLine().toUserOutput()));
        });
    };

    const auto unpause = [instanceConfig] {
        return ProcessTask([instanceConfig](Process &process) {
            connectProcessToLog(process, instanceConfig, Tr::tr("Resume Container"));

            CommandLine
                startCmdLine{instanceConfig.dockerCli, {"unpause", containerName(instanceConfig)}};
            process.setCommand(startCmdLine);
            process.setWorkingDirectory(instanceConfig.workspaceFolder);

            instanceConfig.logFunction(
                QString(Tr::tr("Resuming Container: %1")).arg(process.commandLine().toUserOutput()));
        });
    };

    Storage<QString> containerStateStorage;

    const auto readyToStart = [containerStateStorage] {
        return (*containerStateStorage == "created" || *containerStateStorage == "exited")
                   ? DoneResult::Success
                   : DoneResult::Error;
    };
    const auto paused = [containerStateStorage] {
        return *containerStateStorage == "paused" ? DoneResult::Success : DoneResult::Error;
    };

    // clang-format off
    return Group
    {
        containerStateStorage,
        containerState(instanceConfig, containerStateStorage),
        Group {
            If (readyToStart) >> Then {
                When (eventMonitor("start", instanceConfig), &Process::started) >> Do {
                    start()
                }
            } >> ElseIf (paused) >> Then {
                When (eventMonitor("unpause", instanceConfig), &Process::started) >> Do {
                    unpause()
                }
            }
        },
    };
    // clang-format on
}

static Result<Group> prepareContainerRecipe(
    const DockerfileContainer &containerConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const auto setupBuildImage = [containerConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, Tr::tr("Build Dockerfile"));

        const FilePath configFileDir = instanceConfig.configFilePath.parentDir();
        const FilePath contextPath = configFileDir.resolvePath(containerConfig.context);
        const FilePath dockerFile = configFileDir.resolvePath(containerConfig.dockerfile);

        CommandLine buildCmdLine{
            instanceConfig.dockerCli,
            {"build",
             {"-f", dockerFile.nativePath()},
             {"-t", imageName(instanceConfig)},
             contextPath.nativePath()}};
        process.setCommand(buildCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString(Tr::tr("Building Dockerfile: %1")).arg(process.commandLine().toUserOutput()));
    };

    Storage<ImageDetails> imageDetails;
    Storage<ContainerDetails> containerDetails;
    Storage<RunningContainerDetails> runningDetails;

    // clang-format off
    return Group {
        imageDetails,
        runningDetails,
        containerDetails,

        ProcessTask(setupBuildImage),
        inspectImageTask(imageDetails, instanceConfig, imageName(instanceConfig)),
        createContainerRecipe(
            imageDetails, containerConfig, commonConfig, instanceConfig),
        inspectContainerTask(containerDetails, instanceConfig),
        startContainerRecipe(instanceConfig),
        runningContainerDetailsTask(containerDetails, runningDetails, commonConfig, instanceConfig),
        runLifecycleHooksRecipe(commonConfig, instanceConfig),
    };
    // clang-format on
}

static ExecutableItem prepareDockerImageRecipe(
    Storage<ImageDetails> imageDetails,
    const ImageContainer &imageConfig,
    const InstanceConfig &instanceConfig)
{
    const auto setupPullImage = [imageConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Pull Image");

        CommandLine pullCmdLine{instanceConfig.dockerCli, {"pull", imageConfig.image}};
        process.setCommand(pullCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Pulling Image: %1").arg(process.commandLine().toUserOutput()));
    };

    const auto setupTagImage = [imageConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Tag Image");

        CommandLine tagCmdLine{
            instanceConfig.dockerCli, {"tag", imageConfig.image, imageName(instanceConfig)}};
        process.setCommand(tagCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Tagging Image: %1").arg(process.commandLine().toUserOutput()));
    };

    return Group{
        If(inspectImageTask(imageDetails, instanceConfig, imageConfig.image)) >> Then{
            ProcessTask(setupTagImage),
        } >> Else {
            ProcessTask(setupPullImage),
            ProcessTask(setupTagImage),
            inspectImageTask(imageDetails, instanceConfig, imageName(instanceConfig)),
        }};
}

static Result<Group> prepareContainerRecipe(
    const ImageContainer &imageConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    Storage<ImageDetails> imageDetails;
    Storage<ContainerDetails> containerDetails;
    Storage<RunningContainerDetails> runningDetails;

    // clang-format off
    return Group {
        imageDetails,
        containerDetails,
        runningDetails,
        prepareDockerImageRecipe(imageDetails, imageConfig, instanceConfig),
        createContainerRecipe(imageDetails, imageConfig, commonConfig, instanceConfig),
        inspectContainerTask(containerDetails, instanceConfig),
        startContainerRecipe(instanceConfig),
        runningContainerDetailsTask(containerDetails, runningDetails, commonConfig, instanceConfig),
        runLifecycleHooksRecipe(commonConfig, instanceConfig),
    };
    // clang-format on
}

static Result<Group> prepareContainerRecipe(
    const ComposeContainer &config,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    Q_UNUSED(commonConfig);
    const auto setupComposeUp = [config, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Compose Up");

        const FilePath configFileDir = instanceConfig.configFilePath.parentDir();

        QStringList composeFiles = std::visit(
            overloaded{
                [](const QString &file) { return QStringList{file}; },
                [](const QStringList &files) { return files; }},
            config.dockerComposeFile);

        composeFiles
            = Utils::transform(composeFiles, [&configFileDir](const QString &relativeComposeFile) {
                  return configFileDir.resolvePath(relativeComposeFile).nativePath();
              });

        QStringList composeFilesWithFlag;
        for (const QString &file : composeFiles) {
            composeFilesWithFlag.append("-f");
            composeFilesWithFlag.append(file);
        }

        QStringList runServices = config.runServices.value_or(QStringList{});
        QSet<QString> services = {config.service};
        services.unite({runServices.begin(), runServices.end()});

        CommandLine composeCmdLine{
            instanceConfig.dockerComposeCli,
            {"up",
             composeFilesWithFlag,
             {
                 "--build",
                 "--detach",
             },
             services.values()}};
        process.setCommand(composeCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Compose Up: %1").arg(process.commandLine().toUserOutput()));
    };

    return ResultError("Docker Compose is not yet supported in DevContainer.");
}

static Result<Group> prepareRecipe(const Config &config, const InstanceConfig &instanceConfig)
{
    return std::visit(
        [&instanceConfig, commonConfig = config.common](const auto &containerConfig) {
            return prepareContainerRecipe(containerConfig, commonConfig, instanceConfig);
        },
        *config.containerConfig);
}

static void setupRemoveContainer(const InstanceConfig &instanceConfig, Process &process)
{
    connectProcessToLog(process, instanceConfig, Tr::tr("Remove Container"));

    CommandLine removeCmdLine{instanceConfig.dockerCli, {"rm", "-f", containerName(instanceConfig)}};
    process.setCommand(removeCmdLine);
    process.setWorkingDirectory(instanceConfig.workspaceFolder);

    instanceConfig.logFunction(
        QString(Tr::tr("Removing Container: %1")).arg(process.commandLine().toUserOutput()));
}

static Result<Group> downContainerRecipe(
    const DockerfileContainer &containerConfig, const InstanceConfig &instanceConfig)
{
    const auto setupRMContainer = [containerConfig, instanceConfig](Process &process) {
        setupRemoveContainer(instanceConfig, process);
    };

    return Group{ProcessTask(setupRMContainer)};
}

static Result<Group> downContainerRecipe(
    const ImageContainer &imageConfig, const InstanceConfig &instanceConfig)
{
    const auto setupRemoveImage = [imageConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, Tr::tr("Remove Image"));

        CommandLine removeCmdLine{instanceConfig.dockerCli, {"rmi", imageName(instanceConfig)}};
        process.setCommand(removeCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString(Tr::tr("Removing Image: %1")).arg(process.commandLine().toUserOutput()));
    };

    const auto setupRMContainer = [imageConfig, instanceConfig](Process &process) {
        setupRemoveContainer(instanceConfig, process);
    };

    return Group{ProcessTask(setupRMContainer), ProcessTask(setupRemoveImage)};
}

static Result<Group> downContainerRecipe(
    const ComposeContainer &config, const InstanceConfig &instanceConfig)
{
    Q_UNUSED(config);
    const auto setupComposeDown = [instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Compose Down");

        CommandLine composeCmdLine{instanceConfig.dockerComposeCli, {"down", "--remove-orphans"}};
        process.setCommand(composeCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Compose Down: %1").arg(process.commandLine().toUserOutput()));
    };

    return Group{ProcessTask(setupComposeDown)};
}

static Result<Group> downRecipe(const Config &config, const InstanceConfig &instanceConfig)
{
    return std::visit(
        [&instanceConfig](const auto &containerConfig) {
            return downContainerRecipe(containerConfig, instanceConfig);
        },
        *config.containerConfig);
}

Result<> Instance::up()
{
    if (!d->config.containerConfig)
        return ResultOk;

    const Utils::Result<Tasking::Group> recipeResult = upRecipe();
    if (!recipeResult)
        return ResultError(recipeResult.error());

    d->taskTree.setRecipe(std::move(*recipeResult));
    d->taskTree.start();

    return ResultOk;
}

Result<> Instance::down()
{
    if (!d->config.containerConfig)
        return ResultOk;

    const Utils::Result<Tasking::Group> recipeResult = downRecipe();
    if (!recipeResult)
        return ResultError(recipeResult.error());
    d->taskTree.setRecipe(std::move(*recipeResult));
    d->taskTree.start();

    return ResultOk;
}

Result<Tasking::Group> Instance::upRecipe() const
{
    return prepareRecipe(d->config, d->instanceConfig);
}

Result<Tasking::Group> Instance::downRecipe() const
{
    return ::DevContainer::downRecipe(d->config, d->instanceConfig);
}

const Config &Instance::config() const
{
    return d->config;
}

} // namespace DevContainer
