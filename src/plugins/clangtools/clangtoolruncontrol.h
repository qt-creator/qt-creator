// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangfileinfo.h"
#include "clangtoolssettings.h"

#include <cppeditor/clangdiagnosticconfig.h>
#include <cppeditor/projectinfo.h>
#include <projectexplorer/runcontrol.h>
#include <utils/environment.h>
#include <utils/temporarydirectory.h>

#include <QElapsedTimer>
#include <QFutureInterface>
#include <QSet>
#include <QStringList>

namespace ClangTools {
namespace Internal {
class ClangTool;
class ClangToolRunner;
class ProjectBuilder;

struct AnalyzeUnit {
    AnalyzeUnit(const FileInfo &fileInfo,
                const Utils::FilePath &clangResourceDir,
                const QString &clangVersion);

    QString file;
    QStringList arguments; // without file itself and "-o somePath"
};
using AnalyzeUnits = QList<AnalyzeUnit>;

using RunnerCreator = std::function<ClangToolRunner*()>;

class ClangToolRunWorker : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    ClangToolRunWorker(ClangTool *tool,
                       ProjectExplorer::RunControl *runControl,
                       const RunSettings &runSettings,
                       const CppEditor::ClangDiagnosticConfig &diagnosticConfig,
                       const FileInfos &fileInfos,
                       bool buildBeforeAnalysis);

    int filesAnalyzed() const { return m_filesAnalyzed.size(); }
    int filesNotAnalyzed() const { return m_filesNotAnalyzed.size(); }
    int totalFilesToAnalyze() const { return int(m_fileInfos.size()); }

signals:
    void buildFailed();
    void runnerFinished();
    void startFailed();

protected:
    void onRunnerFinishedWithSuccess(ClangToolRunner *runner, const QString &filePath);
    void onRunnerFinishedWithFailure(ClangToolRunner *runner, const QString &errorMessage,
                                     const QString &errorDetails);

private:
    void start() final;
    void stop() final;

    QList<RunnerCreator> runnerCreators(const AnalyzeUnit &unit);
    ClangToolRunner *createRunner(CppEditor::ClangToolType tool, const AnalyzeUnit &unit);

    AnalyzeUnits unitsToAnalyze(const Utils::FilePath &clangIncludeDir,
                                const QString &clangVersion);
    void analyzeNextFile();

    void handleFinished(ClangToolRunner *runner);

    void onProgressCanceled();
    void updateProgressValue();

    void finalize();

private:
    ClangTool * const m_tool;
    RunSettings m_runSettings;
    CppEditor::ClangDiagnosticConfig m_diagnosticConfig;
    FileInfos m_fileInfos;

    ProjectBuilder *m_projectBuilder = nullptr;
    Utils::Environment m_environment;
    Utils::TemporaryDirectory m_temporaryDir;

    CppEditor::ProjectInfo::ConstPtr m_projectInfoBeforeBuild;
    CppEditor::ProjectInfo::ConstPtr m_projectInfo;
    QString m_targetTriple;
    Utils::Id m_toolChainType;

    QFutureInterface<void> m_progress;
    QList<RunnerCreator> m_runnerCreators;
    QSet<Utils::FilePath> m_projectFiles;
    QSet<ClangToolRunner *> m_runners;
    int m_initialQueueSize = 0;
    QSet<QString> m_filesAnalyzed;
    QSet<QString> m_filesNotAnalyzed;

    QElapsedTimer m_elapsed;
};

} // namespace Internal
} // namespace ClangTools
