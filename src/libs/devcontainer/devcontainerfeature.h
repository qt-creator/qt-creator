// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "devcontainer_global.h"
#include "devcontainerconfig.h"

#include <utils/filepath.h>
#include <utils/result.h>

#include <QJsonObject>
#include <QUrl>

namespace DevContainer {

struct DEVCONTAINER_EXPORT FeatureOption
{
    static FeatureOption fromJson(
        const QJsonObject &obj, const JsonStringToString &jsonStringToString);

    QString type;
    QVariant defaultValue;
    QString description;
    QStringList enumValues;
    QStringList proposals;
};

struct DEVCONTAINER_EXPORT Feature
{
    static Utils::Result<Feature> fromJson(
        const QJsonObject &obj, const JsonStringToString &jsonStringToString);

    static Utils::Result<Feature> fromJson(
        const QByteArray &data, const JsonStringToString &jsonStringToString);

    // Identification fields
    QString id;
    QStringList legacyIds;
    QString version;

    // Meta data fields
    QString description;
    QString documentationURL;
    QString licenseURL;
    QString name;
    QStringList keywords;
    bool deprecated = false;

    // Fields that are added / merged onto the container config
    QString entrypoint;
    QStringList capAdd;
    QStringList securityOpt;
    std::map<QString, QString> containerEnv;
    QJsonObject customizations;
    std::vector<std::variant<Mount, QString>> mounts;

    bool init = false;
    bool privileged = false;

    std::optional<Command> onCreateCommand;
    std::optional<Command> updateContentCommand;
    std::optional<Command> postCreateCommand;
    std::optional<Command> postStartCommand;
    std::optional<Command> postAttachCommand;

    // Installation order fields (https://containers.dev/implementors/features/#installation-order)
    QStringList installsAfter;
    QList<FeatureDependency> dependsOn;

    // Options for the feature (https://containers.dev/implementors/features/#options-property)
    QMap<QString, FeatureOption> options;
};

// Where a feature comes from, as written in the "features" map of a devcontainer.json, or in
// the "dependsOn" map of a devcontainer-feature.json.
// See https://containers.dev/implementors/features-distribution/#referencing-a-feature
struct DEVCONTAINER_EXPORT FeatureReference
{
    enum class Type { Oci, Tarball, Local };

    // Relative local references are resolved against configFolder, the folder containing the
    // devcontainer.json.
    static Utils::Result<FeatureReference> parse(
        const QString &reference, const Utils::FilePath &configFolder);

    // The reference without its version, e.g. "ghcr.io/devcontainers/features/node".
    // This is what "installsAfter" and "overrideFeatureInstallOrder" refer to.
    QString resource() const;

    Type type = Type::Oci;
    QString userReference;
    QString id;

    QString registry;
    QString repository;
    QString tag = "latest";
    QString digest;

    QString url;

    Utils::FilePath localPath;
};

struct DEVCONTAINER_EXPORT ResolvedFeature
{
    FeatureReference reference;
    QJsonObject userOptions;
    Feature feature;
    // The folder on the host that holds the extracted feature, including its install.sh.
    Utils::FilePath folder;

    // Identifies the feature together with the options it is installed with, so the same
    // feature requested twice with the same options is installed once.
    QString key() const;
    // The options the install script sees: the defaults of the feature overridden by the user.
    QJsonObject effectiveOptions() const;
};

DEVCONTAINER_EXPORT QString
featureKey(const FeatureReference &reference, const QJsonObject &options);

// Turns an option name into the name of the environment variable the install script sees.
DEVCONTAINER_EXPORT QString featureOptionEnvName(const QString &optionName);
DEVCONTAINER_EXPORT QByteArray featureEnvFileContents(const ResolvedFeature &feature);

// Returns the features in the order they have to be installed in.
DEVCONTAINER_EXPORT Utils::Result<std::vector<ResolvedFeature>> featureInstallOrder(
    std::vector<ResolvedFeature> features, const std::optional<QStringList> &overrideOrder);

struct DEVCONTAINER_EXPORT FeatureBuildContext
{
    QString containerUser;
    QString remoteUser;
    // The user the base image runs as. Restored after the features are installed.
    QString imageUser;
};

// Writes the Dockerfile, the per feature option files and the built-in variables into
// contextFolder. The features have to be in install order, and the folder of each has to be
// in contextFolder / "dev-container-features". Returns the Dockerfile.
DEVCONTAINER_EXPORT Utils::Result<Utils::FilePath> writeFeaturesBuildContext(
    const std::vector<ResolvedFeature> &features,
    const FeatureBuildContext &context,
    const Utils::FilePath &contextFolder);

DEVCONTAINER_EXPORT QString featuresDockerfile(
    const std::vector<ResolvedFeature> &features, const FeatureBuildContext &context);

DEVCONTAINER_EXPORT QJsonObject mergeCustomizations(QJsonObject left, const QJsonObject &right);

// The customizations of the features, in install order, merged with the ones of the config.
DEVCONTAINER_EXPORT QJsonObject mergedCustomizations(
    const std::vector<ResolvedFeature> &features, const QJsonObject &configCustomizations);

struct DEVCONTAINER_EXPORT RegistryCredentials
{
    QString username;
    QString secret;
};

// Where the credentials for a registry come from, as the Docker CLI configures it in
// $DOCKER_CONFIG/config.json or ~/.docker/config.json.
struct DEVCONTAINER_EXPORT RegistryCredentialSource
{
    std::optional<RegistryCredentials> credentials;
    // The name of a credential helper: "docker-credential-<helper> get" provides them.
    QString helper;
    // What the helper is asked for.
    QString serverUrl;
    // Why the Docker configuration could not be read, if it exists but is broken.
    QString error;
};

// Whether the registry is on this machine, and is talked to over plain HTTP.
DEVCONTAINER_EXPORT bool isLocalRegistry(const QString &registry);

// The host that serves the registry API, which is not the registry name for Docker Hub.
DEVCONTAINER_EXPORT QString registryApiHost(const QString &registry);

// Whether both URLs have the same scheme, host and port, so credentials for one may go to the
// other.
DEVCONTAINER_EXPORT bool isSameOrigin(const QUrl &first, const QUrl &second);

// The path in the container a mount is mounted at.
DEVCONTAINER_EXPORT QString mountTarget(const std::variant<Mount, QString> &mount);

// Keeps the last of the mounts with the same target, as a later source wins.
DEVCONTAINER_EXPORT std::vector<std::variant<Mount, QString>> mergeMounts(
    const std::vector<std::variant<Mount, QString>> &mounts);

DEVCONTAINER_EXPORT RegistryCredentialSource
registryCredentialSource(const QString &registry, const Utils::Environment &environment);
DEVCONTAINER_EXPORT Utils::Result<RegistryCredentials> parseCredentialHelperOutput(
    const QByteArray &output);

// A Dockerfile that changes the UID and GID of a user of the base image. It takes the build
// arguments _DEV_CONTAINERS_BASE_IMAGE, REMOTE_USER, NEW_UID, NEW_GID and IMAGE_USER.
DEVCONTAINER_EXPORT QString updateUidDockerfile();

} // namespace DevContainer
