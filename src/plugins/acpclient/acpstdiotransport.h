// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "acptransport.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>

#include <QStringDecoder>

#include <optional>

namespace Utils { class Process; }

namespace AcpClient::Internal {

// A bounded buffer of recent stderr for inclusion in error messages. A
// progress line rewrites itself with a carriage return, so what is kept is
// what a terminal would be showing - the last state of each line - and not
// every tick of it, which would push the reason for a failure out of the
// buffer long before it is read.
class StderrTail
{
public:
    void append(const QString &text);
    void clear();
    QString text() const { return m_text; }

private:
    QString m_text;
    int m_lineStart = 0;
    // A carriage return only rewrites the line when a line feed does not
    // follow it, which may be in the next read.
    bool m_pendingCarriageReturn = false;
};

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
    void sendConsoleInput(const QByteArray &data) override;
    void endConsoleInput() override;

private:
    void readOutput();
    void readError();
    QString exitMessageWithStderr(const QString &message) const;

    Utils::CommandLine m_cmd;
    Utils::FilePath m_workingDirectory;
    std::optional<Utils::Environment> m_env;
    Utils::Process *m_process = nullptr;
    bool m_expectStop = false;
    // A character may be split across two reads.
    QStringDecoder m_stderrDecoder{QStringDecoder::Utf8};
    StderrTail m_stderrTail;
};

} // namespace AcpClient::Internal
