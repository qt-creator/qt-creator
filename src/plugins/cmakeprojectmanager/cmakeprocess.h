// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/outputformatter.h>

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>

#include <memory>

namespace Utils {
class ProcessResultData;
class Process;
}

namespace CMakeProjectManager::Internal {

class BuildDirParameters;

class CMakeProcess : public QObject
{
    Q_OBJECT

public:
    CMakeProcess();
    ~CMakeProcess();

    void run(const BuildDirParameters &parameters, const QStringList &arguments);
    void stop();

signals:
    void finished(int exitCode);
    void stdOutReady(const QString &s);

private:
    std::unique_ptr<Utils::Process> m_process;
    Utils::OutputFormatter m_parser;
    QElapsedTimer m_elapsed;
};

QString addCMakePrefix(const QString &str);
QStringList addCMakePrefix(const QStringList &list);

} // CMakeProjectManager::Internal
