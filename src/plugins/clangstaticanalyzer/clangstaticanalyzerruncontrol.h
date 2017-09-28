/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <projectexplorer/runconfiguration.h>
#include <cpptools/projectinfo.h>
#include <utils/environment.h>

#include <QFutureInterface>
#include <QStringList>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerRunner;
class ProjectBuilder;
class Diagnostic;

struct AnalyzeUnit {
    AnalyzeUnit(const QString &file, const QStringList &options)
        : file(file), arguments(options) {}

    QString file;
    QStringList arguments; // without file itself and "-o somePath"
};
typedef QList<AnalyzeUnit> AnalyzeUnits;

class ClangStaticAnalyzerToolRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    ClangStaticAnalyzerToolRunner(ProjectExplorer::RunControl *runControl,
                                  ProjectExplorer::Target *target);

    bool success() const { return m_success; } // For testing.

private:
    void start() final;
    void stop() final;

    AnalyzeUnits sortedUnitsToAnalyze();
    void analyzeNextFile();
    ClangStaticAnalyzerRunner *createRunner();

    void onRunnerFinishedWithSuccess(const QString &logFilePath);
    void onRunnerFinishedWithFailure(const QString &errorMessage, const QString &errorDetails);
    void handleFinished();

    void onProgressCanceled();
    void updateProgressValue();

    void finalize();

private:
    QPointer<ProjectExplorer::Target> m_target;
    ProjectBuilder *m_projectBuilder;

    CppTools::ProjectInfo m_projectInfoBeforeBuild;
    CppTools::ProjectInfo m_projectInfo;
    QString m_targetTriple;
    Core::Id m_toolChainType;

    Utils::Environment m_environment;
    QString m_clangExecutable;
    QString m_clangLogFileDir;
    QFutureInterface<void> m_progress;
    AnalyzeUnits m_unitsToProcess;
    QSet<ClangStaticAnalyzerRunner *> m_runners;
    int m_initialFilesToProcessSize = 0;
    int m_filesAnalyzed = 0;
    int m_filesNotAnalyzed = 0;
    bool m_success = false;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer
