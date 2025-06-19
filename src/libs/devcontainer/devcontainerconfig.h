// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "devcontainer_global.h"

#include <utils/result.h>

#include <map>
#include <optional>
#include <variant>
#include <vector>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVariant>

namespace DevContainer {

// Forward declarations
struct BuildOptions;
struct Mount;
struct DevContainerCommon;
struct NonComposeBase;
struct DockerfileContainer;
struct ImageContainer;
struct ComposeContainer;
struct Config;

// Enums
enum class ShutdownAction { None, StopContainer, StopCompose };

enum class WaitFor {
    InitializeCommand,
    OnCreateCommand,
    UpdateContentCommand,
    PostCreateCommand,
    PostStartCommand
};

enum class UserEnvProbe { None, LoginShell, LoginInteractiveShell, InteractiveShell };
enum class OnAutoForward { Notify, OpenBrowser, OpenBrowserOnce, OpenPreview, Silent, Ignore };
enum class MountType { Bind, Volume };

// Command can be string, array, or object of commands
using CommandMap = std::map<QString, std::variant<QString, QStringList>>;
using Command = std::variant<QString, QStringList, CommandMap>;

// Port attributes structure
struct PortAttributes
{
    OnAutoForward onAutoForward = OnAutoForward::Notify;
    bool elevateIfNeeded = false;
    QString label = "Application";
    bool requireLocalPort = false;
    std::optional<QString> protocol;

    static Utils::Result<PortAttributes> fromJson(const QJsonObject &json);
};

// GPU requirements structure
struct DEVCONTAINER_EXPORT GpuRequirements
{
    struct GpuDetailedRequirements
    {
        std::optional<int> cores;
        std::optional<QString> memory;

        static GpuDetailedRequirements fromJson(const QJsonObject &json);
    };

    std::variant<bool, QString, struct GpuDetailedRequirements> requirements;

    static GpuRequirements fromJson(const QJsonValue &value);
};

// Host hardware requirements structure
struct DEVCONTAINER_EXPORT HostRequirements
{
    std::optional<int> cpus;
    std::optional<QString> memory;
    std::optional<QString> storage;
    std::optional<GpuRequirements> gpu;

    static HostRequirements fromJson(const QJsonObject &json);
};

// Secret metadata structure
struct DEVCONTAINER_EXPORT SecretMetadata
{
    std::optional<QString> description;
    std::optional<QString> documentationUrl;

    static SecretMetadata fromJson(const QJsonObject &json);
};

// Mount structure
struct DEVCONTAINER_EXPORT Mount
{
    MountType type;
    std::optional<QString> source;
    QString target;

    static Utils::Result<Mount> fromJson(const QJsonObject &json);

    static Utils::Result<std::variant<Mount, QString>> fromJsonVariant(const QJsonValue &value);
};

// Build options structure
struct DEVCONTAINER_EXPORT BuildOptions
{
    std::optional<QString> target;
    std::map<QString, QString> args;
    std::optional<std::variant<QString, QStringList>> cacheFrom;
    QStringList options;

    static BuildOptions fromJson(const QJsonObject &json);
};

// Non-compose base structure
struct DEVCONTAINER_EXPORT NonComposeBase
{
    std::optional<std::variant<int, QString, QList<std::variant<int, QString>>>> appPort;
    std::optional<QStringList> runArgs;
    ShutdownAction shutdownAction = ShutdownAction::StopContainer;
    bool overrideCommand = true;
    std::optional<QString> workspaceFolder;
    std::optional<QString> workspaceMount;

    Utils::Result<> fromJson(const QJsonObject &json);
};

// Dockerfile container structure
struct DEVCONTAINER_EXPORT DockerfileContainer : NonComposeBase
{
    QString dockerfile;
    //! Path that the Docker build command should be run from. Relative to the devcontainer.json file.
    QString context = ".";
    std::optional<BuildOptions> buildOptions;

    static Utils::Result<DockerfileContainer> fromJson(const QJsonObject &json);

    static bool isDockerfileContainer(const QJsonObject &json);
};

// Image container structure
struct DEVCONTAINER_EXPORT ImageContainer : NonComposeBase
{
    QString image;

    static Utils::Result<ImageContainer> fromJson(const QJsonObject &json);

    static bool isImageContainer(const QJsonObject &json);
};

// Compose container structure
struct DEVCONTAINER_EXPORT ComposeContainer
{
    std::variant<QString, QStringList> dockerComposeFile;
    QString service;
    std::optional<QStringList> runServices;
    QString workspaceFolder;
    ShutdownAction shutdownAction = ShutdownAction::StopCompose;
    bool overrideCommand = true;

    static Utils::Result<ComposeContainer> fromJson(const QJsonObject &json);

    static bool isComposeContainer(const QJsonObject &json);
};

// Dev container common structure
struct DEVCONTAINER_EXPORT DevContainerCommon
{
    std::optional<QString> schema;
    std::optional<QString> name;
    std::map<QString, QJsonValue> features;
    std::optional<QStringList> overrideFeatureInstallOrder;
    std::map<QString, SecretMetadata> secrets;
    std::optional<QList<std::variant<int, QString>>> forwardPorts;
    std::map<QString, PortAttributes> portsAttributes;
    std::optional<PortAttributes> otherPortsAttributes;
    std::optional<bool> updateRemoteUserUID;
    std::map<QString, QString> containerEnv;
    std::optional<QString> containerUser;
    std::vector<std::variant<Mount, QString>> mounts;
    std::optional<bool> init;
    std::optional<bool> privileged;
    std::optional<QStringList> capAdd;
    std::optional<QStringList> securityOpt;
    std::map<QString, std::optional<QString>> remoteEnv;
    std::optional<QString> remoteUser;
    std::optional<Command> initializeCommand;
    std::optional<Command> onCreateCommand;
    std::optional<Command> updateContentCommand;
    std::optional<Command> postCreateCommand;
    std::optional<Command> postStartCommand;
    std::optional<Command> postAttachCommand;
    std::optional<WaitFor> waitFor;
    UserEnvProbe userEnvProbe = UserEnvProbe::LoginInteractiveShell;
    std::optional<HostRequirements> hostRequirements;
    QJsonObject customizations;
    QJsonObject additionalProperties;

    static Utils::Result<DevContainerCommon> fromJson(const QJsonObject &json);
};

// Complete DevContainer Configuration
struct DEVCONTAINER_EXPORT Config
{
    DevContainerCommon common;

    std::optional<std::variant<DockerfileContainer, ImageContainer, ComposeContainer>>
        containerConfig;

    static Utils::Result<Config> fromJson(const QJsonObject &json);
    static Utils::Result<Config> fromJson(const QByteArray &data);
};

// QDebug stream operators for all DevContainer structures
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::OnAutoForward &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::ShutdownAction &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::WaitFor &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::UserEnvProbe &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::MountType &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::PortAttributes &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::GpuRequirements::GpuDetailedRequirements &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::GpuRequirements &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::HostRequirements &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::SecretMetadata &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::Mount &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const std::variant<DevContainer::Mount, QString> &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::BuildOptions &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::DockerfileContainer &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::ImageContainer &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::ComposeContainer &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::NonComposeBase &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::Command &cmd);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::DevContainerCommon &value);
DEVCONTAINER_EXPORT QDebug operator<<(QDebug debug, const DevContainer::Config &value);

} // namespace DevContainer
