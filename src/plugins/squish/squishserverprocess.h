// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "squishprocessbase.h"

#include <QObject>

namespace Squish::Internal {

class SquishServerProcess : public SquishProcessBase
{
    Q_OBJECT
public:
    explicit SquishServerProcess(QObject *parent = nullptr);
    ~SquishServerProcess() = default;

    int port() const { return m_serverPort; }

    void start(const Utils::CommandLine &commandLine,
               const Utils::Environment &environment) override;
    void stop();

signals:
    void portRetrieved();

private:
    void onStandardOutput();
    void onErrorOutput() override;
    void onDone() override;

    int m_serverPort = -1;
};

} // namespace Squish::Internal
