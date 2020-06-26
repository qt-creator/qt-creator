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
#include "clangtoolssettings.h"

#include <cpptools/clangdiagnosticconfig.h>
#include <cpptools/projectinfo.h>
#include <projectexplorer/runcontrol.h>
#include <utils/environment.h>
#include <utils/temporarydirectory.h>

#include <QElapsedTimer>
#include <QFutureInterface>
#include <QSet>
#include <QStringList>

namespace ClangTools {
namespace Internal {

class ClangToolRunner;
class ProjectBuilder;

struct AnalyzeUnit {
    AnalyzeUnit(const QString &file, const QStringList &options)
        : file(file), arguments(options) {}

    QString file;
    QStringList arguments; // without file itself and "-o somePath"
};
using AnalyzeUnits = QList<AnalyzeUnit>;

using RunnerCreator = std::function<ClangToolRunner*()>;

struct QueueItem {
    AnalyzeUnit unit;
    RunnerCreator runnerCreator;
};
using QueueItems = QList<QueueItem>;

class ClangToolRunWorker : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    ClangToolRunWorker(ProjectExplorer::RunControl *runControl,
                       const RunSettings &runSettings,
                       const CppTools::ClangDiagnosticConfig &diagnosticConfig,
                       const FileInfos &fileInfos,
                       bool buildBeforeAnalysis);

    int filesAnalyzed() const { return m_filesAnalyzed.size(); }
    int filesNotAnalyzed() const { return m_filesNotAnalyzed.size(); }
    int totalFilesToAnalyze() const { return m_fileInfos.size(); }

signals:
    void buildFailed();
    void runnerFinished();
    void startFailed();

protected:
    void onRunnerFinishedWithSuccess(const QString &filePath);
    void onRunnerFinishedWithFailure(const QString &errorMessage, const QString &errorDetails);

private:
    void start() final;
    void stop() final;

    QList<RunnerCreator> runnerCreators();
    template <class T> ClangToolRunner *createRunner();

    AnalyzeUnits unitsToAnalyze(const Utils::FilePath &clangResourceDir,
                                const QString &clangVersion);
    void analyzeNextFile();

    void handleFinished();

    void onProgressCanceled();
    void updateProgressValue();

    void finalize();

private:
    RunSettings m_runSettings;
    CppTools::ClangDiagnosticConfig m_diagnosticConfig;
    FileInfos m_fileInfos;

    ProjectBuilder *m_projectBuilder = nullptr;
    Utils::Environment m_environment;
    Utils::TemporaryDirectory m_temporaryDir;

    CppTools::ProjectInfo m_projectInfoBeforeBuild;
    CppTools::ProjectInfo m_projectInfo;
    QString m_targetTriple;
    Utils::Id m_toolChainType;

    QFutureInterface<void> m_progress;
    QueueItems m_queue;
    QSet<Utils::FilePath> m_projectFiles;
    QSet<ClangToolRunner *> m_runners;
    int m_initialQueueSize = 0;
    QSet<QString> m_filesAnalyzed;
    QSet<QString> m_filesNotAnalyzed;

    QElapsedTimer m_elapsed;
};

} // namespace Internal
} // namespace ClangTools
