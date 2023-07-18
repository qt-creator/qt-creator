// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/process.h>

namespace Nim::Suggest {

class NimSuggestServer : public QObject
{
    Q_OBJECT

public:
    NimSuggestServer(QObject *parent = nullptr);

    bool start(const Utils::FilePath &executablePath, const Utils::FilePath &projectFilePath);
    void stop();

    quint16 port() const;

signals:
    void started();
    void done();

private:
    void onStandardOutputAvailable();
    void onDone();
    void clearState();

    bool m_portAvailable = false;
    Utils::Process m_process;
    quint16 m_port = 0;
    Utils::FilePath m_projectFilePath;
    Utils::FilePath m_executablePath;
};

} // Nim::Suggest
