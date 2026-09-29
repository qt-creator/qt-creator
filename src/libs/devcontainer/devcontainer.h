// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once
#include "devcontainer_global.h"
#include "devcontainerconfig.h"

#include <QtTaskTree/QTaskTree>

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/processinterface.h>
#include <utils/result.h>

#include <memory>

namespace DevContainer {

struct InstancePrivate;

struct DEVCONTAINER_EXPORT LocalUser
{
    uint uid = 0;
    uint gid = 0;
};

struct DEVCONTAINER_EXPORT InstanceConfig
{
    Utils::FilePath dockerCli = Utils::FilePath("docker").searchInPath();
    Utils::FilePath workspaceFolder;
    Utils::FilePath configFilePath;

    bool runProcessesInTerminal = false;

    std::vector<std::variant<Mount, QString>> mounts;

    Utils::Environment localEnvironment = Utils::Environment::systemEnvironment();

    using LogFunction = std::function<void(const QString &)>;
    LogFunction logFunction = [](const QString &msg) { qDebug().noquote() << msg; };

    //! Where downloaded features are kept. Without it, features are downloaded on every start.
    Utils::FilePath featureCacheFolder;

    //! The user the remote user's UID and GID are changed to ("updateRemoteUserUID").
    //! Only set by default on Linux hosts, where the container shares the IDs of the host.
    std::optional<LocalUser> localUser = defaultLocalUser();

    QString jsonToString(const QJsonValue &value) const;
    QString devContainerId() const;

    static std::optional<LocalUser> defaultLocalUser();
};

struct DEVCONTAINER_EXPORT RunningInstanceData
{
    Utils::OsType osType;
    Utils::OsArch osArch;
    Utils::Environment remoteEnvironment;
    QString containerId;
    //! The customizations of the features merged with the ones of the configuration.
    QJsonObject customizations;
};

using RunningInstance = std::shared_ptr<RunningInstanceData>;

class DEVCONTAINER_EXPORT Instance
{
public:
    explicit Instance(Config config, InstanceConfig instanceConfig);
    static Utils::Result<std::unique_ptr<Instance>> fromFile(InstanceConfig instanceConfig);
    static std::unique_ptr<Instance> fromConfig(
        const Config &config, InstanceConfig instanceConfig = {});

    static Utils::Result<Config> configFromFile(InstanceConfig instanceConfig);

    ~Instance();

    Utils::ProcessInterface *createProcessInterface(const RunningInstance &runningInstance) const;

    Utils::Result<QtTaskTree::Group> upRecipe(const RunningInstance &runningInstance) const;
    Utils::Result<QtTaskTree::Group> downRecipe(bool forceDown) const;

    const Config &config() const;

    //! The names of all images the instance may create, e.g. to remove them.
    QStringList imageNames() const;

private:
    std::unique_ptr<InstancePrivate> d;
};

struct UserFromPasswd
{
    QString name;
    QString uid;
    QString gid;
    QString home;
    QString shell;
};

#ifdef WITH_TESTS
DEVCONTAINER_EXPORT Utils::Result<UserFromPasswd> parseUserFromPasswd(const QString &passwdLine);
#endif

} // namespace DevContainer
