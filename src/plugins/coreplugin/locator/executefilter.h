// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

#include <utils/commandline.h>

#include <QStringList>
#include <QTextCodec>

namespace Utils { class Process; }

namespace Core::Internal {

class ExecuteFilter : public Core::ILocatorFilter
{
    struct ExecuteData
    {
        Utils::CommandLine command;
        Utils::FilePath workingDirectory;
    };

public:
    ExecuteFilter();
    ~ExecuteFilter() override;

private:
    LocatorMatcherTasks matchers() final;
    void acceptCommand(const QString &cmd);
    void done();
    void readStdOutput();
    void readStdError();
    void runHeadCommand();

    void createProcess();
    void removeProcess();

    void saveState(QJsonObject &object) const final;
    void restoreState(const QJsonObject &object) final;

    QString headCommand() const;

    QList<ExecuteData> m_taskQueue;
    QStringList m_commandHistory;
    Utils::Process *m_process = nullptr;
    QTextCodec::ConverterState m_stdoutState;
    QTextCodec::ConverterState m_stderrState;
};

} // namespace Core::Internal
