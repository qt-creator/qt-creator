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
#include <QSet>

namespace Utils { class TaskTree; }

namespace ClangTools {
namespace Internal {

struct AnalyzeOutputData;
class ClangTool;
class ProjectBuilder;

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
    ~ClangToolRunWorker();

    int filesAnalyzed() const { return m_filesAnalyzed.size(); }
    int filesNotAnalyzed() const { return m_filesNotAnalyzed.size(); }
    int totalFilesToAnalyze() const { return int(m_fileInfos.size()); }

signals:
    void buildFailed();
    void runnerFinished();
    void startFailed();

private:
    void start() final;
    void stop() final;
    void onDone(const AnalyzeOutputData &output);
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

    std::unique_ptr<Utils::TaskTree> m_taskTree;
    QSet<Utils::FilePath> m_projectFiles;
    QSet<Utils::FilePath> m_filesAnalyzed;
    QSet<Utils::FilePath> m_filesNotAnalyzed;

    QElapsedTimer m_elapsed;
};

} // namespace Internal
} // namespace ClangTools
