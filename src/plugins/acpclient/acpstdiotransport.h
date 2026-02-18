// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "acptransport.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>

#include <optional>

namespace Utils { class Process; }

namespace AcpClient::Internal {

class AcpStdioTransport : public AcpTransport
{
    Q_OBJECT

public:
    explicit AcpStdioTransport(QObject *parent = nullptr);
    ~AcpStdioTransport() override;

    void setCommandLine(const Utils::CommandLine &cmd);
    void setWorkingDirectory(const Utils::FilePath &workingDirectory);
    void setEnvironment(const Utils::Environment &environment);

    void start() override;
    void stop() override;

protected:
    void sendData(const QByteArray &data) override;

private:
    void readOutput();
    void readError();

    Utils::CommandLine m_cmd;
    Utils::FilePath m_workingDirectory;
    std::optional<Utils::Environment> m_env;
    Utils::Process *m_process = nullptr;
    bool m_expectStop = false;
    QString m_stderrBuffer;
};

} // namespace AcpClient::Internal
