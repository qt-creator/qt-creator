// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangfileinfo.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolsdiagnosticmodel.h"

#include <debugger/debuggermainwindow.h>

#include <variant>

QT_BEGIN_NAMESPACE
class QToolButton;

namespace QtTaskTree { class Group; }
QT_END_NAMESPACE

namespace CppEditor { class ClangDiagnosticConfig; }
namespace ProjectExplorer { class RunControl; }
namespace Utils { class FilePath; }

namespace ClangTools::Internal {

class InfoBarWidget;
class Diagnostic;
class DiagnosticFilterModel;
class DiagnosticView;
class RunSettingsData;
class SelectFixitsCheckBox;

class ClangTool : public QObject
{
    Q_OBJECT

public:
    void selectPerspective();

    enum class FileSelectionType {
        AllFiles,
        CurrentFile,
        AskUser,
    };

    using FileSelection = std::variant<FileSelectionType, Utils::FilePath>;

    void startTool(FileSelection fileSelection);
    void startTool(FileSelection fileSelection, const RunSettingsData &runSettings,
                   const CppEditor::ClangDiagnosticConfig &diagnosticConfig);

    FileInfos collectFileInfos(ProjectExplorer::Project *project,
                               FileSelection fileSelection);

    // For testing.
    QSet<Diagnostic> diagnostics() const;

    const QString &name() const;

    enum class RootItemUse { Existing, New };
    void onNewDiagnosticsAvailable(
        const Diagnostics &diagnostics, bool generateMarks, RootItemUse rootItemUse);

    QAction *startAction() const { return m_startAction; }
    QAction *startOnCurrentFileAction() const { return m_startOnCurrentFileAction; }

signals:
    void finished(const QString &errorText); // For testing.

protected:
    ClangTool(const QString &name, Utils::Id id, CppEditor::ClangToolType type);

private:
    QtTaskTree::Group runRecipe(const RunSettingsData &runSettings,
                             const CppEditor::ClangDiagnosticConfig &diagnosticConfig,
                             const FileInfos &fileInfos, bool buildBeforeAnalysis);

    enum class State {
        Initial,
        PreparationStarted,
        PreparationFailed,
        AnalyzerRunning,
        StoppedByUser,
        AnalyzerFinished,
        ImportFinished,
    };
    void setState(State state);
    void update();
    void updateForCurrentState();
    void updateForInitialState();

    void help();

    void filter();
    void clearFilter();
    void filterForCurrentKind();
    void filterOutCurrentKind();
    void setFilterOptions(const OptionalFilterOptions &filterOptions);

    void initDiagnosticView();
    void loadDiagnosticsFromFiles();

    DiagnosticItem *diagnosticItem(const QModelIndex &index) const;
    void showOutputPane();

    void reset();

    FileInfoProviders fileInfoProviders(ProjectExplorer::Project *project,
                                        const FileInfos &allFileInfos);

    const QString m_name;
    ClangToolsDiagnosticModel *m_diagnosticModel = nullptr;
    ProjectExplorer::RunControl *m_runControl = nullptr;

    InfoBarWidget *m_infoBarWidget = nullptr;
    DiagnosticView *m_diagnosticView = nullptr;;

    QAction *m_startAction = nullptr;
    QAction *m_startOnCurrentFileAction = nullptr;
    QAction *m_stopAction = nullptr;

    State m_state = State::Initial;
    int m_filesCount = 0;
    int m_filesSucceeded = 0;
    int m_filesFailed = 0;

    DiagnosticFilterModel *m_diagnosticFilterModel = nullptr;

    QAction *m_showFilter = nullptr;
    SelectFixitsCheckBox *m_selectFixitsCheckBox = nullptr;
    QToolButton *m_applyFixitsButton = nullptr;

    QAction *m_openProjectSettings = nullptr;
    QAction *m_goBack = nullptr;
    QAction *m_goNext = nullptr;
    QAction *m_loadExported = nullptr;
    QAction *m_clear = nullptr;
    QAction *m_expandCollapse = nullptr;

    Utils::Perspective m_perspective;
    const CppEditor::ClangToolType m_type;
};

class ClangTidyTool : public ClangTool
{
public:
    ClangTidyTool();
    static ClangTool *instance() { return m_instance; }

private:
    static inline ClangTool *m_instance = nullptr;
};

class ClazyTool : public ClangTool
{
public:
    ClazyTool();
    static ClangTool *instance() { return m_instance; }

private:
    static inline ClangTool *m_instance = nullptr;
};

} // namespace ClangTools::Internal
