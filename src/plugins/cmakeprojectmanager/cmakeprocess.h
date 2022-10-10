// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/outputformatter.h>

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>

#include <memory>

namespace Utils {
class ProcessResultData;
class QtcProcess;
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

    int lastExitCode() const { return m_lastExitCode; }

signals:
    void started();
    void finished();

private:
    void handleProcessDone(const Utils::ProcessResultData &resultData);

    std::unique_ptr<Utils::QtcProcess> m_process;
    Utils::OutputFormatter m_parser;
    QElapsedTimer m_elapsed;
    int m_lastExitCode = 0;
};

} // CMakeProjectManager::Internal
