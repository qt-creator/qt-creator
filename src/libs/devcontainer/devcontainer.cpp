// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainer.h"

#include "devcontainertr.h"

#include <utils/algorithm.h>
#include <utils/overloaded.h>
#include <utils/qtcprocess.h>

#include <QCryptographicHash>

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

static Result<Group> prepareContainerRecipe(
    const DockerfileContainer &containerConfig,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const auto setupBuild = [containerConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, Tr::tr("Build Dockerfile"));

        const FilePath configFileDir = instanceConfig.configFilePath.parentDir();
        const FilePath contextPath = configFileDir.resolvePath(
            containerConfig.context.value_or("."));
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
        connectProcessToLog(process, instanceConfig, Tr::tr("Create Container"));

        QStringList containerEnvArgs;

        for (auto &[key, value] : commonConfig.containerEnv)
            containerEnvArgs << "-e" << QString("%1=%2").arg(key, value);

        CommandLine createCmdLine{
            instanceConfig.dockerCli,
            {"create",
             {"--name", imageName(instanceConfig) + "-container"},
             containerEnvArgs,
             //             {"-v", containerConfig.volumeMounts.join(" -v ")},
             //             {"-p", containerConfig.portMappings.createArguments()},
             imageName(instanceConfig)}};
        process.setCommand(createCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString(Tr::tr("Creating Container: %1")).arg(process.commandLine().toUserOutput()));
    };

    return Group{ProcessTask(setupBuild)};
}

static Result<Group> prepareContainerRecipe(
    const ImageContainer &config,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
    const auto setupPull = [config, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Pull Image");

        CommandLine pullCmdLine{instanceConfig.dockerCli, {"pull", config.image}};
        process.setCommand(pullCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Pulling Image: %1").arg(process.commandLine().toUserOutput()));
    };

    const auto setupTag = [config, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, "Tag Image");

        CommandLine
            tagCmdLine{instanceConfig.dockerCli, {"tag", config.image, imageName(instanceConfig)}};
        process.setCommand(tagCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString("Tagging Image: %1").arg(process.commandLine().toUserOutput()));
    };

    // clang-format off
    return Group {
        ProcessTask(setupPull),
        ProcessTask(setupTag)
    };
    // clang-format on
}

static Result<Group> prepareContainerRecipe(
    const ComposeContainer &config,
    const DevContainerCommon &commonConfig,
    const InstanceConfig &instanceConfig)
{
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

Result<> Instance::down()
{
    return ResultError(Tr::tr("Down operation is not implemented yet."));
}

Result<Tasking::Group> Instance::upRecipe(const InstanceConfig &instanceConfig) const
{
    return prepareRecipe(d->config, instanceConfig);
}

Result<Tasking::Group> Instance::downRecipe() const
{
    return ResultError(Tr::tr("Down recipe is not implemented yet."));
}

} // namespace DevContainer
