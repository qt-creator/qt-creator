// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainerconfig.h"

#include "devcontainertr.h"

#include <utils/stringutils.h>

using namespace Utils;

namespace DevContainer {

template<typename... Ts>
inline QDebug operator<<(QDebug debug, const std::variant<Ts...> &value);

template<typename... Ts>
inline QDebug operator<<(QDebug debug, const QList<std::variant<Ts...>> &value)
{
    bool first = true;
    for (const auto &val : value) {
        if (!first)
            debug << ", ";
        debug << val;
        first = false;
    }

    return debug;
}

template<typename... Ts>
inline QDebug operator<<(QDebug debug, const std::variant<Ts...> &value)
{
    std::visit([&debug](const auto &v) { debug << v; }, value);
    return debug;
}

// Helper for parsing enum values
template<typename EnumType>
Result<EnumType> parseEnum(
    const QString &value, const std::map<QString, EnumType> &mapping, EnumType defaultValue)
{
    auto it = mapping.find(value.toLower());
    if (it != mapping.end()) {
        return it->second;
    }
    return defaultValue;
}

std::variant<QString, QStringList> parseStringOrList(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();

    if (value.isArray()) {
        QStringList list;
        QJsonArray array = value.toArray();
        for (const QJsonValue &v : array) {
            if (v.isString())
                list.append(v.toString());
            else
                qWarning() << "Expected string in array, found:" << v;
        }
        return list;
    }

    return QString();
}

// Parse Command objects (string, array or object)
Command parseCommand(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();

    if (value.isArray()) {
        QStringList commands;
        QJsonArray commandArray = value.toArray();
        for (const QJsonValue &cmd : commandArray)
            commands.append(cmd.toString());

        return commands;
    }

    if (value.isObject()) {
        QJsonObject commandObj = value.toObject();
        std::map<QString, std::variant<QString, QStringList>> commandMap;

        for (auto it = commandObj.begin(); it != commandObj.end(); ++it)
            commandMap[it.key()] = parseStringOrList(it.value());

        return commandMap;
    }

    // Default case
    return QString();
}

Result<DevContainer::Config> DevContainer::Config::fromJson(const QJsonObject &json)
{
    Config config;

    // Parse common properties
    Result<DevContainerCommon> common = DevContainerCommon::fromJson(json);
    if (!common)
        return ResultError(common.error());

    config.common = *common;

    // Determine and parse the container configuration type
    if (ComposeContainer::isComposeContainer(json)) {
        Result<ComposeContainer> containerConfig = ComposeContainer::fromJson(json);
        if (!containerConfig)
            return ResultError(containerConfig.error());
        config.containerConfig = *containerConfig;
    } else if (DockerfileContainer::isDockerfileContainer(json)) {
        Result<DockerfileContainer> containerConfig = DockerfileContainer::fromJson(json);
        if (!containerConfig)
            return ResultError(containerConfig.error());
        config.containerConfig = *containerConfig;
    } else if (ImageContainer::isImageContainer(json)) {
        Result<ImageContainer> containerConfig = ImageContainer::fromJson(json);
        if (!containerConfig)
            return ResultError(containerConfig.error());
        config.containerConfig = *containerConfig;
    }

    return config;
}

Result<DevContainer::DevContainerCommon> DevContainer::DevContainerCommon::fromJson(
    const QJsonObject &json)
{
    DevContainerCommon common;

    if (json.contains("$schema"))
        common.schema = json["$schema"].toString();

    if (json.contains("name"))
        common.name = json["name"].toString();

    if (json.contains("features") && json["features"].isObject()) {
        QJsonObject featuresObj = json["features"].toObject();
        for (auto it = featuresObj.begin(); it != featuresObj.end(); ++it) {
            common.features[it.key()] = it.value();
        }
    }

    if (json.contains("overrideFeatureInstallOrder")
        && json["overrideFeatureInstallOrder"].isArray()) {
        QStringList features;
        QJsonArray featuresArray = json["overrideFeatureInstallOrder"].toArray();
        for (const QJsonValue &value : featuresArray) {
            features.append(value.toString());
        }
        common.overrideFeatureInstallOrder = features;
    }

    if (json.contains("secrets") && json["secrets"].isObject()) {
        QJsonObject secretsObj = json["secrets"].toObject();
        for (auto it = secretsObj.begin(); it != secretsObj.end(); ++it) {
            if (it.value().isObject())
                common.secrets[it.key()] = SecretMetadata::fromJson(it.value().toObject());
        }
    }

    if (json.contains("forwardPorts") && json["forwardPorts"].isArray()) {
        QList<std::variant<int, QString>> ports;
        QJsonArray portsArray = json["forwardPorts"].toArray();
        for (const QJsonValue &value : portsArray) {
            if (value.isDouble())
                ports.append(value.toInt());
            else if (value.isString())
                ports.append(value.toString());
        }
        common.forwardPorts = ports;
    }

    if (json.contains("portsAttributes") && json["portsAttributes"].isObject()) {
        QJsonObject portAttrsObj = json["portsAttributes"].toObject();
        for (auto it = portAttrsObj.begin(); it != portAttrsObj.end(); ++it) {
            if (it.value().isObject()) {
                const Result<PortAttributes> portAttributes = PortAttributes::fromJson(
                    it.value().toObject());
                if (!portAttributes)
                    return ResultError(portAttributes.error());
                common.portsAttributes[it.key()] = *portAttributes;
            }
        }
    }

    if (json.contains("otherPortsAttributes") && json["otherPortsAttributes"].isObject()) {
        const Result<PortAttributes> portAttributes = PortAttributes::fromJson(
            json["otherPortsAttributes"].toObject());
        if (!portAttributes)
            return ResultError(portAttributes.error());
        common.otherPortsAttributes = *portAttributes;
    }

    if (json.contains("updateRemoteUserUID")) {
        common.updateRemoteUserUID = json["updateRemoteUserUID"].toBool();
    }

    if (json.contains("containerEnv") && json["containerEnv"].isObject()) {
        QJsonObject envObj = json["containerEnv"].toObject();
        for (auto it = envObj.begin(); it != envObj.end(); ++it)
            common.containerEnv[it.key()] = it.value().toString();
    }

    if (json.contains("containerUser"))
        common.containerUser = json["containerUser"].toString();

    if (json.contains("mounts") && json["mounts"].isArray()) {
        QJsonArray mountsArray = json["mounts"].toArray();
        for (const QJsonValue &value : mountsArray) {
            const Result<std::variant<Mount, QString>> mount = Mount::fromJsonVariant(value);
            if (!mount)
                return ResultError(mount.error());
            common.mounts.push_back(*mount);
        }
    }

    if (json.contains("init"))
        common.init = json["init"].toBool();

    if (json.contains("privileged"))
        common.privileged = json["privileged"].toBool();

    if (json.contains("capAdd") && json["capAdd"].isArray()) {
        QStringList capabilities;
        QJsonArray capArray = json["capAdd"].toArray();
        for (const QJsonValue &value : capArray)
            capabilities.append(value.toString());
        common.capAdd = capabilities;
    }

    if (json.contains("securityOpt") && json["securityOpt"].isArray()) {
        QStringList secOpts;
        QJsonArray secOptsArray = json["securityOpt"].toArray();
        for (const QJsonValue &value : secOptsArray)
            secOpts.append(value.toString());
        common.securityOpt = secOpts;
    }

    if (json.contains("remoteEnv") && json["remoteEnv"].isObject()) {
        QJsonObject envObj = json["remoteEnv"].toObject();
        for (auto it = envObj.begin(); it != envObj.end(); ++it) {
            if (it.value().isNull())
                common.remoteEnv[it.key()] = std::nullopt;
            else
                common.remoteEnv[it.key()] = it.value().toString();
        }
    }

    if (json.contains("remoteUser"))
        common.remoteUser = json["remoteUser"].toString();

    // Parse all the command fields
    if (json.contains("initializeCommand"))
        common.initializeCommand = parseCommand(json["initializeCommand"]);

    if (json.contains("onCreateCommand"))
        common.onCreateCommand = parseCommand(json["onCreateCommand"]);

    if (json.contains("updateContentCommand"))
        common.updateContentCommand = parseCommand(json["updateContentCommand"]);

    if (json.contains("postCreateCommand"))
        common.postCreateCommand = parseCommand(json["postCreateCommand"]);

    if (json.contains("postStartCommand"))
        common.postStartCommand = parseCommand(json["postStartCommand"]);

    if (json.contains("postAttachCommand"))
        common.postAttachCommand = parseCommand(json["postAttachCommand"]);

    if (json.contains("waitFor")) {
        static const std::map<QString, WaitFor> waitForMap
            = {{"initializeCommand", WaitFor::InitializeCommand},
               {"onCreateCommand", WaitFor::OnCreateCommand},
               {"updateContentCommand", WaitFor::UpdateContentCommand},
               {"postCreateCommand", WaitFor::PostCreateCommand},
               {"postStartCommand", WaitFor::PostStartCommand}};

        const Result<WaitFor> waitFor = parseEnum<WaitFor>(
            json["waitFor"].toString(), waitForMap, WaitFor::UpdateContentCommand);

        if (!waitFor)
            return ResultError(waitFor.error());

        common.waitFor = *waitFor;
    }

    if (json.contains("userEnvProbe")) {
        static const std::map<QString, UserEnvProbe> userEnvProbeMap
            = {{"none", UserEnvProbe::None},
               {"loginShell", UserEnvProbe::LoginShell},
               {"loginInteractiveShell", UserEnvProbe::LoginInteractiveShell},
               {"interactiveShell", UserEnvProbe::InteractiveShell}};
        const Result<UserEnvProbe> userEnvProbe = parseEnum<UserEnvProbe>(
            json["userEnvProbe"].toString(), userEnvProbeMap, UserEnvProbe::LoginInteractiveShell);

        if (!userEnvProbe)
            return ResultError(userEnvProbe.error());

        common.userEnvProbe = *userEnvProbe;
    }

    if (json.contains("hostRequirements") && json["hostRequirements"].isObject())
        common.hostRequirements = HostRequirements::fromJson(json["hostRequirements"].toObject());

    if (json.contains("customizations") && json["customizations"].isObject())
        common.customizations = json["customizations"].toObject();

    // Extract any additional properties that aren't explicitly covered
    for (auto it = json.begin(); it != json.end(); ++it) {
        if (!it.key().startsWith("$") && // Skip schema
            !json.contains(it.key()) &&  // Skip already processed fields
            it.value().isObject()) {
            common.additionalProperties.insert(it.key(), it.value());
        }
    }

    return common;
}

Result<> NonComposeBase::fromJson(const QJsonObject &json)
{
    if (json.contains("appPort")) {
        if (json["appPort"].isDouble()) {
            appPort = json["appPort"].toInt();
        } else if (json["appPort"].isString()) {
            appPort = json["appPort"].toString();
        } else if (json["appPort"].isArray()) {
            QList<std::variant<int, QString>> ports;
            QJsonArray portsArray = json["appPort"].toArray();
            for (const QJsonValue &value : portsArray) {
                if (value.isDouble())
                    ports.append(value.toInt());
                else if (value.isString())
                    ports.append(value.toString());
            }
            appPort = ports;
        }
    }

    if (json.contains("runArgs") && json["runArgs"].isArray()) {
        QStringList args;
        QJsonArray argsArray = json["runArgs"].toArray();
        for (const QJsonValue &value : argsArray)
            args.append(value.toString());
        runArgs = args;
    }

    if (json.contains("shutdownAction")) {
        static const std::map<QString, ShutdownAction> shutdownActionMap
            = {{"none", ShutdownAction::None}, {"stopContainer", ShutdownAction::StopContainer}};

        const Result<ShutdownAction> sa = parseEnum<ShutdownAction>(
            json["shutdownAction"].toString(), shutdownActionMap, ShutdownAction::StopContainer);
        if (!sa)
            return ResultError(sa.error());

        shutdownAction = *sa;
    }

    if (json.contains("overrideCommand"))
        overrideCommand = json["overrideCommand"].toBool();

    if (json.contains("workspaceFolder"))
        workspaceFolder = json["workspaceFolder"].toString();

    if (json.contains("workspaceMount"))
        workspaceMount = json["workspaceMount"].toString();

    return ResultOk;
}

bool ComposeContainer::isComposeContainer(const QJsonObject &json)
{
    return json.contains("dockerComposeFile") && json.contains("service")
           && json.contains("workspaceFolder");
}

Result<ComposeContainer> ComposeContainer::fromJson(const QJsonObject &json)
{
    ComposeContainer container;

    if (json.contains("dockerComposeFile")) {
        if (json["dockerComposeFile"].isString()) {
            container.dockerComposeFile = json["dockerComposeFile"].toString();
        } else if (json["dockerComposeFile"].isArray()) {
            QStringList composeFiles;
            QJsonArray composeArray = json["dockerComposeFile"].toArray();
            for (const QJsonValue &value : composeArray) {
                composeFiles.append(value.toString());
            }
            container.dockerComposeFile = composeFiles;
        }
    }

    if (json.contains("service"))
        container.service = json["service"].toString();

    if (json.contains("runServices") && json["runServices"].isArray()) {
        QStringList services;
        QJsonArray servicesArray = json["runServices"].toArray();
        for (const QJsonValue &value : servicesArray)
            services.append(value.toString());
        container.runServices = services;
    }

    if (json.contains("workspaceFolder"))
        container.workspaceFolder = json["workspaceFolder"].toString();

    if (json.contains("shutdownAction")) {
        static const std::map<QString, ShutdownAction> shutdownActionMap
            = {{"none", ShutdownAction::None}, {"stopCompose", ShutdownAction::StopCompose}};

        const Result<ShutdownAction> sa = parseEnum<ShutdownAction>(
            json["shutdownAction"].toString(), shutdownActionMap, ShutdownAction::StopCompose);
        if (!sa)
            return ResultError(sa.error());
        container.shutdownAction = *sa;
    }

    if (json.contains("overrideCommand"))
        container.overrideCommand = json["overrideCommand"].toBool();

    return container;
}

bool ImageContainer::isImageContainer(const QJsonObject &json)
{
    return json.contains("image");
}

Result<ImageContainer> ImageContainer::fromJson(const QJsonObject &json)
{
    ImageContainer container;

    Result<> baseResult = container.NonComposeBase::fromJson(json);
    if (!baseResult)
        return ResultError(baseResult.error());

    if (json.contains("image"))
        container.image = json["image"].toString();

    return container;
}

bool DockerfileContainer::isDockerfileContainer(const QJsonObject &json)
{
    return (json.contains("build") && json["build"].isObject()) || json.contains("dockerFile");
}

Result<DockerfileContainer> DockerfileContainer::fromJson(const QJsonObject &json)
{
    DockerfileContainer container;
    Result<> baseResult = container.NonComposeBase::fromJson(json);
    if (!baseResult)
        return ResultError(baseResult.error());

    // Handle the two different formats
    if (json.contains("build") && json["build"].isObject()) {
        QJsonObject buildObj = json["build"].toObject();

        if (buildObj.contains("dockerfile"))
            container.dockerfile = buildObj["dockerfile"].toString();

        if (buildObj.contains("context"))
            container.context = buildObj["context"].toString();

        // Extract build options
        container.buildOptions = BuildOptions::fromJson(buildObj);
        return container;
    }
    // Alternative format
    if (json.contains("dockerFile"))
        container.dockerfile = json["dockerFile"].toString();

    if (json.contains("context"))
        container.context = json["context"].toString();

    if (json.contains("build") && json["build"].isObject())
        container.buildOptions = BuildOptions::fromJson(json["build"].toObject());

    return container;
}

BuildOptions BuildOptions::fromJson(const QJsonObject &json)
{
    BuildOptions opts;

    if (json.contains("target"))
        opts.target = json["target"].toString();

    if (json.contains("args") && json["args"].isObject()) {
        QJsonObject argsObj = json["args"].toObject();
        for (auto it = argsObj.begin(); it != argsObj.end(); ++it)
            opts.args[it.key()] = it.value().toString();
    }

    if (json.contains("cacheFrom")) {
        if (json["cacheFrom"].isString()) {
            opts.cacheFrom = json["cacheFrom"].toString();
        } else if (json["cacheFrom"].isArray()) {
            QStringList cacheList;
            QJsonArray cacheArray = json["cacheFrom"].toArray();
            for (const QJsonValue &value : cacheArray)
                cacheList.append(value.toString());
            opts.cacheFrom = cacheList;
        }
    }

    if (json.contains("options") && json["options"].isArray()) {
        QJsonArray optionsArray = json["options"].toArray();
        for (const QJsonValue &value : optionsArray)
            opts.options.append(value.toString());
    }

    return opts;
}

Result<std::variant<Mount, QString>> Mount::fromJsonVariant(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();
    else if (value.isObject())
        return Mount::fromJson(value.toObject());

    return ResultError(
        Tr::tr("Invalid mount format: expected string or object, found %1").arg(value.type()));
}

Result<Mount> Mount::fromJson(const QJsonObject &json)
{
    Mount mount;

    if (!json.contains("type"))
        return ResultError(Tr::tr("Invalid mount format: missing 'type' field in mount object"));
    if (!json.contains("target"))
        return ResultError(Tr::tr("Invalid mount format: missing 'target' field in mount object"));

    static const std::map<QString, MountType> mountTypeMap
        = {{"bind", MountType::Bind}, {"volume", MountType::Volume}};

    const Result<MountType> mountType
        = parseEnum<MountType>(json["type"].toString(), mountTypeMap, MountType::Bind);

    if (!mountType)
        return ResultError(mountType.error());

    mount.type = *mountType;

    if (json.contains("source"))
        mount.source = json["source"].toString();

    mount.target = json["target"].toString();

    return mount;
}

SecretMetadata SecretMetadata::fromJson(const QJsonObject &json)
{
    SecretMetadata metadata;

    if (json.contains("description"))
        metadata.description = json["description"].toString();

    if (json.contains("documentationUrl"))
        metadata.documentationUrl = json["documentationUrl"].toString();

    return metadata;
}

HostRequirements HostRequirements::fromJson(const QJsonObject &json)
{
    HostRequirements req;

    if (json.contains("cpus"))
        req.cpus = json["cpus"].toInt();

    if (json.contains("memory"))
        req.memory = json["memory"].toString();

    if (json.contains("storage"))
        req.storage = json["storage"].toString();

    if (json.contains("gpu"))
        req.gpu = GpuRequirements::fromJson(json["gpu"]);

    return req;
}

GpuRequirements GpuRequirements::fromJson(const QJsonValue &value)
{
    GpuRequirements req;

    if (value.isBool())
        req.requirements = value.toBool();
    else if (value.isString())
        req.requirements = value.toString();
    else if (value.isObject())
        req.requirements = GpuDetailedRequirements::fromJson(value.toObject());

    return req;
}

GpuRequirements::GpuDetailedRequirements GpuRequirements::GpuDetailedRequirements::fromJson(
    const QJsonObject &json)
{
    GpuDetailedRequirements req;

    if (json.contains("cores"))
        req.cores = json["cores"].toInt();

    if (json.contains("memory"))
        req.memory = json["memory"].toString();

    return req;
}

Result<PortAttributes> PortAttributes::fromJson(const QJsonObject &json)
{
    PortAttributes attrs;

    if (json.contains("onAutoForward")) {
        static const std::map<QString, OnAutoForward> onAutoForwardMap
            = {{"notify", OnAutoForward::Notify},
               {"openBrowser", OnAutoForward::OpenBrowser},
               {"openBrowserOnce", OnAutoForward::OpenBrowserOnce},
               {"openPreview", OnAutoForward::OpenPreview},
               {"silent", OnAutoForward::Silent},
               {"ignore", OnAutoForward::Ignore}};
        const Result<OnAutoForward> onAutoForward = parseEnum<OnAutoForward>(
            json["onAutoForward"].toString(), onAutoForwardMap, OnAutoForward::Notify);
        if (!onAutoForward)
            return ResultError(onAutoForward.error());
        attrs.onAutoForward = *onAutoForward;
    }

    if (json.contains("elevateIfNeeded"))
        attrs.elevateIfNeeded = json["elevateIfNeeded"].toBool();

    if (json.contains("label"))
        attrs.label = json["label"].toString();

    if (json.contains("requireLocalPort"))
        attrs.requireLocalPort = json["requireLocalPort"].toBool();

    if (json.contains("protocol"))
        attrs.protocol = json["protocol"].toString();

    return attrs;
}

Result<Config> Config::fromJson(const QByteArray &data)
{
    const QByteArray cleanedInput = removeCommentsFromJson(data);

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(cleanedInput, &error);
    if (error.error != QJsonParseError::NoError) {
        return ResultError(
            Tr::tr("Failed to parse devcontainer json file: %1").arg(error.errorString()));
    }

    if (!doc.isObject())
        return ResultError(Tr::tr("Invalid devcontainer json file: expected an object"));

    QJsonObject json = doc.object();
    return Config::fromJson(json);
}

QDebug operator<<(QDebug debug, const DevContainer::Config &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Config(\n";

    debug << "Common: " << value.common << "\n";

    debug << "Container: ";
    if (!value.containerConfig) {
        debug << "None (Common properties only)";
    } else {
        if (std::holds_alternative<DevContainer::DockerfileContainer>(*value.containerConfig)) {
            const auto &dockerfile = std::get<DevContainer::DockerfileContainer>(
                *value.containerConfig);
            debug << "Dockerfile Container\n";
            debug << "  Dockerfile: " << dockerfile << "\n";
            debug << "  Base: " << (NonComposeBase) dockerfile;
        } else if (std::holds_alternative<DevContainer::ImageContainer>(*value.containerConfig)) {
            const auto image = std::get<DevContainer::ImageContainer>(*value.containerConfig);
            debug << "Image Container\n";
            debug << "  Image: " << image << "\n";
            debug << "  Base: " << (NonComposeBase) image;
        } else if (std::holds_alternative<DevContainer::ComposeContainer>(*value.containerConfig)) {
            debug << "Compose Container\n";
            debug << "  " << std::get<DevContainer::ComposeContainer>(*value.containerConfig);
        }
    }

    debug << "\n)";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::DevContainerCommon &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "DevContainerCommon(\n";

    if (value.schema)
        debug << "  schema=" << *value.schema << "\n";

    if (value.name)
        debug << "  name=" << *value.name << "\n";

    if (!value.features.empty()) {
        debug << "  features={";
        bool first = true;
        for (const auto &[key, val] : value.features) {
            if (!first)
                debug << ", ";
            debug << key;
            first = false;
        }
        debug << "}\n";
    }

    if (value.overrideFeatureInstallOrder)
        debug << "  overrideFeatureInstallOrder=" << *value.overrideFeatureInstallOrder << "\n";

    if (!value.secrets.empty()) {
        debug << "  secrets={\n";
        for (const auto &[key, metadata] : value.secrets) {
            debug << "    " << key << ": " << metadata << "\n";
        }
        debug << "  }\n";
    }

    if (value.forwardPorts)
        debug << "  forwardPorts=[" << *value.forwardPorts << "]\n";

    if (!value.portsAttributes.empty()) {
        debug << "  portsAttributes={\n";
        for (const auto &[key, attrs] : value.portsAttributes) {
            debug << "    " << key << ": " << attrs << "\n";
        }
        debug << "  }\n";
    }

    if (value.otherPortsAttributes)
        debug << "  otherPortsAttributes=" << *value.otherPortsAttributes << "\n";

    if (value.updateRemoteUserUID)
        debug << "  updateRemoteUserUID=" << *value.updateRemoteUserUID << "\n";

    if (!value.containerEnv.empty()) {
        debug << "  containerEnv={\n";
        for (const auto &[key, val] : value.containerEnv) {
            debug << "    " << key << ": " << val << "\n";
        }
        debug << "  }\n";
    }

    if (value.containerUser)
        debug << "  containerUser=" << *value.containerUser << "\n";

    if (!value.mounts.empty()) {
        debug << "  mounts=[\n";
        for (const auto &mount : value.mounts) {
            debug << "    " << mount << "\n";
        }
        debug << "  ]\n";
    }

    if (value.init)
        debug << "  init=" << *value.init << "\n";

    if (value.privileged)
        debug << "  privileged=" << *value.privileged << "\n";

    if (value.capAdd)
        debug << "  capAdd=" << *value.capAdd << "\n";

    if (value.securityOpt)
        debug << "  securityOpt=" << *value.securityOpt << "\n";

    if (!value.remoteEnv.empty()) {
        debug << "  remoteEnv={\n";
        for (const auto &[key, val] : value.remoteEnv) {
            debug << "    " << key << ": ";
            if (val) {
                debug << *val;
            } else {
                debug << "null";
            }
            debug << "\n";
        }
        debug << "  }\n";
    }

    if (value.remoteUser)
        debug << "  remoteUser=" << *value.remoteUser << "\n";

    if (value.initializeCommand)
        debug << "  initializeCommand=" << *value.initializeCommand << "\n";

    if (value.onCreateCommand)
        debug << "  onCreateCommand=" << *value.onCreateCommand << "\n";

    if (value.updateContentCommand)
        debug << "  updateContentCommand=" << *value.updateContentCommand << "\n";

    if (value.postCreateCommand)
        debug << "  postCreateCommand=" << *value.postCreateCommand << "\n";

    if (value.postStartCommand)
        debug << "  postStartCommand=" << *value.postStartCommand << "\n";

    if (value.postAttachCommand)
        debug << "  postAttachCommand=" << *value.postAttachCommand << "\n";

    if (value.waitFor)
        debug << "  waitFor=" << *value.waitFor << "\n";

    debug << "  userEnvProbe=" << value.userEnvProbe << "\n";

    if (value.hostRequirements)
        debug << "  hostRequirements=" << *value.hostRequirements << "\n";

    if (!value.customizations.isEmpty())
        debug << "  customizations=<JSON object>\n";

    if (!value.additionalProperties.isEmpty())
        debug << "  additionalProperties=<JSON object>\n";

    debug << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::Command &cmd)
{
    QDebugStateSaver saver(debug);
    if (std::holds_alternative<QString>(cmd))
        return debug << "Command(string=" << std::get<QString>(cmd) << ")";

    if (std::holds_alternative<QStringList>(cmd))
        return debug << "Command(array=" << std::get<QStringList>(cmd) << ")";

    debug << "Command(parallel={";
    const auto &map = std::get<std::map<QString, std::variant<QString, QStringList>>>(cmd);
    bool first = true;

    for (const auto &[key, val] : map) {
        if (!first)
            debug << ", ";
        debug << key << ":";
        if (std::holds_alternative<QString>(val))
            debug << std::get<QString>(val);
        else if (std::holds_alternative<QStringList>(val))
            debug << std::get<QStringList>(val);

        first = false;
    }
    debug << "})";

    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::OnAutoForward &value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case DevContainer::OnAutoForward::Notify:
        debug << "Notify";
        break;
    case DevContainer::OnAutoForward::OpenBrowser:
        debug << "OpenBrowser";
        break;
    case DevContainer::OnAutoForward::OpenBrowserOnce:
        debug << "OpenBrowserOnce";
        break;
    case DevContainer::OnAutoForward::OpenPreview:
        debug << "OpenPreview";
        break;
    case DevContainer::OnAutoForward::Silent:
        debug << "Silent";
        break;
    case DevContainer::OnAutoForward::Ignore:
        debug << "Ignore";
        break;
    }
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::ShutdownAction &value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case DevContainer::ShutdownAction::None:
        debug << "None";
        break;
    case DevContainer::ShutdownAction::StopContainer:
        debug << "StopContainer";
        break;
    case DevContainer::ShutdownAction::StopCompose:
        debug << "StopCompose";
        break;
    }
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::WaitFor &value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case DevContainer::WaitFor::InitializeCommand:
        debug << "InitializeCommand";
        break;
    case DevContainer::WaitFor::OnCreateCommand:
        debug << "OnCreateCommand";
        break;
    case DevContainer::WaitFor::UpdateContentCommand:
        debug << "UpdateContentCommand";
        break;
    case DevContainer::WaitFor::PostCreateCommand:
        debug << "PostCreateCommand";
        break;
    case DevContainer::WaitFor::PostStartCommand:
        debug << "PostStartCommand";
        break;
    }
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::UserEnvProbe &value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case DevContainer::UserEnvProbe::None:
        debug << "None";
        break;
    case DevContainer::UserEnvProbe::LoginShell:
        debug << "LoginShell";
        break;
    case DevContainer::UserEnvProbe::LoginInteractiveShell:
        debug << "LoginInteractiveShell";
        break;
    case DevContainer::UserEnvProbe::InteractiveShell:
        debug << "InteractiveShell";
        break;
    }
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::MountType &value)
{
    QDebugStateSaver saver(debug);
    switch (value) {
    case DevContainer::MountType::Bind:
        debug << "Bind";
        break;
    case DevContainer::MountType::Volume:
        debug << "Volume";
        break;
    }
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::PortAttributes &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "PortAttributes(";
    debug << "onAutoForward=" << value.onAutoForward;
    debug << ", elevateIfNeeded=" << value.elevateIfNeeded;
    debug << ", label=" << value.label;
    debug << ", requireLocalPort=" << value.requireLocalPort;
    if (value.protocol) {
        debug << ", protocol=" << *value.protocol;
    }
    debug << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::GpuRequirements::GpuDetailedRequirements &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "GpuDetailedRequirements(";
    if (value.cores) {
        debug << "cores=" << *value.cores;
    }
    if (value.memory) {
        debug << (value.cores ? ", " : "") << "memory=" << *value.memory;
    }
    debug << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::GpuRequirements &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "GpuRequirements(";
    if (std::holds_alternative<bool>(value.requirements)) {
        debug << "required=" << std::get<bool>(value.requirements);
    } else if (std::holds_alternative<QString>(value.requirements)) {
        debug << "option=" << std::get<QString>(value.requirements);
    } else if (std::holds_alternative<DevContainer::GpuRequirements::GpuDetailedRequirements>(
                   value.requirements)) {
        debug << std::get<DevContainer::GpuRequirements::GpuDetailedRequirements>(
            value.requirements);
    }
    debug << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::HostRequirements &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "HostRequirements(";
    if (value.cpus) {
        debug << "cpus=" << *value.cpus;
    }
    if (value.memory) {
        debug << (value.cpus ? ", " : "") << "memory=" << *value.memory;
    }
    if (value.storage) {
        debug << ((value.cpus || value.memory) ? ", " : "") << "storage=" << *value.storage;
    }
    if (value.gpu) {
        debug << ((value.cpus || value.memory || value.storage) ? ", " : "")
              << "gpu=" << *value.gpu;
    }
    debug << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::SecretMetadata &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "SecretMetadata(";
    if (value.description) {
        debug << "description=" << *value.description;
    }
    if (value.documentationUrl) {
        debug << (value.description ? ", " : "") << "documentationUrl=" << *value.documentationUrl;
    }
    debug << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::Mount &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Mount(type=" << value.type;
    if (value.source) {
        debug << ", source=" << *value.source;
    }
    debug << ", target=" << value.target << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const std::variant<DevContainer::Mount, QString> &value)
{
    QDebugStateSaver saver(debug);
    if (std::holds_alternative<DevContainer::Mount>(value)) {
        debug << std::get<DevContainer::Mount>(value);
    } else if (std::holds_alternative<QString>(value)) {
        debug << "MountString(" << std::get<QString>(value) << ")";
    }
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::BuildOptions &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "BuildOptions(";
    if (value.target) {
        debug << "target=" << *value.target;
    }

    if (!value.args.empty()) {
        debug << (value.target ? ", " : "") << "args={";
        bool first = true;
        for (const auto &[key, val] : value.args) {
            if (!first)
                debug << ", ";
            debug << key << ":" << val;
            first = false;
        }
        debug << "}";
    }

    if (value.cacheFrom)
        debug << "cacheFrom=" << *value.cacheFrom;

    //if (std::holds_alternative<QString>(value.cacheFrom)) {
    //    debug << ((value.target || !value.args.empty()) ? ", " : "")
    //          << "cacheFrom=" << std::get<QString>(value.cacheFrom);
    //} else if (std::holds_alternative<QStringList>(value.cacheFrom)) {
    //    debug << ((value.target || !value.args.empty()) ? ", " : "")
    //          << "cacheFrom=" << std::get<QStringList>(*value.cacheFrom);
    //}

    if (!value.options.isEmpty()) {
        debug << ((value.target || !value.args.empty() || value.cacheFrom) ? ", " : "")
              << "options=" << value.options;
    }

    debug << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::DockerfileContainer &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "DockerfileContainer(";
    debug << "dockerfile=" << value.dockerfile;

    debug << ", " << "context=" << value.context;

    if (value.buildOptions)
        debug << ", " << "buildOptions=" << *value.buildOptions;

    debug << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::ImageContainer &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ImageContainer(image=" << value.image << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::ComposeContainer &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ComposeContainer(";

    debug << "dockerComposeFile=";
    if (std::holds_alternative<QString>(value.dockerComposeFile))
        debug << std::get<QString>(value.dockerComposeFile);
    else if (std::holds_alternative<QStringList>(value.dockerComposeFile))
        debug << std::get<QStringList>(value.dockerComposeFile);

    debug << ", service=" << value.service;

    if (value.runServices)
        debug << ", runServices=" << *value.runServices;

    debug << ", workspaceFolder=" << value.workspaceFolder;
    debug << ", shutdownAction=" << value.shutdownAction;

    debug << ", overrideCommand=" << value.overrideCommand;

    debug << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const DevContainer::NonComposeBase &value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "NonComposeBase(";

    if (value.appPort)
        debug << "appPort=[" << *value.appPort << "], ";

    if (value.runArgs)
        debug << "runArgs=" << *value.runArgs << ", ";

    debug << "shutdownAction=" << value.shutdownAction;

    debug << ", overrideCommand=" << value.overrideCommand;

    if (value.workspaceFolder)
        debug << ", workspaceFolder=" << *value.workspaceFolder;

    if (value.workspaceMount)
        debug << ", workspaceMount=" << *value.workspaceMount;

    debug << ")";
    return debug;
}
} // namespace DevContainer
