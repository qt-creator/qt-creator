// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainer.h"

#include "devcontainertr.h"

#include <tasking/barrier.h>

#include <utils/algorithm.h>
#include <utils/overloaded.h>
#include <utils/qtcprocess.h>

#include <QCryptographicHash>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(devcontainerlog, "devcontainer")

using namespace Utils;

namespace DevContainer {

struct InstancePrivate
{
    Config config;
    Tasking::TaskTree taskTree;
};

Instance::Instance(Config config)
    : d(std::make_unique<InstancePrivate>())
{
    d->config = std::move(config);
}

Result<std::unique_ptr<Instance>> Instance::fromFile(const FilePath &filePath)
{
    const Result<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return ResultError(contents.error());

    const Result<Config> config = Config::fromJson(*contents);
    if (!config)
        return ResultError(config.error());

    return std::make_unique<Instance>(*config);
}

std::unique_ptr<Instance> Instance::fromConfig(const Config &config)
{
    return std::make_unique<Instance>(config);
}

Instance::~Instance() {};

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
    process.setTextChannelMode(Channel::Output, TextChannelMode::SingleLine);
    process.setTextChannelMode(Channel::Error, TextChannelMode::SingleLine);
    QObject::connect(
        &process, &Process::textOnStandardOutput, [instanceConfig, context](const QString &text) {
            instanceConfig.logFunction(QString("[%1] %2").arg(context).arg(text.trimmed()));
        });

