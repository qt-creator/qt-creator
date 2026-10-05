// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainer.h"

#include "devcontainerfeature.h"
#include "devcontainertr.h"
#include "featuredownloader.h"
#include "substitute.h"

#include <QtTaskTree/QBarrier>
#include <QtTaskTree/QConditional>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>
#include <utils/unarchiver.h>
#include <utils/utility.h>

#include <QCryptographicHash>
#include <QDir>
#include <QLoggingCategory>
#include <QSet>
#include <QTemporaryDir>

#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

static Q_LOGGING_CATEGORY(devcontainerlog, "devcontainer", QtWarningMsg)

using namespace Utils;
using namespace QtTaskTree;

namespace DevContainer {

struct InstancePrivate
{
    Config config;
    InstanceConfig instanceConfig;
    QTaskTree taskTree;
};

using DynamicString = std::variant<QString, Storage<QString>, std::function<QString()>>;

static QString dynamicStringToString(const DynamicString &containerId)
{
    return std::visit(
        overloaded{
            [](const QString &id) { return id; },
            [](const Storage<QString> &id) { return *id; },
            [](const std::function<QString()> &f) { return f(); }},
        containerId);
}

// Generates a unique ID for the devcontainer instance based on the workspace folder
// and config file path.
QString InstanceConfig::devContainerId() const
{
    const QByteArray workspace = workspaceFolder.toUrlishString().toUtf8();
    const QByteArray config = configFilePath.toUrlishString().toUtf8();
    const QByteArray combined = workspace + config;
    QString id = QString::fromLatin1(
        QCryptographicHash::hash(combined, QCryptographicHash::Sha256).toHex());
    return id;
}

std::optional<LocalUser> InstanceConfig::defaultLocalUser()
{
#ifdef Q_OS_LINUX
    return LocalUser{uint(::getuid()), uint(::getgid())};
#else
    return std::nullopt;
#endif
}

QString InstanceConfig::jsonToString(const QJsonValue &value) const
{
    QString str = value.toString();

    const Internal::Replacers replacers
        = {{"localWorkspaceFolder", [this](const QStringList &) { return workspaceFolder.path(); }},
           {"localWorkspaceFolderBasename",
            [this](const QStringList &) { return workspaceFolder.fileName(); }},
           {"devcontainerId", [this](const QStringList &) { return devContainerId(); }},
           {"localEnv",
            [this](const QStringList &parts) {
                if (parts.isEmpty())
                    return QString();
                const QString varname = parts.first();
                const QString defaultValue = parts.mid(1).join(':');
                return localEnvironment.value_or(varname, defaultValue);
            }},
           {"containerEnv",
            [](const QStringList &parts) { return QString("${%1}").arg(parts.join(':')); }}};

    Internal::substituteVariables(str, replacers);
    return str;
}

Instance::Instance(Config config, InstanceConfig instanceConfig)
    : d(std::make_unique<InstancePrivate>())
{
    d->config = std::move(config);
    d->instanceConfig = std::move(instanceConfig);
}

Result<std::unique_ptr<Instance>> Instance::fromFile(InstanceConfig instanceConfig)
{
    const Result<Config> config = configFromFile(instanceConfig);
    if (!config)
        return ResultError(config.error());

    return std::make_unique<Instance>(*config, instanceConfig);
}

Result<Config> Instance::configFromFile(InstanceConfig instanceConfig)
{
    const Result<QByteArray> contents = instanceConfig.configFilePath.fileContents();
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
    QString Image;

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
static QDebug operator<<(QDebug debug, const ContainerDetails &details)
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
static QDebug operator<<(QDebug debug, const ImageDetails &details)
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

    QObject::connect(&process, &Process::done, [instanceConfig, context, &process] {
        if (process.error() != ProcessError::UnknownError) {
            const QString line = process.verboseExitMessage();
            if (context.isEmpty())
                instanceConfig.logFunction(line.trimmed());
            else
                instanceConfig.logFunction(QString("[%1] %2").arg(context).arg(line.trimmed()));
        }
    });
}

static QString imageName(const InstanceConfig &instanceConfig)
{
    return QString("qtc-devcontainer-%1").arg(instanceConfig.devContainerId());
}

static QString containerName(const InstanceConfig &instanceConfig)
{
    return imageName(instanceConfig) + "-container";
}

static QString projectName(const InstanceConfig &instanceConfig)
{
    QRegularExpression invalidChars("[^-_a-z0-9]");
    QString fileName = instanceConfig.workspaceFolder.fileName().toLower().remove(invalidChars);
    return imageName(instanceConfig) + "-" + fileName;
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

static QStringList createAppPortArgs(std::variant<int, QString, QList<std::variant<int, QString>>> appPort)
{
    return std::visit(
        overloaded{
            [](int port) { return toAppPortArg(port); },
            [](const QString &port) { return toAppPortArg(port); },
            [](const QList<std::variant<int, QString>> &ports) { return toAppPortArg(ports); }},
        appPort);
}

static ProcessTask checkDocker(const InstanceConfig &instanceConfig)
{
    return ProcessTask([instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Check Docker");
        CommandLine cmdLine{instanceConfig.dockerCli, {"system", "df"}};
        process.setCommand(cmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
    });
}

static ProcessTask findContainerId(
    Storage<QString> containerId,
    const ComposeContainer &composeContainer,
    const InstanceConfig &instanceConfig)
{
    const auto setup = [composeContainer, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Find Container Id");
        CommandLine cmdLine{
            instanceConfig.dockerCli,
            {"ps",
             {"-q", "--no-trunc", "-a"},
             {"--filter", "label=com.docker.compose.project=" + projectName(instanceConfig)},
             {"--filter", "label=com.docker.compose.service=" + composeContainer.service}}};
        process.setCommand(cmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);
    };

    const auto done = [containerId](const Process &process) -> DoneResult {
        const QString output = process.cleanedStdOut().trimmed();
        if (output.isEmpty()) {
            qCWarning(devcontainerlog) << "No container found for compose service.";
            return DoneResult::Error;
        }
        *containerId = output;
        return DoneResult::Success;
    };

    return ProcessTask(setup, done);
}

static ExecutableItem testBuildKit(const InstanceConfig &instanceConfig, Storage<bool> useBuildKit)
{
    return ProcessTask(
        [instanceConfig](Process &process) {
            connectProcessToLog(process, instanceConfig, "Fetch BuildKit Info");
            process.setCommand({instanceConfig.dockerCli, {"buildx", "version"}});
            process.setEnvironment(instanceConfig.localEnvironment);
        },
        [instanceConfig, useBuildKit](const Process &process, DoneWith doneWith) -> DoneResult {
            if (doneWith == DoneWith::Error) {
                // We end up here if buildx is not available.
                return DoneResult::Success;
            }

            // Parse the output and store it in buildKitInfo
            const QString output = process.cleanedStdOut().trimmed();
            const QRegularExpression versionRegex(R"(([0-9]+)\.([0-9]+)\.([0-9]+))");
            const QRegularExpressionMatch match = versionRegex.match(output);
            if (match.hasMatch())
                *useBuildKit = true;

            return DoneResult::Success;
        });
}

static ProcessTask inspectContainerTask(
    Storage<ContainerDetails> containerDetails,
    const InstanceConfig &instanceConfig,
    const DynamicString &identifier)
{
    const auto setupInspectContainer = [identifier, instanceConfig](Process &process) {
        CommandLine inspectCmdLine{
            instanceConfig.dockerCli,
            {"inspect", {"--type", "container"}, dynamicStringToString(identifier)}};

        process.setCommand(inspectCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            Tr::tr("Inspecting container: %1").arg(process.commandLine().toUserOutput()));
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
        details.Image = json.value("Image").toString();

        QJsonObject stateObj = json.value("State").toObject();
        details.State.Status = stateObj.value("Status").toString();
        details.State.StartedAt = stateObj.value("StartedAt").toString();
        details.State.FinishedAt = stateObj.value("FinishedAt").toString();

        QJsonObject configObj = json.value("Config").toObject();
        details.Config.Image = configObj.value("Image").toString();
        details.Config.User = configObj.value("User").toString();

        if (configObj.contains("Env")) {
            const QJsonArray envArray = configObj.value("Env").toArray();
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
            const QJsonArray mountsArray = json["Mounts"].toArray();
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
                    const QJsonArray portBindingsArray = it.value().toArray();
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

static ProcessTask inspectContainerTask(
    Storage<ContainerDetails> containerDetails, const InstanceConfig &instanceConfig)
{
    return inspectContainerTask(containerDetails, instanceConfig, containerName(instanceConfig));
}

static ProcessTask inspectImageTask(
    Storage<ImageDetails> imageDetails,
    const InstanceConfig &instanceConfig,
    const DynamicString &imageName)
{
    const auto setupInspectImage = [imageDetails, instanceConfig, imageName](Process &process) {
        CommandLine inspectCmdLine{
            instanceConfig.dockerCli,
            {"inspect", {"--type", "image"}, dynamicStringToString(imageName)}};

        process.setCommand(inspectCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            Tr::tr("Inspecting image: %1").arg(process.commandLine().toUserOutput()));
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
            const QJsonArray envArray = config.value("Env").toArray();
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
            const QJsonArray entrypointArray = config.value("Entrypoint").toArray();
            details.Config.Entrypoint = QStringList();
            for (const QJsonValue &entryValue : entrypointArray)
                details.Config.Entrypoint->append(entryValue.toString());
        }
        if (config.contains("Cmd")) {
            const QJsonArray cmdArray = config.value("Cmd").toArray();
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

// What this start did to the container, which decides the lifecycle hooks that run.
struct ContainerLifecycle
{
    bool created = false;
    bool started = false;
    // The container and its state before "docker compose up".
    QString previousId;
    QString previousState;
};

struct FeaturesState
{
    struct Pending
    {
        FeatureReference reference;
        QJsonObject options;
    };

    Result<FilePath> contextFolder();
    // Kept apart from the build context, which is sent to the Docker daemon as a whole.
    Result<FilePath> downloadFolder();

    std::unique_ptr<QTemporaryDir> contextDir;
    std::unique_ptr<QTemporaryDir> downloadDir;
    std::vector<Pending> pending;
    QSet<QString> requested;
    std::vector<ResolvedFeature> features;
    int fetchCount = 0;
};

static Result<FilePath> temporaryFolder(std::unique_ptr<QTemporaryDir> &dir)
{
    if (!dir) {
        const QString base = TemporaryDirectory::masterTemporaryDirectory()
                                 ? TemporaryDirectory::masterDirectoryPath()
                                 : QDir::tempPath();
        dir = std::make_unique<QTemporaryDir>(base + "/devcontainer-XXXXXX");
        if (!dir->isValid()) {
            const QString error = dir->errorString();
            dir.reset();
            return ResultError(
                Tr::tr("Cannot create a temporary directory for the build: %1").arg(error));
        }
    }
    return FilePath::fromString(dir->path());
}

Result<FilePath> FeaturesState::contextFolder()
{
    return temporaryFolder(contextDir);
}

Result<FilePath> FeaturesState::downloadFolder()
{
    return temporaryFolder(downloadDir);
}

static void addPendingFeature(
    FeaturesState &state, const FeatureReference &reference, const QJsonObject &options)
{
    const QString key = featureKey(reference, options);
    if (state.requested.contains(key))
        return;
    state.requested.insert(key);
    state.pending.push_back({reference, options});
}

static QString featureFolderName(int index, const QString &id)
{
    static const QRegularExpression invalidChars("[^A-Za-z0-9_.-]");
    return QString("%1_%2").arg(index).arg(QString(id).replace(invalidChars, "_"));
}

static DoneResult readFetchedFeature(
    FeaturesState *state,
    const FeaturesState::Pending &pending,
    const FilePath &folder,
    const InstanceConfig &instanceConfig)
{
    const Result<> result = [&]() -> Result<> {
        const Result<QByteArray> contents = (folder / "devcontainer-feature.json").fileContents();
        if (!contents) {
            return ResultError(
                Tr::tr("The feature \"%1\" has no devcontainer-feature.json: %2")
                    .arg(pending.reference.userReference, contents.error()));
        }

        Result<Feature> feature
            = Feature::fromJson(*contents, [&instanceConfig](const QJsonValue &value) {
                  return instanceConfig.jsonToString(value);
              });
        if (!feature) {
            return ResultError(
                Tr::tr("Cannot read the feature \"%1\": %2")
                    .arg(pending.reference.userReference, feature.error()));
        }

        if (feature->deprecated) {
            instanceConfig.logFunction(
                Tr::tr("The feature \"%1\" is deprecated.").arg(pending.reference.userReference));
        }

        for (const FeatureDependency &dependency : feature->dependsOn) {
            const Result<FeatureReference> reference
                = FeatureReference::parse(dependency.id, instanceConfig.configFilePath.parentDir());
            if (!reference)
                return ResultError(reference.error());
            addPendingFeature(*state, *reference, dependency.options);
        }

        state->features.push_back(
            ResolvedFeature{pending.reference, pending.options, *feature, folder});
        instanceConfig.logFunction(
            Tr::tr("Fetched the feature \"%1\" (%2 %3).")
                .arg(
                    pending.reference.userReference,
                    feature->name.isEmpty() ? feature->id : feature->name,
                    feature->version));
        return ResultOk;
    }();

    if (!result) {
        instanceConfig.logFunction(result.error());
        return DoneResult::Error;
    }
    return DoneResult::Success;
}

static GroupItem fetchFeatureRecipe(
    FeaturesState *state,
    const FeaturesState::Pending &pending,
    const FilePath &contextFolder,
    const FilePath &downloadFolder,
    const InstanceConfig &instanceConfig)
{
    const int index = state->fetchCount++;
    const FilePath folder = contextFolder / "dev-container-features"
                            / featureFolderName(index, pending.reference.id);
    const FilePath archive = downloadFolder / QString("%1.tar").arg(index);

    const auto readFeature = QSyncTask([state, pending, folder, instanceConfig] {
        return readFetchedFeature(state, pending, folder, instanceConfig);
    });

    if (pending.reference.type == FeatureReference::Type::Local) {
        const auto copyLocal = QSyncTask([pending, folder, instanceConfig] {
            instanceConfig.logFunction(
                Tr::tr("Copying the local feature \"%1\".")
                    .arg(pending.reference.localPath.toUserOutput()));
            if (!pending.reference.localPath.isDir()) {
                instanceConfig.logFunction(
                    Tr::tr("The local feature \"%1\" does not exist.")
                        .arg(pending.reference.localPath.toUserOutput()));
                return DoneResult::Error;
            }
            const Result<> result = pending.reference.localPath.copyRecursively(folder);
            if (!result) {
                instanceConfig.logFunction(
                    Tr::tr("Cannot copy the feature \"%1\": %2")
                        .arg(pending.reference.userReference, result.error()));
                return DoneResult::Error;
            }
            return DoneResult::Success;
        });
        return Group{copyLocal, readFeature};
    }

    const RegistryCredentialSource credentialSource
        = pending.reference.type == FeatureReference::Type::Oci
              ? registryCredentialSource(pending.reference.registry, instanceConfig.localEnvironment)
              : RegistryCredentialSource{};
    const Storage<std::optional<RegistryCredentials>> credentials(credentialSource.credentials);
    if (!credentialSource.error.isEmpty())
        instanceConfig.logFunction(credentialSource.error);

    const auto setupHelper = [credentialSource, instanceConfig](Process &process) {
        const FilePath helper = instanceConfig.localEnvironment.searchInPath(
            "docker-credential-" + credentialSource.helper);
        if (helper.isEmpty()) {
            instanceConfig.logFunction(
                Tr::tr("The credential helper \"docker-credential-%1\" is not in the PATH.")
                    .arg(credentialSource.helper));
            return SetupResult::StopWithSuccess;
        }
        process.setCommand({helper, {"get"}});
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWriteData(credentialSource.serverUrl.toUtf8());
        return SetupResult::Continue;
    };

    // Without credentials, the registry may still allow anonymous access.
    const auto doneHelper = [credentials](const Process &process, DoneWith doneWith) {
        if (doneWith != DoneWith::Success)
            return DoneResult::Success;
        const Result<RegistryCredentials> result = parseCredentialHelperOutput(process.rawStdOut());
        if (result)
            *credentials = *result;
        return DoneResult::Success;
    };

    const auto setupDownload =
        [pending, instanceConfig, credentials](Internal::FeatureDownloader &downloader) {
            instanceConfig.logFunction(
                Tr::tr("Downloading the feature \"%1\".").arg(pending.reference.userReference));
            downloader.setReference(pending.reference);
            downloader.setCredentials(*credentials);
            downloader.setCacheFolder(instanceConfig.featureCacheFolder);
            downloader.setLogFunction(instanceConfig.logFunction);
        };

    const auto doneDownload =
        [archive, instanceConfig](const Internal::FeatureDownloader &downloader, DoneWith doneWith)
        -> DoneResult {
        if (doneWith != DoneWith::Success) {
            instanceConfig.logFunction(downloader.errorString());
            return DoneResult::Error;
        }
        const Result<> dir = archive.parentDir().ensureWritableDir();
        const Result<qint64> written = dir ? archive.writeFileContents(downloader.archive())
                                           : Result<qint64>(ResultError(dir.error()));
        if (!written) {
            instanceConfig.logFunction(written.error());
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const auto setupUnarchive = [archive, folder](Unarchiver &unarchiver) {
        unarchiver.setArchive(archive);
        unarchiver.setDestination(folder);
    };

    const auto doneUnarchive = [pending, instanceConfig](const Unarchiver &unarchiver) {
        const Result<> result = unarchiver.result();
        if (!result) {
            instanceConfig.logFunction(
                Tr::tr("Cannot extract the feature \"%1\": %2")
                    .arg(pending.reference.userReference, result.error()));
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    const bool hasHelper = !credentialSource.helper.isEmpty();

    // clang-format off
    return Group {
        credentials,
        If ([hasHelper] { return hasHelper; }) >> Then {
            ProcessTask(setupHelper, doneHelper, CallDoneFlag::Always)
        },
        Internal::FeatureDownloaderTask(setupDownload, doneDownload),
        UnarchiverTask(setupUnarchive, doneUnarchive),
        readFeature
    };
    // clang-format on
}

// Fetches the features the config asks for, and the ones they depend on, and puts them in
// the order they have to be installed in.
static ExecutableItem fetchFeaturesRecipe(
    Storage<FeaturesState> featuresState,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const auto onSetup = [featuresState, commonConfig, instanceConfig] {
        if (commonConfig.features.isEmpty())
            return SetupResult::StopWithSuccess;

        for (const FeatureDependency &dependency : commonConfig.features) {
            const Result<FeatureReference> reference
                = FeatureReference::parse(dependency.id, instanceConfig.configFilePath.parentDir());
            if (!reference) {
                instanceConfig.logFunction(reference.error());
                return SetupResult::StopWithError;
            }
            addPendingFeature(*featuresState, *reference, dependency.options);
        }

        if (const Result<FilePath> folder = featuresState->contextFolder(); !folder) {
            instanceConfig.logFunction(folder.error());
            return SetupResult::StopWithError;
        }
        return SetupResult::Continue;
    };

    // The features a fetched feature depends on are only known once it is fetched, so the
    // fetching goes in rounds until nothing new is asked for.
    const auto setupRound = [featuresState, instanceConfig](QTaskTree &taskTree) {
        FeaturesState *state = featuresState.activeStorage();
        const Result<FilePath> contextFolder = state->contextFolder();
        const Result<FilePath> downloadFolder = state->downloadFolder();
        if (!contextFolder || !downloadFolder) {
            instanceConfig.logFunction(
                contextFolder ? downloadFolder.error() : contextFolder.error());
            return SetupResult::StopWithError;
        }
        const std::vector<FeaturesState::Pending> pending = std::exchange(state->pending, {});

        GroupItems fetches{parallelIdealThreadCountLimit};
        for (const FeaturesState::Pending &p : pending) {
            fetches.append(
                fetchFeatureRecipe(state, p, *contextFolder, *downloadFolder, instanceConfig));
        }
        taskTree.setRecipe(Group(fetches));
        return SetupResult::Continue;
    };

    const auto order = [featuresState, commonConfig, instanceConfig] {
        Result<std::vector<ResolvedFeature>> ordered = featureInstallOrder(
            std::move(featuresState->features), commonConfig.overrideFeatureInstallOrder);
        if (!ordered) {
            instanceConfig.logFunction(ordered.error());
            return DoneResult::Error;
        }
        featuresState->features = std::move(*ordered);

        const QStringList names
            = Utils::transform<QStringList>(featuresState->features, [](const ResolvedFeature &f) {
                  return f.reference.userReference;
              });
        instanceConfig.logFunction(
            Tr::tr("Installing the features in this order: %1").arg(names.join(", ")));
        return DoneResult::Success;
    };

    // clang-format off
    return Group {
        onGroupSetup(onSetup),
        For (UntilIterator([featuresState](qsizetype) { return !featuresState->pending.empty(); })) >> Do {
            QTaskTreeTask(setupRound)
        },
        QSyncTask(order)
    };
    // clang-format on
}

// An empty "remoteUser" is the same as none.
static std::optional<QString> remoteUser(const DevContainerCommon &commonConfig)
{
    if (commonConfig.remoteUser && !commonConfig.remoteUser->isEmpty())
        return commonConfig.remoteUser;
    return std::nullopt;
}

static bool hasFeatures(const Storage<FeaturesState> &featuresState)
{
    return !featuresState->features.empty();
}

static QString userName(const QString &configUser)
{
    const QString name = configUser.section(':', 0, 0);
    return name.isEmpty() ? QString("root") : name;
}

// Builds targetImage from baseImage with the features installed.
static ExecutableItem buildFeaturesImageTask(
    Storage<FeaturesState> featuresState,
    Storage<ImageDetails> baseImageDetails,
    const DynamicString &baseImage,
    const DynamicString &targetImage,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const auto setup = [=](Process &process) {
        connectProcessToLog(process, instanceConfig, Tr::tr("Build Features"));

        const QString imageUser = baseImageDetails->Config.User;
        FeatureBuildContext context;
        context.imageUser = imageUser;
        context.containerUser = commonConfig.containerUser.value_or(userName(imageUser));
        context.remoteUser = remoteUser(commonConfig).value_or(context.containerUser);

        const Result<FilePath> contextFolder = featuresState->contextFolder();
        const Result<FilePath> dockerfile
            = contextFolder
                  ? writeFeaturesBuildContext(featuresState->features, context, *contextFolder)
                  : Result<FilePath>(ResultError(contextFolder.error()));
        if (!dockerfile) {
            instanceConfig.logFunction(
                Tr::tr("Cannot prepare the build of the features: %1").arg(dockerfile.error()));
            return SetupResult::StopWithError;
        }

        // Without the platform of the base image, BuildKit looks for the host's platform, does
        // not accept the local image of another one, and tries to pull it from Docker Hub.
        QString platform = baseImageDetails->Os + '/' + baseImageDetails->Architecture;
        if (baseImageDetails->Variant && !baseImageDetails->Variant->isEmpty())
            platform += '/' + *baseImageDetails->Variant;

        CommandLine buildCmdLine{
            instanceConfig.dockerCli,
            {"build",
             {"-f", dockerfile->nativePath()},
             {"-t", dynamicStringToString(targetImage)},
             {"--platform", platform},
             {"--build-arg", "_DEV_CONTAINERS_BASE_IMAGE=" + dynamicStringToString(baseImage)},
             contextFolder->nativePath()}};

        process.setCommand(buildCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(*contextFolder);
        if (instanceConfig.runProcessesInTerminal)
            process.setTerminalMode(TerminalMode::Run);

        instanceConfig.logFunction(
            Tr::tr("Building the image with the features: %1")
                .arg(process.commandLine().toUserOutput()));
        return SetupResult::Continue;
    };

    return ProcessTask(setup);
}

struct ContainerProperties
{
    QStringList entrypoints;
    std::vector<std::variant<Mount, QString>> mounts;
    QStringList capAdd;
    QStringList securityOpt;
    bool init = false;
    bool privileged = false;
};

// Merges what the features contribute to the container onto what the config asks for.
static ContainerProperties containerProperties(
    const DevContainerCommon &commonConfig, const std::vector<ResolvedFeature> &features)
{
    ContainerProperties properties;
    properties.init = commonConfig.init;
    properties.privileged = commonConfig.privileged;

    for (const ResolvedFeature &f : features) {
        if (!f.feature.entrypoint.isEmpty())
            properties.entrypoints << f.feature.entrypoint;
        properties.mounts
            .insert(properties.mounts.end(), f.feature.mounts.begin(), f.feature.mounts.end());
        properties.capAdd << f.feature.capAdd;
        properties.securityOpt << f.feature.securityOpt;
        properties.init |= f.feature.init;
        properties.privileged |= f.feature.privileged;
    }

    properties.mounts
        .insert(properties.mounts.end(), commonConfig.mounts.begin(), commonConfig.mounts.end());
    properties.mounts = mergeMounts(properties.mounts);
    properties.capAdd << commonConfig.capAdd;
    properties.securityOpt << commonConfig.securityOpt;
    properties.capAdd.removeDuplicates();
    properties.securityOpt.removeDuplicates();
    return properties;
}

static QStringList generateMountArgs(
    const InstanceConfig &instanceConfig, const std::vector<std::variant<Mount, QString>> &mounts)
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

    std::vector<std::variant<Mount, QString>> allMounts = mounts;
    allMounts.insert(allMounts.end(), instanceConfig.mounts.begin(), instanceConfig.mounts.end());
    return Utils::transform<QStringList>(mergeMounts(allMounts), mountToString);
}

template<typename C>
static void setupCreateContainerFromImage(
    const C &containerConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig,
    const ImageDetails &imageDetails,
    const std::vector<ResolvedFeature> &features,
    Process &process)
{
    connectProcessToLog(process, instanceConfig, Tr::tr("Create Container"));

    const ContainerProperties properties = containerProperties(commonConfig, features);

    QStringList containerEnvArgs;

    for (auto &[key, value] : commonConfig.containerEnv)
        containerEnvArgs << "-e" << QString("%1=%2").arg(key, value);

    QStringList appPortArgs;

    if (containerConfig.appPort)
        appPortArgs = createAppPortArgs(*containerConfig.appPort);

    QStringList cmd
        = {"-c",
           QString(R"(echo Container started.
trap "exit 0" TERM
%1
exec "$@"
while sleep 1 & wait $!; do :; done
)")
               .arg(properties.entrypoints.join('\n')),
           "-"};

    if (!containerConfig.overrideCommand) {
        cmd.append(imageDetails.Config.Entrypoint.value_or(QStringList()));
        cmd.append(imageDetails.Config.Cmd.value_or(QStringList()));
    }
    QStringList workspaceMountArgs;

    if (containerConfig.workspaceMount) {
        workspaceMountArgs = {"--mount", *containerConfig.workspaceMount};
    } else {
        workspaceMountArgs
            = {"--mount",
               QString("type=bind,source=%1,target=%2")
                   .arg(
                       instanceConfig.workspaceFolder.path(),
                       containerConfig.workspaceFolder)};
    }

    const auto containerUserArgs = [&commonConfig]() -> QStringList {
        if (commonConfig.containerUser)
            return {"-u", *commonConfig.containerUser};
        return {};
    }();

    const auto featureArgs = [&properties]() -> QStringList {
        QStringList args;
        if (properties.init)
            args << "--init";
        if (properties.privileged)
            args << "--privileged";
        for (const QString &cap : properties.capAdd)
            args << "--cap-add" << cap;
        for (const QString &securityOpt : properties.securityOpt)
            args << "--security-opt" << securityOpt;
        return args;
    }();

    CommandLine createCmdLine{
        instanceConfig.dockerCli,
        {"create",
         {"--name", containerName(instanceConfig)},
         containerEnvArgs,
         containerUserArgs,
         appPortArgs,
         workspaceMountArgs,
         generateMountArgs(instanceConfig, properties.mounts),
         featureArgs,
         {"--entrypoint", "/bin/sh"},
         imageName(instanceConfig),
         cmd}};
    process.setCommand(createCmdLine);
    process.setEnvironment(instanceConfig.localEnvironment);
    process.setWorkingDirectory(instanceConfig.workspaceFolder);

    instanceConfig.logFunction(
        Tr::tr("Creating container: %1").arg(process.commandLine().toUserOutput()));
}

static QString anyOf(QJsonObject obj, QStringList keyNames)
{
    for (const QString &key : keyNames) {
        if (obj.contains(key))
            return obj.value(key).toString();
    }
    return QString();
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
        process.setEnvironment(instanceConfig.localEnvironment);

        instanceConfig.logFunction(
            Tr::tr("Waiting for container to start: %1").arg(eventsCmdLine.toUserOutput()));

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
                if (anyOf(event, {"status", "Status", "Action"}) == eventType) {
                    qCDebug(devcontainerlog) << "Container started!";
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

static QStringList remoteUserArgs(const DevContainerCommon &commonConfig)
{
    if (const std::optional<QString> user = remoteUser(commonConfig))
        return {"-u", *user};
    return {};
}

static ExecutableItem execInContainerTask(
    const QString &logPrefix,
    const InstanceConfig &instanceConfig,
    const DynamicString &containerId,
    const QStringList &execArgs,
    const std::variant<std::function<CommandLine()>, CommandLine, QString> &cmdLine,
    const ProcessTask::TaskDoneHandler &doneHandler)
{
    const auto setupExec = [instanceConfig, containerId, execArgs, cmdLine, logPrefix](
                               Process &process) {
        connectProcessToLog(process, instanceConfig, logPrefix);

        CommandLine execCmdLine{
            instanceConfig.dockerCli, {"exec", execArgs, dynamicStringToString(containerId)}};
        if (std::holds_alternative<CommandLine>(cmdLine)) {
            execCmdLine.addCommandLineAsArgs(std::get<CommandLine>(cmdLine));
        } else if (std::holds_alternative<QString>(cmdLine)) {
            execCmdLine.addArgs({std::get<QString>(cmdLine)}, CommandLine::Raw);
        } else if (std::holds_alternative<std::function<CommandLine()>>(cmdLine)) {
            const CommandLine cmd = std::get<std::function<CommandLine()>>(cmdLine)();
            if (cmd.isEmpty()) {
                qCWarning(devcontainerlog)
                    << "Empty command provided for execInContainerTask." << cmd.toUserOutput();
                return;
            }
            execCmdLine.addCommandLineAsArgs(cmd);
        } else {
            qCWarning(devcontainerlog) << "Unsupported command line type for execInContainerTask.";
            return;
        }

        process.setCommand(execCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            Tr::tr("Executing in container: %1").arg(process.commandLine().toUserOutput()));
    };

    return ProcessTask{setupExec, doneHandler};
}

static ExecutableItem probeUserEnvTask(
    Storage<RunningContainerDetails> containerDetails,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig,
    const DynamicString &containerId)
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
        containerId,
        remoteUserArgs(commonConfig),
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

            Environment env(output.split('\n', Qt::SkipEmptyParts), Utils::OsTypeLinux);

            // We don't want to capture the following environment variables:
            for (const char *key : {"_", "PWD"})
                env.unset(QLatin1StringView(key));

            containerDetails->probedUserEnvironment = env;

            return DoneResult::Success;
        });
}

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
    const InstanceConfig &instanceConfig,
    const DynamicString &containerId)
{
    const ExecutableItem idTask = execInContainerTask(
        "Get Running Container User",
        instanceConfig,
        containerId,
        remoteUserArgs(commonConfig),
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
        containerId,
        {},
        [containerDetails, commonConfig]() -> CommandLine {
            const QString userName
                = remoteUser(commonConfig).value_or(containerUser(*containerDetails));
            QString userEscapedForGrep = userName;
            userEscapedForGrep.replace(QRegularExpression("([.*+?^${}()|[\\]\\\\])"), "\\\\1");

            // The script runs in the shell of the container, so it is quoted for Linux whatever
            // the host is.
            const QString script
                = QString("command -v getent >/dev/null 2>&1 && getent passwd %1"
                          " || grep -E %2 /etc/passwd || true")
                      .arg(
                          ProcessArgs::quoteArg(userName, OsTypeLinux),
                          ProcessArgs::quoteArg(
                              QString("^(%1:|[^:]*:[^:]*:%1:)").arg(userEscapedForGrep),
                              OsTypeLinux));
            return CommandLine{"/bin/sh", {"-c", script}};
        },
        [instanceConfig,
         containerDetails,
         runningDetails](const Process &process, DoneWith doneWith) -> DoneResult {
            const QString output = process.cleanedStdOut().trimmed();

            runningDetails->userShell = containerDetails->Config.Env.value("SHELL", "/bin/sh");
            instanceConfig.logFunction(
                "Running container user shell (default): " + runningDetails->userShell);

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

            instanceConfig.logFunction(
                QString("Running container user: %1 UID: %2 GID: %3 Home: %4 Shell: %5")
                    .arg(user->name, user->uid, user->gid, user->home, user->shell));

            runningDetails->userShell = user->shell;

            return DoneResult::Success;
        });

    return Group{
        idTask,
        shellTask,
        probeUserEnvTask(runningDetails, commonConfig, instanceConfig, containerId)};
}

static ProcessTask lifecycleHookTask(
    const CommandLine &cmdLine, const InstanceConfig &instanceConfig, const QString &name)
{
    return ProcessTask([cmdLine, instanceConfig, name](Process &process) {
        connectProcessToLog(process, instanceConfig, name);
        process.setCommand(cmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
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
    std::optional<CommandLine> dockerExecCmd = std::nullopt,
    const QString &source = {})
{
    if (!command)
        return Group{};

    const QString from = source.isEmpty() ? instanceConfig.configFilePath.fileName() : source;
    auto logExecution = QSyncTask([instanceConfig, hookName, from] {
        instanceConfig.logFunction(QString("Executing the %1 hook from %2").arg(hookName, from));
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

static ExecutableItem initializeCommandRecipe(
    const DevContainerCommon &commonConfig, const InstanceConfig &instanceConfig)
{
    return lifecycleHookRecipe("initializeCommand", instanceConfig, commonConfig.initializeCommand);
}

// Runs the hooks inside the container. The hooks of the features run first, in the order the
// features were installed in.
static ExecutableItem runLifecycleHooksRecipe(
    Storage<FeaturesState> featuresState,
    Storage<ContainerLifecycle> lifecycle,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig,
    const DynamicString &containerId)
{
    const auto setupHooks =
        [featuresState, lifecycle, commonConfig, instanceConfig, containerId](QTaskTree &taskTree) {
            const CommandLine dockerExecPrefix{
                instanceConfig.dockerCli,
                {"exec", remoteUserArgs(commonConfig), dynamicStringToString(containerId)}};

            // The commands of the creation run once, postStartCommand on each start of the
            // container, and postAttachCommand each time Qt Creator connects to it.
            struct Hook
            {
                QString name;
                std::optional<Command> Feature::*featureCommand;
                std::optional<Command> DevContainerCommon::*command;
                bool run;
                QString skipMessage;
            };
            const bool created = lifecycle->created;
            const bool started = lifecycle->started || created;
            const QString existed = Tr::tr("Skipping %1, as the container existed already.");
            const QString running = Tr::tr("Skipping %1, as the container was running already.");
            const QList<Hook> hooks
                = {{"onCreateCommand",
                    &Feature::onCreateCommand,
                    &DevContainerCommon::onCreateCommand,
                    created,
                    existed},
                   {"updateContentCommand",
                    &Feature::updateContentCommand,
                    &DevContainerCommon::updateContentCommand,
                    created,
                    existed},
                   {"postCreateCommand",
                    &Feature::postCreateCommand,
                    &DevContainerCommon::postCreateCommand,
                    created,
                    existed},
                   {"postStartCommand",
                    &Feature::postStartCommand,
                    &DevContainerCommon::postStartCommand,
                    started,
                    running},
                   {"postAttachCommand",
                    &Feature::postAttachCommand,
                    &DevContainerCommon::postAttachCommand,
                    true,
                    {}}};

            GroupItems items;
            for (const Hook &hook : hooks) {
                bool hasCommand = bool(commonConfig.*hook.command);
                if (!hasCommand) {
                    for (const ResolvedFeature &feature : featuresState->features) {
                        if (feature.feature.*hook.featureCommand) {
                            hasCommand = true;
                            break;
                        }
                    }
                }

                if (!hook.run) {
                    if (hasCommand)
                        instanceConfig.logFunction(hook.skipMessage.arg(hook.name));
                    continue;
                }
                for (const ResolvedFeature &feature : featuresState->features) {
                    items.append(lifecycleHookRecipe(
                        hook.name,
                        instanceConfig,
                        feature.feature.*hook.featureCommand,
                        dockerExecPrefix,
                        feature.reference.userReference));
                }
                items.append(lifecycleHookRecipe(
                    hook.name, instanceConfig, commonConfig.*hook.command, dockerExecPrefix));
            }
            taskTree.setRecipe(Group(items));
        };

    return QTaskTreeTask(setupHooks);
}

static ExecutableItem containerDoesNotExistTask(const InstanceConfig &instanceConfig)
{
    return ProcessTask(
        [instanceConfig](Process &process) {
            instanceConfig.logFunction(
                Tr::tr("Checking if container exists: %1").arg(containerName(instanceConfig)));

            connectProcessToLog(process, instanceConfig, "Check Container Existence");

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
            process.setEnvironment(instanceConfig.localEnvironment);
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
                instanceConfig.logFunction(Tr::tr("Container already exists: %1").arg(output));
                return DoneResult::Error;
            }

            instanceConfig.logFunction(
                Tr::tr("Container does not exist, proceeding to create: %1").arg(output));

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
            process.setEnvironment(instanceConfig.localEnvironment);
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
    Storage<FeaturesState> featuresState,
    Storage<ContainerLifecycle> lifecycle,
    const C &containerConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    auto createContainerSetup =
        [imageDetails, featuresState, containerConfig, commonConfig, instanceConfig](
            Process &process) {
            setupCreateContainerFromImage(
                containerConfig,
                commonConfig,
                instanceConfig,
                *imageDetails,
                featuresState->features,
                process);
        };

    // clang-format off
    return Group {
        If (containerDoesNotExistTask(instanceConfig)) >> Then {
            ProcessTask(createContainerSetup, [lifecycle] { lifecycle->created = true; },
                        CallDoneFlag::OnSuccess)
        }
    };
    // clang-format on
}

static ExecutableItem startContainerRecipe(
    const InstanceConfig &instanceConfig, Storage<ContainerLifecycle> lifecycle)
{
    const auto onStarted = [lifecycle] { lifecycle->started = true; };

    const auto start = [instanceConfig, onStarted] {
        return ProcessTask(
            [instanceConfig](Process &process) {
                connectProcessToLog(process, instanceConfig, Tr::tr("Start Container"));

                CommandLine
                    startCmdLine{instanceConfig.dockerCli, {"start", containerName(instanceConfig)}};
                process.setCommand(startCmdLine);
                process.setEnvironment(instanceConfig.localEnvironment);
                process.setWorkingDirectory(instanceConfig.workspaceFolder);

                instanceConfig.logFunction(
                    Tr::tr("Starting container: %1").arg(process.commandLine().toUserOutput()));
            },
            onStarted,
            CallDoneFlag::OnSuccess);
    };

    const auto unpause = [instanceConfig, onStarted] {
        return ProcessTask(
            [instanceConfig](Process &process) {
                connectProcessToLog(process, instanceConfig, Tr::tr("Resume Container"));

                CommandLine startCmdLine{
                    instanceConfig.dockerCli, {"unpause", containerName(instanceConfig)}};
                process.setCommand(startCmdLine);
                process.setEnvironment(instanceConfig.localEnvironment);
                process.setWorkingDirectory(instanceConfig.workspaceFolder);

                instanceConfig.logFunction(
                    Tr::tr("Resuming container: %1").arg(process.commandLine().toUserOutput()));
            },
            onStarted,
            CallDoneFlag::OnSuccess);
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

// The container is set up and used with the shell and tools of Linux.
static QSyncTask requireLinuxImage(
    const Storage<ImageDetails> &imageDetails, const InstanceConfig &instanceConfig)
{
    return QSyncTask([imageDetails, instanceConfig] {
        const Result<OsType> osType = osTypeFromString(imageDetails->Os);
        if (osType && *osType == OsTypeLinux)
            return DoneResult::Success;
        instanceConfig.logFunction(
            Tr::tr("The image is for \"%1\", but dev containers need a Linux image.")
                .arg(imageDetails->Os));
        return DoneResult::Error;
    });
}

static QSyncTask fillRunningInstance(
    const RunningInstance &runningInstance,
    const Storage<RunningContainerDetails> &runningDetails,
    const Storage<ImageDetails> &imageDetails,
    const Storage<FeaturesState> &featuresState,
    const DevContainerCommon &commonConfig,
    const DynamicString &containerId)
{
    const QJsonObject customizations = commonConfig.customizations;
    return QSyncTask([=]() {
        runningInstance->remoteEnvironment = runningDetails->probedUserEnvironment;
        runningInstance->customizations
            = mergedCustomizations(featuresState->features, customizations);

        runningInstance->osType = osTypeFromString(imageDetails->Os).value_or(OsType::OsTypeOther);
        runningInstance->osArch
            = osArchFromString(imageDetails->Architecture).value_or(OsArch::OsArchUnknown);
        runningInstance->containerId = dynamicStringToString(containerId);
    });
}

static QString baseImageName(const InstanceConfig &instanceConfig)
{
    return imageName(instanceConfig) + "-base";
}

static ProcessTask pullImageTask(const DynamicString &image, const InstanceConfig &instanceConfig)
{
    return ProcessTask([image, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Pull Image");

        CommandLine pullCmdLine{instanceConfig.dockerCli, {"pull", dynamicStringToString(image)}};
        process.setCommand(pullCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Pulling Image: %1").arg(process.commandLine().toUserOutput()));
    });
}

// Makes sure the image exists locally, and fills imageDetails with it.
static ExecutableItem ensureImageRecipe(
    Storage<ImageDetails> imageDetails,
    const DynamicString &image,
    const InstanceConfig &instanceConfig)
{
    return Group{
        stopOnSuccess,
        inspectImageTask(imageDetails, instanceConfig, image),
        Group{
            pullImageTask(image, instanceConfig),
            inspectImageTask(imageDetails, instanceConfig, image)}};
}

static QString featuresImageName(const InstanceConfig &instanceConfig)
{
    return imageName(instanceConfig) + "-features";
}

// The user whose UID and GID are changed to the ones of the local user, if any.
static std::optional<QString> uidUpdateUser(
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig,
    const QString &imageUser)
{
    if (!instanceConfig.localUser || !commonConfig.updateRemoteUserUID.value_or(true))
        return std::nullopt;

    const QString user = remoteUser(commonConfig)
                             .value_or(commonConfig.containerUser.value_or(userName(imageUser)));
    static const QRegularExpression numeric("^[0-9]+$");
    if (user.isEmpty() || user == "root" || numeric.match(user).hasMatch())
        return std::nullopt;
    return user;
}

static ProcessTask updateUidTask(
    Storage<FeaturesState> featuresState,
    Storage<ImageDetails> imageDetails,
    Storage<std::optional<QString>> user,
    const DynamicString &baseImage,
    const InstanceConfig &instanceConfig)
{
    const auto setup = [=](Process &process) {
        connectProcessToLog(process, instanceConfig, Tr::tr("Update UID"));

        const Result<FilePath> contextFolder = featuresState->contextFolder();
        const FilePath folder = contextFolder ? *contextFolder / "update-uid" : FilePath();
        const Result<> dir = contextFolder ? folder.ensureWritableDir()
                                           : Result<>(ResultError(contextFolder.error()));
        const Result<qint64> written
            = dir ? (folder / "Dockerfile").writeFileContents(updateUidDockerfile().toUtf8())
                  : Result<qint64>(ResultError(dir.error()));
        if (!written) {
            instanceConfig.logFunction(
                Tr::tr("Cannot prepare the update of the user ID: %1").arg(written.error()));
            return SetupResult::StopWithError;
        }

        const QString imageUser = imageDetails->Config.User.isEmpty() ? QString("root")
                                                                      : imageDetails->Config.User;
        const LocalUser localUser = *instanceConfig.localUser;

        CommandLine buildCmdLine{
            instanceConfig.dockerCli,
            {"build",
             {"-f", (folder / "Dockerfile").nativePath()},
             {"-t", imageName(instanceConfig)},
             {"--build-arg", "_DEV_CONTAINERS_BASE_IMAGE=" + dynamicStringToString(baseImage)},
             {"--build-arg", "REMOTE_USER=" + **user},
             {"--build-arg", QString("NEW_UID=%1").arg(localUser.uid)},
             {"--build-arg", QString("NEW_GID=%1").arg(localUser.gid)},
             {"--build-arg", "IMAGE_USER=" + imageUser},
             folder.nativePath()}};

        process.setCommand(buildCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(folder);

        instanceConfig.logFunction(
            Tr::tr("Changing the user ID of \"%1\" to %2:%3: %4")
                .arg(**user)
                .arg(localUser.uid)
                .arg(localUser.gid)
                .arg(process.commandLine().toUserOutput()));
        return SetupResult::Continue;
    };

    return ProcessTask(setup);
}

static ProcessTask tagImageTask(const DynamicString &image, const InstanceConfig &instanceConfig)
{
    return ProcessTask([image, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Tag Image");

        CommandLine tagCmdLine{
            instanceConfig.dockerCli,
            {"tag", dynamicStringToString(image), imageName(instanceConfig)}};
        process.setCommand(tagCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Tagging Image: %1").arg(process.commandLine().toUserOutput()));
    });
}

// Adds the features and the update of the user ID to baseImage, whose details imageDetails
// holds, and leaves the result as imageName(), with its details in imageDetails.
static ExecutableItem finalizeImageRecipe(
    Storage<FeaturesState> featuresState,
    Storage<ImageDetails> imageDetails,
    const DynamicString &baseImage,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const Storage<QString> currentImage;
    const Storage<std::optional<QString>> uidUser;

    const auto init = [=] {
        *currentImage = dynamicStringToString(baseImage);
        *uidUser = uidUpdateUser(commonConfig, instanceConfig, imageDetails->Config.User);
    };

    const DynamicString current = std::function<QString()>([currentImage] { return *currentImage; });
    const DynamicString featuresTarget = std::function<QString()>([uidUser, instanceConfig] {
        return *uidUser ? featuresImageName(instanceConfig) : imageName(instanceConfig);
    });
    const auto afterFeatures = [currentImage, featuresTarget] {
        *currentImage = dynamicStringToString(featuresTarget);
    };
    const auto needsUidUpdate = [uidUser] { return bool(*uidUser); };
    const auto needsTag = [currentImage, instanceConfig] {
        return *currentImage != imageName(instanceConfig);
    };

    // clang-format off
    return Group {
        currentImage,
        uidUser,
        QSyncTask(init),
        If ([featuresState] { return hasFeatures(featuresState); }) >> Then {
            buildFeaturesImageTask(featuresState, imageDetails, current, featuresTarget,
                                   commonConfig, instanceConfig),
            QSyncTask(afterFeatures)
        },
        If (needsUidUpdate) >> Then {
            updateUidTask(featuresState, imageDetails, uidUser, current, instanceConfig)
        } >> ElseIf (needsTag) >> Then {
            tagImageTask(current, instanceConfig)
        },
        inspectImageTask(imageDetails, instanceConfig, imageName(instanceConfig))
    };
    // clang-format on
}

static Result<Group> prepareContainerRecipe(
    const DockerfileContainer &containerConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig,
    const RunningInstance &runningInstance)
{
    Storage<FeaturesState> featuresState;

    const auto setupBuildImage = [containerConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, Tr::tr("Build Dockerfile"));

        const FilePath configFileDir = instanceConfig.configFilePath.parentDir();
        const FilePath contextPath = configFileDir.resolvePath(containerConfig.context);
        const FilePath dockerFile = configFileDir.resolvePath(containerConfig.dockerfile);
        const BuildOptions buildOptions = containerConfig.buildOptions.value_or(BuildOptions{});

        const QStringList cacheFromArgs = [&] {
            if (!buildOptions.cacheFrom)
                return QStringList{};

            return std::visit(
                overloaded{
                    [](const QString &cacheFrom) { return QStringList{"--cache-from", cacheFrom}; },
                    [](const QStringList &cacheFroms) {
                        return Utils::transform<QStringList>(cacheFroms, [](const QString &cf) {
                            return QString("--cache-from=%1").arg(cf);
                        });
                    }},
                *buildOptions.cacheFrom);
        }();

        const QStringList target = [&] {
            if (!buildOptions.target)
                return QStringList{};
            return QStringList{"--target", *buildOptions.target};
        }();

        const QStringList extraBuildArgs = [&] {
            QStringList args;
            for (const auto &[k, v] : buildOptions.args) {
                if (v.isEmpty())
                    args << QStringList{"--build-arg", k};
                else
                    args << QStringList{"--build-arg", QString("%1=%2").arg(k, v)};
            }
            return args;
        }();

        // The image of the Dockerfile is the base the features and the user ID update are
        // added to.
        const QString tag = baseImageName(instanceConfig);

        CommandLine buildCmdLine{
            instanceConfig.dockerCli,
            {"build",
             {"-f", dockerFile.nativePath()},
             {"-t", tag},
             buildOptions.options,
             cacheFromArgs,
             target,
             extraBuildArgs,
             contextPath.nativePath()}};

        process.setCommand(buildCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);
        if (instanceConfig.runProcessesInTerminal)
            process.setTerminalMode(TerminalMode::Run);

        instanceConfig.logFunction(
            Tr::tr("Building Dockerfile: %1").arg(process.commandLine().toUserOutput()));
    };

    Storage<ImageDetails> imageDetails;
    Storage<ContainerDetails> containerDetails;
    Storage<RunningContainerDetails> runningDetails;
    Storage<ContainerLifecycle> lifecycle;
    Storage<bool> useBuildKit(false);

    // clang-format off
    return Group {
        imageDetails,
        runningDetails,
        containerDetails,
        useBuildKit,
        featuresState,
        lifecycle,
        checkDocker(instanceConfig),
        testBuildKit(instanceConfig, useBuildKit),
        initializeCommandRecipe(commonConfig, instanceConfig),
        fetchFeaturesRecipe(featuresState, commonConfig, instanceConfig),
        ProcessTask(setupBuildImage),
        inspectImageTask(imageDetails, instanceConfig, baseImageName(instanceConfig)),
        requireLinuxImage(imageDetails, instanceConfig),
        finalizeImageRecipe(featuresState, imageDetails, baseImageName(instanceConfig),
                            commonConfig, instanceConfig),
        createContainerRecipe(
            imageDetails, featuresState, lifecycle, containerConfig, commonConfig, instanceConfig),
        inspectContainerTask(containerDetails, instanceConfig),
        startContainerRecipe(instanceConfig, lifecycle),
        runningContainerDetailsTask(containerDetails, runningDetails, commonConfig, instanceConfig, containerName(instanceConfig)),
        runLifecycleHooksRecipe(featuresState, lifecycle, commonConfig, instanceConfig, containerName(instanceConfig)),
        fillRunningInstance(runningInstance, runningDetails, imageDetails, featuresState, commonConfig, containerName(instanceConfig))
    };
    // clang-format on
}

static Result<Group> prepareContainerRecipe(
    const ImageContainer &imageConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig,
    const RunningInstance &runningInstance)
{
    Storage<ImageDetails> imageDetails;
    Storage<ContainerDetails> containerDetails;
    Storage<RunningContainerDetails> runningDetails;
    Storage<FeaturesState> featuresState;
    Storage<ContainerLifecycle> lifecycle;
    Storage<bool> useBuildKit(false);

    // clang-format off
    return Group {
        imageDetails,
        containerDetails,
        runningDetails,
        featuresState,
        lifecycle,
        useBuildKit,
        checkDocker(instanceConfig),
        testBuildKit(instanceConfig, useBuildKit),
        initializeCommandRecipe(commonConfig, instanceConfig),
        fetchFeaturesRecipe(featuresState, commonConfig, instanceConfig),
        ensureImageRecipe(imageDetails, imageConfig.image, instanceConfig),
        requireLinuxImage(imageDetails, instanceConfig),
        finalizeImageRecipe(featuresState, imageDetails, imageConfig.image, commonConfig, instanceConfig),
        createContainerRecipe(imageDetails, featuresState, lifecycle, imageConfig, commonConfig, instanceConfig),
        inspectContainerTask(containerDetails, instanceConfig),
        startContainerRecipe(instanceConfig, lifecycle),
        runningContainerDetailsTask(containerDetails, runningDetails, commonConfig, instanceConfig, containerName(instanceConfig)),
        runLifecycleHooksRecipe(featuresState, lifecycle, commonConfig, instanceConfig, containerName(instanceConfig)),
        fillRunningInstance(runningInstance, runningDetails, imageDetails, featuresState, commonConfig, containerName(instanceConfig)),
    };
    // clang-format on
}

static QStringList composeFileArgs(
    const ComposeContainer &config, const InstanceConfig &instanceConfig)
{
    const FilePath configFileDir = instanceConfig.configFilePath.parentDir();
    QStringList args;
    for (const QString &file : config.dockerComposeFiles)
        args << "-f" << configFileDir.resolvePath(file).nativePath();
    return args;
}

struct ComposeServiceInfo
{
    // The image the service runs, before the features are added.
    QString image;
    QString user;
    // The started services with a "build" section, including the ones they depend on.
    QStringList servicesToBuild;
    // Whether the service runs an image built for the dev container.
    bool customImage = false;
    std::optional<QStringList> entrypoint;
    std::optional<QStringList> command;
};

static std::optional<QStringList> stringListFromJson(const QJsonValue &value)
{
    if (value.isArray())
        return Utils::transform<QStringList>(value.toArray().toVariantList(), &QVariant::toString);
    if (value.isString())
        return QStringList{"/bin/sh", "-c", value.toString()};
    return std::nullopt;
}

static ProcessTask composeServiceInfoTask(
    Storage<ComposeServiceInfo> serviceInfo,
    const ComposeContainer &config,
    const InstanceConfig &instanceConfig)
{
    const auto setup = [config, instanceConfig](Process &process) {
        CommandLine cmdLine{
            instanceConfig.dockerCli,
            {"compose",
             composeFileArgs(config, instanceConfig),
             {"--project-name", projectName(instanceConfig)},
             "config",
             {"--format", "json"}}};
        process.setCommand(cmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.configFilePath.parentDir());

        instanceConfig.logFunction(
            QString("Compose Config: %1").arg(process.commandLine().toUserOutput()));
    };

    const auto done = [serviceInfo,
                       config,
                       instanceConfig](const Process &process, DoneWith doneWith) -> DoneResult {
        if (doneWith != DoneWith::Success) {
            instanceConfig.logFunction(process.verboseExitMessage());
            instanceConfig.logFunction(process.cleanedStdErr());
            return DoneResult::Error;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(process.rawStdOut(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            instanceConfig.logFunction(
                Tr::tr("Cannot read the configuration from \"docker compose config\": %1")
                    .arg(
                        parseError.error != QJsonParseError::NoError
                            ? parseError.errorString()
                            : Tr::tr("The configuration is not a JSON object.")));
            return DoneResult::Error;
        }

        const QJsonObject services = doc.object().value("services").toObject();
        const QJsonObject service = services.value(config.service).toObject();
        if (service.isEmpty()) {
            instanceConfig.logFunction(
                Tr::tr("The compose files do not contain the service \"%1\".").arg(config.service));
            return DoneResult::Error;
        }

        QStringList pending = QStringList{config.service}
                              + config.runServices.value_or(QStringList());
        QSet<QString> seen;
        serviceInfo->servicesToBuild.clear();
        while (!pending.isEmpty()) {
            const QString name = pending.takeFirst();
            if (seen.contains(name))
                continue;
            seen.insert(name);
            const QJsonObject s = services.value(name).toObject();
            if (s.contains("build"))
                serviceInfo->servicesToBuild << name;
            const QJsonValue dependsOn = s.value("depends_on");
            if (dependsOn.isObject())
                pending << dependsOn.toObject().keys();
            else
                pending << Utils::transform<QStringList>(
                    dependsOn.toArray().toVariantList(), &QVariant::toString);
        }
        // Without an image name, compose names the built image after project and service.
        serviceInfo->image = service.value("image").toString(
            projectName(instanceConfig) + '-' + config.service);
        serviceInfo->user = service.value("user").toString();
        serviceInfo->entrypoint = stringListFromJson(service.value("entrypoint"));
        serviceInfo->command = stringListFromJson(service.value("command"));
        return DoneResult::Success;
    };

    return ProcessTask(setup, done);
}

// Builds without the override file, so the service keeps the name of its base image.
static ProcessTask composeBuildTask(
    Storage<ComposeServiceInfo> serviceInfo,
    const ComposeContainer &config,
    const InstanceConfig &instanceConfig)
{
    return ProcessTask([serviceInfo, config, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Compose Build");
        CommandLine cmdLine{
            instanceConfig.dockerCli,
            {"compose",
             composeFileArgs(config, instanceConfig),
             {"--project-name", projectName(instanceConfig)},
             "build",
             serviceInfo->servicesToBuild}};
        process.setCommand(cmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.configFilePath.parentDir());
        if (instanceConfig.runProcessesInTerminal)
            process.setTerminalMode(TerminalMode::Run);

        instanceConfig.logFunction(
            QString("Compose Build: %1").arg(process.commandLine().toUserOutput()));
    });
}

// Turns a mount into the long syntax of a compose service volume.
static QJsonObject composeVolume(const std::variant<Mount, QString> &mount)
{
    return std::visit(
        overloaded{
            [](const Mount &m) {
                QJsonObject volume{
                    {"type", m.type == MountType::Bind ? QString("bind") : QString("volume")},
                    {"target", m.target}};
                if (m.source)
                    volume.insert("source", *m.source);
                return volume;
            },
            [](const QString &m) {
                QJsonObject volume{{"type", "volume"}};
                for (const QString &part : m.split(',', Qt::SkipEmptyParts)) {
                    const auto [key, value] = Utils::splitAtFirst(part, '=');
                    const QString k = key.trimmed().toString();
                    const QString v = value.trimmed().toString();
                    if (k == "type")
                        volume.insert("type", v);
                    else if (k == "source" || k == "src")
                        volume.insert("source", v);
                    else if (k == "target" || k == "destination" || k == "dst")
                        volume.insert("target", v);
                    else if ((k == "readonly" || k == "ro") && v != "false")
                        volume.insert("read_only", true);
                }
                return volume;
            }},
        mount);
}

static QJsonValue escapeComposeInterpolation(const QJsonValue &value)
{
    if (value.isString())
        return QString(value.toString()).replace('$', "$$");
    if (value.isArray()) {
        QJsonArray array;
        for (const QJsonValue &item : value.toArray())
            array.append(escapeComposeInterpolation(item));
        return array;
    }
    if (value.isObject()) {
        QJsonObject object = value.toObject();
        for (auto it = object.begin(); it != object.end(); ++it)
            it.value() = escapeComposeInterpolation(it.value());
        return object;
    }
    return value;
}

// Writes the compose file that adds what the config and the features ask for to the service.
static Result<FilePath> writeComposeOverride(
    FeaturesState &featuresState,
    const ComposeServiceInfo &serviceInfo,
    const ImageDetails &baseImage,
    const ComposeContainer &config,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const ContainerProperties properties = containerProperties(commonConfig, featuresState.features);

    QJsonObject service;
    QJsonObject volumes;

    if (serviceInfo.customImage)
        service.insert("image", imageName(instanceConfig));

    if (!properties.entrypoints.isEmpty()) {
        const QString script = QString(R"(echo Container started.
trap "exit 0" TERM
%1
exec "$@"
while sleep 1 & wait $!; do :; done
)")
                                   .arg(properties.entrypoints.join('\n'));
        service.insert("entrypoint", QJsonArray{"/bin/sh", "-c", script, "-"});
        // An entrypoint of the service replaces both the ENTRYPOINT and the CMD of the image.
        const QStringList command = serviceInfo.entrypoint
                                        ? *serviceInfo.entrypoint
                                              + serviceInfo.command.value_or(QStringList())
                                        : baseImage.Config.Entrypoint.value_or(QStringList())
                                              + serviceInfo.command.value_or(
                                                  baseImage.Config.Cmd.value_or(QStringList()));
        service.insert("command", QJsonArray::fromStringList(command));
    }

    QJsonObject environment;
    for (const auto &[key, value] : commonConfig.containerEnv)
        environment.insert(key, value);
    if (!environment.isEmpty())
        service.insert("environment", environment);

    if (properties.init)
        service.insert("init", true);
    if (properties.privileged)
        service.insert("privileged", true);
    if (!properties.capAdd.isEmpty())
        service.insert("cap_add", QJsonArray::fromStringList(properties.capAdd));
    if (!properties.securityOpt.isEmpty())
        service.insert("security_opt", QJsonArray::fromStringList(properties.securityOpt));

    std::vector<std::variant<Mount, QString>> mounts = properties.mounts;
    mounts.insert(mounts.end(), instanceConfig.mounts.begin(), instanceConfig.mounts.end());
    QJsonArray serviceVolumes;
    for (const auto &mount : mergeMounts(mounts)) {
        const QJsonObject volume = composeVolume(mount);
        serviceVolumes.append(volume);
        const QString source = volume.value("source").toString();
        if (volume.value("type").toString() == "volume" && !source.isEmpty())
            volumes.insert(source, QJsonObject{{"name", source}});
    }
    if (!serviceVolumes.isEmpty())
        service.insert("volumes", serviceVolumes);

    if (service.isEmpty())
        return FilePath();

    QJsonObject compose{{"services", QJsonObject{{config.service, service}}}};
    if (!volumes.isEmpty())
        compose.insert("volumes", volumes);

    const Result<FilePath> folder = featuresState.contextFolder();
    if (!folder)
        return ResultError(folder.error());

    // JSON is YAML, so compose reads it as it is. Compose fills in "$VAR" from the host
    // environment in every file, so the values are escaped to arrive as they are written, as
    // they do with "docker run".
    compose = escapeComposeInterpolation(compose).toObject();
    const FilePath file = *folder / "docker-compose.devcontainer.json";
    if (const Result<qint64> res = file.writeFileContents(QJsonDocument(compose).toJson()); !res)
        return ResultError(res.error());

    instanceConfig.logFunction(
        QString("Compose override: %1")
            .arg(QString::fromUtf8(QJsonDocument(compose).toJson(QJsonDocument::Compact))));
    return file;
}

static Result<Group> prepareContainerRecipe(
    const ComposeContainer &config,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig,
    const RunningInstance &runningInstance)
{
    Storage<ContainerDetails> containerDetails;
    Storage<RunningContainerDetails> runningDetails;
    Storage<QString> containerId;
    Storage<ImageDetails> imageDetails;
    Storage<FeaturesState> featuresState;
    Storage<ComposeServiceInfo> serviceInfo;
    Storage<FilePath> overrideFile;
    Storage<ContainerLifecycle> lifecycle;
    Storage<bool> useBuildKit(false);

    const auto setupComposeUp = [config, instanceConfig, serviceInfo, overrideFile](
                                    Process &process) {
        connectProcessToLog(process, instanceConfig, "Compose Up");

        QStringList composeFiles = composeFileArgs(config, instanceConfig);
        if (!overrideFile->isEmpty())
            composeFiles << "-f" << overrideFile->nativePath();

        QStringList runServices = config.runServices.value_or(QStringList{});
        QSet<QString> services = {config.service};
        services.unite({runServices.begin(), runServices.end()});

        // With a custom image, the started services are built already, and building the
        // service again would replace the custom image.
        const QStringList buildArgs = serviceInfo->customImage ? QStringList()
                                                               : QStringList{"--build"};

        CommandLine composeCmdLine{
            instanceConfig.dockerCli,
            {"compose",
             composeFiles,
             {"--project-name", projectName(instanceConfig)},
             "up",
             buildArgs,
             "--detach",
             services.values()}};
        process.setCommand(composeCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.configFilePath.parentDir());

        instanceConfig.logFunction(
            QString("Compose Up: %1").arg(process.commandLine().toUserOutput()));
    };

    const auto writeOverride = [featuresState,
                                serviceInfo,
                                imageDetails,
                                overrideFile,
                                config,
                                commonConfig,
                                instanceConfig] {
        const Result<FilePath> file = writeComposeOverride(
            *featuresState, *serviceInfo, *imageDetails, config, commonConfig, instanceConfig);
        if (!file) {
            instanceConfig.logFunction(
                Tr::tr("Cannot write the compose override file: %1").arg(file.error()));
            return DoneResult::Error;
        }
        *overrideFile = *file;
        return DoneResult::Success;
    };

    const DynamicString getBaseImage = (std::function<QString()>) [serviceInfo] {
        return serviceInfo->image;
    };

    const DynamicString getImage = (std::function<QString()>) [containerDetails] {
        return containerDetails->Image;
    };

    const auto needsCustomImage = [featuresState, commonConfig, instanceConfig] {
        return hasFeatures(featuresState)
               || (instanceConfig.localUser && commonConfig.updateRemoteUserUID.value_or(true));
    };

    // The user of the service is the one the container runs as.
    const auto applyServiceUser = [serviceInfo, imageDetails] {
        if (!serviceInfo->user.isEmpty())
            imageDetails->Config.User = serviceInfo->user;
        serviceInfo->customImage = true;
    };

    const auto setupState = [config, instanceConfig](Process &process) {
        CommandLine cmdLine{
            instanceConfig.dockerCli,
            {"ps",
             {"-a", "--no-trunc", "--format", "{{.ID}} {{.State}}"},
             {"--filter", "label=com.docker.compose.project=" + projectName(instanceConfig)},
             {"--filter", "label=com.docker.compose.service=" + config.service}}};
        process.setCommand(cmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
    };
    const auto doneState = [lifecycle](const Process &process) {
        const QString line = process.cleanedStdOut().trimmed().section('\n', 0, 0);
        lifecycle->previousId = line.section(' ', 0, 0);
        lifecycle->previousState = line.section(' ', 1, 1);
    };

    // "docker compose up" recreates the container when its configuration or image changed.
    const auto updateLifecycle = [lifecycle, containerId] {
        lifecycle->created = lifecycle->previousId != *containerId;
        lifecycle->started = lifecycle->created || lifecycle->previousState != "running";
    };

    // clang-format off
    return Group {
        containerId, containerDetails, runningDetails, imageDetails, featuresState, serviceInfo,
        overrideFile, lifecycle, useBuildKit,
        checkDocker(instanceConfig),
        testBuildKit(instanceConfig, useBuildKit),
        initializeCommandRecipe(commonConfig, instanceConfig),
        fetchFeaturesRecipe(featuresState, commonConfig, instanceConfig),
        If (needsCustomImage) >> Then {
            composeServiceInfoTask(serviceInfo, config, instanceConfig),
            If ([serviceInfo] { return !serviceInfo->servicesToBuild.isEmpty(); }) >> Then {
                composeBuildTask(serviceInfo, config, instanceConfig)
            },
            ensureImageRecipe(imageDetails, getBaseImage, instanceConfig),
            requireLinuxImage(imageDetails, instanceConfig),
            QSyncTask(applyServiceUser),
            finalizeImageRecipe(featuresState, imageDetails, getBaseImage, commonConfig, instanceConfig)
        },
        QSyncTask(writeOverride),
        ProcessTask(setupState, doneState, CallDoneFlag::OnSuccess),
        ProcessTask(setupComposeUp),
        findContainerId(containerId, config, instanceConfig),
        QSyncTask(updateLifecycle),
        inspectContainerTask(containerDetails, instanceConfig, containerId),
        inspectImageTask(imageDetails, instanceConfig, getImage),
        requireLinuxImage(imageDetails, instanceConfig),
        runningContainerDetailsTask(containerDetails, runningDetails, commonConfig, instanceConfig, containerId),
        runLifecycleHooksRecipe(featuresState, lifecycle, commonConfig, instanceConfig, containerId),
        fillRunningInstance(runningInstance, runningDetails, imageDetails, featuresState, commonConfig, containerId)
    };
    // clang-format on
}

static Result<Group> prepareRecipe(
    const Config &config,
    const InstanceConfig &instanceConfig,
    const RunningInstance &runningInstance)
{
    return std::visit(
        [&instanceConfig, commonConfig = config.common, runningInstance](
            const auto &containerConfig) {
            return prepareContainerRecipe(
                containerConfig, commonConfig, instanceConfig, runningInstance);
        },
        *config.containerConfig);
}

static void setupRemoveContainer(const InstanceConfig &instanceConfig, Process &process)
{
    connectProcessToLog(process, instanceConfig, Tr::tr("Remove Container"));

    CommandLine removeCmdLine{instanceConfig.dockerCli, {"rm", "-f", containerName(instanceConfig)}};
    process.setCommand(removeCmdLine);
    process.setEnvironment(instanceConfig.localEnvironment);
    process.setWorkingDirectory(instanceConfig.workspaceFolder);

    instanceConfig.logFunction(
        Tr::tr("Removing container: %1").arg(process.commandLine().toUserOutput()));
}

static Result<Group> downContainerRecipe(
    const DockerfileContainer &containerConfig, const InstanceConfig &instanceConfig, bool forceDown)
{
    const auto setupRMContainer = [containerConfig, instanceConfig](Process &process) {
        setupRemoveContainer(instanceConfig, process);
    };

    const auto shouldShutdown = [containerConfig, forceDown]() {
        return forceDown || containerConfig.shutdownAction == ShutdownAction::StopContainer;
    };

    // clang-format off
    return Group {
        If (shouldShutdown) >> Then {
            ProcessTask(setupRMContainer)
        }
    };
    // clang-format on
}

static Result<Group> downContainerRecipe(
    const ImageContainer &imageConfig, const InstanceConfig &instanceConfig, bool forceDown)
{
    const auto setupRemoveImage = [imageConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, Tr::tr("Remove Image"));

        CommandLine removeCmdLine{instanceConfig.dockerCli, {"rmi", imageName(instanceConfig)}};
        process.setCommand(removeCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            Tr::tr("Removing image: %1").arg(process.commandLine().toUserOutput()));
    };

    const auto setupRMContainer = [imageConfig, instanceConfig](Process &process) {
        setupRemoveContainer(instanceConfig, process);
    };

    // Only exists if the user ID was updated on top of the features.
    const auto setupRemoveFeaturesImage = [instanceConfig](Process &process) {
        CommandLine
            removeCmdLine{instanceConfig.dockerCli, {"rmi", featuresImageName(instanceConfig)}};
        process.setCommand(removeCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);
    };

    const auto shouldShutdown = [imageConfig, forceDown]() {
        return forceDown || imageConfig.shutdownAction == ShutdownAction::StopContainer;
    };

    // clang-format off
    return Group{
        If (shouldShutdown) >> Then {
            ProcessTask(setupRMContainer),
            ProcessTask(setupRemoveImage),
            ProcessTask(setupRemoveFeaturesImage, DoneResult::Success)
        }
    };
    // clang-format on
}

static Result<Group> downContainerRecipe(
    const ComposeContainer &config, const InstanceConfig &instanceConfig, bool forceDown)
{
    const auto setupComposeDown = [config, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Compose Down");

        const QStringList composeFilesWithFlag = composeFileArgs(config, instanceConfig);

        CommandLine composeCmdLine{
            instanceConfig.dockerCli,
            {"compose",
             {"--project-name", projectName(instanceConfig)},
             composeFilesWithFlag,
             "down"}};
        process.setCommand(composeCmdLine);
        process.setEnvironment(instanceConfig.localEnvironment);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Compose Down: %1").arg(process.commandLine().toUserOutput()));
    };

    const auto shouldShutdown = [config, forceDown]() {
        return forceDown || config.shutdownAction == ShutdownAction::StopCompose;
    };

    // clang-format off
    return Group {
        If (shouldShutdown) >> Then {
            ProcessTask(setupComposeDown)
        }
    };
    // clang-format on
}

static Result<Group> downRecipe(
    const Config &config, const InstanceConfig &instanceConfig, bool forceDown)
{
    return std::visit(
        [&instanceConfig, forceDown](const auto &containerConfig) {
            return downContainerRecipe(containerConfig, instanceConfig, forceDown);
        },
        *config.containerConfig);
}

Result<Group> Instance::upRecipe(const RunningInstance &runningInstance) const
{
    if (!runningInstance)
        return ResultError(Tr::tr("Running instance cannot be null."));

    return prepareRecipe(d->config, d->instanceConfig, runningInstance);
}

Result<Group> Instance::downRecipe(bool forceDown) const
{
    return ::DevContainer::downRecipe(d->config, d->instanceConfig, forceDown);
}

const Config &Instance::config() const
{
    return d->config;
}

QStringList Instance::imageNames() const
{
    return {
        imageName(d->instanceConfig),
        baseImageName(d->instanceConfig),
        featuresImageName(d->instanceConfig)};
}

static WrappedProcessInterface *makeProcessInterface(
    const Config &config,
    const InstanceConfig &instanceConfig,
    const RunningInstance &runningInstance,
    const DynamicString &containerId)
{
    const auto wrapCommandLine = [=](const ProcessSetupData &setupData,
                                     const QString &markerTemplate,
                                     const QString & /*exitCodeTemplate*/) -> Result<CommandLine> {
        CommandLine dockerCmd{instanceConfig.dockerCli, {"exec", remoteUserArgs(config.common)}};

        const bool inTerminal = setupData.m_terminalMode != TerminalMode::Off
                                || setupData.m_ptyData.has_value();

        const bool interactive = setupData.m_processMode == ProcessMode::Writer
                                 || !setupData.m_writeData.isEmpty() || inTerminal;

        if (interactive)
            dockerCmd.addArg("-i");

        if (inTerminal)
            dockerCmd.addArg("-t");

        Environment remoteEnv;
        for (const auto &[k, v] : config.common.remoteEnv) {
            if (v) {
                QString value = *v;
                const Internal::Replacers replacers = {
                    {"containerEnv", [runningInstance](const QStringList &parts) {
                         if (parts.isEmpty())
                             return QString();
                         const QString varname = parts.first();
                         const QString defaultValue = parts.mid(1).join(':');
                         return runningInstance->remoteEnvironment.value_or(varname, defaultValue);
                     }}};
                Internal::substituteVariables(value, replacers);
                remoteEnv.set(k, value);
            } else {
                remoteEnv.set(k, {}, false); // We use the disabled state to unset the variable.}
            }
        }

        const Environment env = setupData.m_environment.appliedToEnvironment(remoteEnv);

        if (env.hasChanges()) {
            env.forEachEntry([&dockerCmd](const QString &key, const QString &value, bool enabled) {
                if (enabled)
                    dockerCmd.addArgs({"-e", key + "=" + value});
                else
                    dockerCmd.addArgs({"-e", key});
            });
        }

        const FilePath workingDirectory = setupData.rawWorkingDirectory().isEmpty()
                                              ? Config::workspaceFolder(config)
                                              : setupData.rawWorkingDirectory();

        dockerCmd.addArgs({"-w", workingDirectory.path()});

        dockerCmd.addArg(dynamicStringToString(containerId));

        dockerCmd.addArgs({"/bin/sh", "-c"});

        CommandLine exec("exec");
        const CommandLine &cmdLine = setupData.m_commandLine;
        if (cmdLine.executable().osType() == OsTypeWindows) {
            // The arguments are quoted for Windows, but the shell in the container is a Linux one.
            exec.addArg(cmdLine.executable().path(), OsTypeLinux);
            exec.addArgs(cmdLine.splitArguments(), OsTypeLinux);
        } else {
            exec.addCommandLineAsArgs(cmdLine, CommandLine::Raw);
        }

        if (!setupData.m_ptyData) {
            //            auto osAndArch = osTypeAndArch();
            //            if (!osAndArch)
            //                return make_unexpected(osAndArch.error());

            // Check the executable for existence.
            CommandLine testType({"type", {}});
            testType.addArg(
                setupData.m_commandLine.executable().path(),
                OsTypeLinux); //osAndArch->first);
            testType.addArgs(">/dev/null", CommandLine::Raw);

            // Send PID only if existence was confirmed, so we can correctly notify
            // a failed start.
            CommandLine echo("echo");
            echo.addArgs(markerTemplate.arg("$$"), CommandLine::Raw);
            echo.addCommandLineWithAnd(exec);

            testType.addCommandLineWithAnd(echo);

            dockerCmd.addCommandLineAsSingleArg(testType);
        } else {
            dockerCmd.addCommandLineAsSingleArg(exec);
        }

        return dockerCmd;
    };

    const auto controlSignal =
        [config, instanceConfig, containerId](ControlSignal controlSignal, qint64 remotePid) {
            const int signal = ProcessInterface::controlSignalToInt(controlSignal);

            // The signal has to come from the user the process runs as.
            CommandLine dockerCmd{
                instanceConfig.dockerCli,
                {"exec",
                 remoteUserArgs(config.common),
                 dynamicStringToString(containerId),
                 {"kill", QString("-%1").arg(signal), QString::number(remotePid)}}};

            Process p;
            p.setCommand(dockerCmd);
            p.setEnvironment(instanceConfig.localEnvironment);
            p.runBlocking();
        };

    auto *processInterface = new WrappedProcessInterface(wrapCommandLine, controlSignal);

    return processInterface;
}

ProcessInterface *Instance::createProcessInterface(const RunningInstance &runningInstance) const
{
    QTC_ASSERT(runningInstance, return nullptr);
    return makeProcessInterface(
        d->config, d->instanceConfig, runningInstance, runningInstance->containerId);
}

} // namespace DevContainer
