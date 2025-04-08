// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/commandline.h>

#include <QThread>

namespace Docker::Internal {

class Internal;

class DockerContainerThread
{
public:
    struct Init
    {
        Utils::CommandLine createContainerCmd;
        Utils::FilePath dockerBinaryPath;
    };

public:
    ~DockerContainerThread();

    QString containerId() const;

    static Utils::expected_str<std::unique_ptr<DockerContainerThread>> create(const Init &init);

private:
    DockerContainerThread(Init init);
    Utils::Result<> start();

private:
    QThread m_thread;
    Internal *m_internal;
    QString m_containerId;
};

} // namespace Docker::Internal