    QObject::connect(
        &process, &Process::textOnStandardError, [instanceConfig, context](const QString &text) {
            instanceConfig.logFunction(QString("[%1] %2").arg(context).arg(text.trimmed()));
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

static ProcessTask inspectImageTask(
    Storage<ImageDetails> imageDetails, const InstanceConfig &instanceConfig)
{
    const auto setupInspectImage = [imageDetails, instanceConfig](Process &process) {
        CommandLine inspectCmdLine{
            instanceConfig.dockerCli, {"inspect", {"--type", "image"}, imageName(instanceConfig)}};

        process.setCommand(inspectCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString(Tr::tr("Inspecting Image: %1")).arg(process.commandLine().toUserOutput()));
    };

    const auto doneInspectImage = [imageDetails](const Process &process) -> DoneResult {
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

template<typename C>
static void setupCreateContainerFromImage(
    const C &containerConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig,
    Process &process)
{
    connectProcessToLog(process, instanceConfig, Tr::tr("Create Container"));

    QStringList containerEnvArgs;

    for (auto &[key, value] : commonConfig.containerEnv)
        containerEnvArgs << "-e" << QString("%1=%2").arg(key, value);

    QStringList appPortArgs;

    if (containerConfig.appPort)
        appPortArgs = createAppPortArgs(*containerConfig.appPort);

    CommandLine createCmdLine{
        instanceConfig.dockerCli,
        {"create",
         {"--name", imageName(instanceConfig) + "-container"},
         containerEnvArgs,
         appPortArgs,
         imageName(instanceConfig)}};
    process.setCommand(createCmdLine);
    process.setWorkingDirectory(instanceConfig.workspaceFolder);

    instanceConfig.logFunction(
        QString(Tr::tr("Creating Container: %1")).arg(process.commandLine().toUserOutput()));
}

static void setupStartContainer(const InstanceConfig &instanceConfig, Process &process)
{
    connectProcessToLog(process, instanceConfig, Tr::tr("Start Container"));

    CommandLine
        startCmdLine{instanceConfig.dockerCli, {"start", imageName(instanceConfig) + "-container"}};
    process.setCommand(startCmdLine);
    process.setWorkingDirectory(instanceConfig.workspaceFolder);

    instanceConfig.logFunction(
        QString(Tr::tr("Starting Container: %1")).arg(process.commandLine().toUserOutput()));
}

static ProcessTask eventMonitor(const InstanceConfig &instanceConfig)
{
    const auto waitForStartedSetup = [instanceConfig](Process &process) {
        CommandLine eventsCmdLine
            = {instanceConfig.dockerCli,
               {"events",
                {"--filter", "event=start"},
                {"--filter", QString("container=%1-container").arg(imageName(instanceConfig))},
                {"--format", "{{json .}}"}}};

        process.setCommand(eventsCmdLine);

        instanceConfig.logFunction(
            QString(Tr::tr("Waiting for Container to Start: %1")).arg(eventsCmdLine.toUserOutput()));

        process.setTextChannelMode(Channel::Output, TextChannelMode::SingleLine);
        process.setTextChannelMode(Channel::Error, TextChannelMode::SingleLine);
        QObject::connect(&process, &Process::textOnStandardOutput, [&process](const QString &text) {
            QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
            if (doc.isNull() || !doc.isObject()) {
                qCWarning(devcontainerlog) << "Received invalid JSON from Docker events:" << text;
                return;
            }
            QJsonObject event = doc.object();
            if (event.contains("status") && event["status"].toString() == "start"
                && event.contains("id")) {
                qCDebug(devcontainerlog) << "Container started:" << event["id"].toString();
                process.stop();
            } else {
                qCWarning(devcontainerlog) << "Unexpected Docker event:" << event;
            }
        });

        QObject::connect(&process, &Process::textOnStandardError, [](const QString &text) {
            qCWarning(devcontainerlog) << "Docker events error:" << text;
        });
    };

    return ProcessTask(waitForStartedSetup, DoneResult::Success);
}

static Result<Group> prepareContainerRecipe(
    const DockerfileContainer &containerConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const auto setupBuild = [containerConfig, instanceConfig](Process &process) {
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

    const auto setupCreate = [commonConfig, containerConfig, instanceConfig](Process &process) {
        setupCreateContainerFromImage(containerConfig, commonConfig, instanceConfig, process);
    };

    const auto setupStart = [instanceConfig](Process &process) {
        setupStartContainer(instanceConfig, process);
    };

    Storage<ImageDetails> imageDetails;

    // clang-format off
    return Group {
        imageDetails,
        ProcessTask(setupBuild),
        inspectImageTask(imageDetails, instanceConfig),
        ProcessTask(setupCreate),
        When (eventMonitor(instanceConfig), &Process::started) >> Do {
            ProcessTask(setupStart)
        }
    };
    // clang-format on
}

static Result<Group> prepareContainerRecipe(
    const ImageContainer &imageConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const auto setupPull = [imageConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Pull Image");

        CommandLine pullCmdLine{instanceConfig.dockerCli, {"pull", imageConfig.image}};
        process.setCommand(pullCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Pulling Image: %1").arg(process.commandLine().toUserOutput()));
    };

    const auto setupTag = [imageConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Tag Image");

        CommandLine tagCmdLine{
            instanceConfig.dockerCli, {"tag", imageConfig.image, imageName(instanceConfig)}};
        process.setCommand(tagCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Tagging Image: %1").arg(process.commandLine().toUserOutput()));
    };

    const auto setupCreate = [commonConfig, imageConfig, instanceConfig](Process &process) {
        setupCreateContainerFromImage(imageConfig, commonConfig, instanceConfig, process);
    };

    const auto setupStart = [instanceConfig](Process &process) {
        setupStartContainer(instanceConfig, process);
    };

    Storage<ImageDetails> imageDetails;

    // clang-format off
    return Group {
        ProcessTask(setupPull),
        ProcessTask(setupTag),
        inspectImageTask(imageDetails, instanceConfig),
        ProcessTask(setupCreate),
        When (eventMonitor(instanceConfig), &Process::started) >> Do {
            ProcessTask(setupStart)
        }
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

    CommandLine removeCmdLine{
        instanceConfig.dockerCli, {"rm", "-f", imageName(instanceConfig) + "-container"}};
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

Result<> Instance::up(const InstanceConfig &instanceConfig)
{
    if (!d->config.containerConfig)
        return ResultOk;

    const Utils::Result<Tasking::Group> recipeResult = upRecipe(instanceConfig);
    if (!recipeResult)
        return ResultError(recipeResult.error());

    d->taskTree.setRecipe(std::move(*recipeResult));
    d->taskTree.start();

    return ResultOk;
}

Result<> Instance::down(const InstanceConfig &instanceConfig)
{
    if (!d->config.containerConfig)
        return ResultOk;

    const Utils::Result<Tasking::Group> recipeResult = downRecipe(instanceConfig);
    if (!recipeResult)
        return ResultError(recipeResult.error());
    d->taskTree.setRecipe(std::move(*recipeResult));
    d->taskTree.start();

    return ResultOk;
}

Result<Tasking::Group> Instance::upRecipe(const InstanceConfig &instanceConfig) const
{
    return prepareRecipe(d->config, instanceConfig);
}

Result<Tasking::Group> Instance::downRecipe(const InstanceConfig &instanceConfig) const
{
    return ::DevContainer::downRecipe(d->config, instanceConfig);
}

} // namespace DevContainer
