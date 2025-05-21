// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainer.h"

#include "devcontainertr.h"

#include <utils/qtcprocess.h>

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

static Group prepareContainerRecipe(
    const DockerfileContainer &containerConfig, const InstanceConfig &instanceConfig)
{
    return Group{ProcessTask([containerConfig, instanceConfig](Process &process) {
        connectProcessToLog(process, instanceConfig, Tr::tr("Build Dockerfile"));

        const FilePath configFileDir = instanceConfig.configFilePath.parentDir();
        const FilePath contextPath = configFileDir.resolvePath(
            containerConfig.context.value_or("."));
        const FilePath dockerFile = configFileDir.resolvePath(containerConfig.dockerfile);

        CommandLine buildCmdLine{
            instanceConfig.dockerCli,
            {"build", "-f", dockerFile.nativePath(), contextPath.nativePath()}};
        process.setCommand(buildCmdLine);
        process.setWorkingDirectory(instanceConfig.workspaceFolder);

        instanceConfig.logFunction(
            QString(Tr::tr("Building Dockerfile: %1")).arg(process.commandLine().toUserOutput()));
    })};
}

static Group prepareContainerRecipe(
    const ImageContainer &config, const InstanceConfig &instanceConfig)
{
    Q_UNUSED(config);
    Q_UNUSED(instanceConfig);
    return Group{};
}

static Group prepareContainerRecipe(
    const ComposeContainer &config, const InstanceConfig &instanceConfig)
{
    Q_UNUSED(config);
    Q_UNUSED(instanceConfig);
    return Group{};
}

static Group prepareContainerRecipe(const std::monostate &, const InstanceConfig &instanceConfig)
{
    Q_UNUSED(instanceConfig);
    return {};
}

static Group prepareRecipe(const Config &config, const InstanceConfig &instanceConfig)
{
    return std::visit(
        [&instanceConfig](const auto &containerConfig) {
            return prepareContainerRecipe(containerConfig, instanceConfig);
        },
        config.containerConfig);
}

void Instance::up(const InstanceConfig &instanceConfig)
{
    d->taskTree.setRecipe(upRecipe(instanceConfig));
    d->taskTree.start();
}

void Instance::down() {}

Tasking::Group Instance::upRecipe(const InstanceConfig &instanceConfig) const
{
    return prepareRecipe(d->config, instanceConfig);
}

Tasking::Group Instance::downRecipe() const
{
    return Group{};
}

} // namespace DevContainer
