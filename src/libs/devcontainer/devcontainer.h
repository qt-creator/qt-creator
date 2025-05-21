// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once
#include "devcontainer_global.h"
#include "devcontainerconfig.h"

#include <tasking/tasktree.h>

#include <utils/filepath.h>

#include <memory>

namespace DevContainer {

struct InstancePrivate;

struct DEVCONTAINER_EXPORT InstanceConfig
{
    Utils::FilePath dockerCli;
    Utils::FilePath dockerComposeCli;
    Utils::FilePath workspaceFolder;
    Utils::FilePath configFilePath;

    using LogFunction = std::function<void(const QString &)>;
    LogFunction logFunction = [](const QString &msg) { qDebug() << msg; };
};

class DEVCONTAINER_EXPORT Instance
{
public:
    explicit Instance(Config config);
    static Utils::Result<std::unique_ptr<Instance>> fromFile(const Utils::FilePath &filePath);
    static std::unique_ptr<Instance> fromConfig(const Config &config);
    ~Instance();

    void up(const InstanceConfig &instanceConfig); // Create and start the container
    void down();                                   // Stop and remove the container

    Tasking::Group upRecipe(const InstanceConfig &instanceConfig) const;
    Tasking::Group downRecipe() const;

private:
    std::unique_ptr<InstancePrivate> d;
};

} // namespace DevContainer
