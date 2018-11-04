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

#include "clangfileinfo.h"

#include <projectexplorer/runconfiguration.h>
#include <cpptools/projectinfo.h>
#include <utils/environment.h>
#include <utils/temporarydirectory.h>

#include <QFutureInterface>
#include <QStringList>

namespace ClangTools {
namespace Internal {

class ClangTool;
class ClangToolRunner;
class Diagnostic;
class ProjectBuilder;

struct AnalyzeUnit {
    AnalyzeUnit(const QString &file, const QStringList &options)
        : file(file), arguments(options) {}

    QString file;
    QStringList arguments; // without file itself and "-o somePath"
};
using AnalyzeUnits = QList<AnalyzeUnit>;

class ClangToolRunControl : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    ClangToolRunControl(ProjectExplorer::RunControl *runControl,
                        ProjectExplorer::Target *target,
                        const FileInfos &fileInfos);

    bool success() const { return m_success; } // For testing.

    virtual ClangTool *tool() = 0;

protected:
    void init();

    virtual ClangToolRunner *createRunner() = 0;

    void onRunnerFinishedWithSuccess(const QString &filePath);
    void onRunnerFinishedWithFailure(const QString &errorMessage, const QString &errorDetails);

private:
    void start() final;
    void stop() final;

    AnalyzeUnits unitsToAnalyze();
    void analyzeNextFile();

    void handleFinished();

    void onProgressCanceled();
    void updateProgressValue();

    void finalize();

protected:
    ProjectBuilder *m_projectBuilder;
    Utils::Environment m_environment;
    QString m_clangExecutable;
    Utils::TemporaryDirectory m_temporaryDir;

private:
    QPointer<ProjectExplorer::Target> m_target;
    FileInfos m_fileInfos;

    CppTools::ProjectInfo m_projectInfoBeforeBuild;
    CppTools::ProjectInfo m_projectInfo;
    QString m_targetTriple;
    Core::Id m_toolChainType;

    QFutureInterface<void> m_progress;
    AnalyzeUnits m_unitsToProcess;
    QSet<ClangToolRunner *> m_runners;
    int m_initialFilesToProcessSize = 0;
    int m_filesAnalyzed = 0;
    int m_filesNotAnalyzed = 0;
    bool m_success = false;
};

} // namespace Internal
} // namespace ClangTools
