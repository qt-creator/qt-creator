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
#include "clangtoolsdiagnostic.h"
#include "clangtoolsdiagnosticmodel.h"
#include "clangtoolslogfilereader.h"

#include <debugger/debuggermainwindow.h>

#include <projectexplorer/runconfiguration.h>
#include <cpptools/projectinfo.h>

#include <utils/variant.h>

QT_BEGIN_NAMESPACE
class QFrame;
class QToolButton;
QT_END_NAMESPACE

namespace CppTools {
class ClangDiagnosticConfig;
}
namespace Debugger {
class DetailedErrorView;
}
namespace ProjectExplorer {
class RunControl;
}
namespace Utils {
class FilePath;
class FancyLineEdit;
} // namespace Utils

namespace ClangTools {
namespace Internal {

class InfoBarWidget;
class ClangToolsDiagnosticModel;
class ClangToolRunWorker;
class Diagnostic;
class DiagnosticFilterModel;
class DiagnosticView;
class RunSettings;
class SelectFixitsCheckBox;

const char ClangTidyClazyPerspectiveId[] = "ClangTidyClazy.Perspective";

class ClangTool : public QObject
{
    Q_OBJECT

public:
    static ClangTool *instance();

    ClangTool();

    void selectPerspective();

    enum class FileSelectionType {
        AllFiles,
        CurrentFile,
        AskUser,
    };

    using FileSelection = Utils::variant<FileSelectionType, Utils::FilePath>;

    void startTool(FileSelection fileSelection);
    void startTool(FileSelection fileSelection,
                   const RunSettings &runSettings,
                   const CppTools::ClangDiagnosticConfig &diagnosticConfig);

    Diagnostics read(OutputFileFormat outputFileFormat,
                     const QString &logFilePath,
                     const QSet<Utils::FilePath> &projectFiles,
                     QString *errorMessage) const;

    FileInfos collectFileInfos(ProjectExplorer::Project *project,
                               FileSelection fileSelection);

    // For testing.
    QSet<Diagnostic> diagnostics() const;

    const QString &name() const;

    void onNewDiagnosticsAvailable(const Diagnostics &diagnostics);

    QAction *startAction() const { return m_startAction; }
    QAction *startOnCurrentFileAction() const { return m_startOnCurrentFileAction; }

signals:
    void finished(const QString &errorText); // For testing.

private:
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

    void onBuildFailed();
    void onStartFailed();
    void onStarted();
    void onRunControlStopped();

    void initDiagnosticView();
    void loadDiagnosticsFromFiles();

    DiagnosticItem *diagnosticItem(const QModelIndex &index) const;
    void showOutputPane();

    void reset();

    FileInfoProviders fileInfoProviders(ProjectExplorer::Project *project,
                                        const FileInfos &allFileInfos);

    ClangToolsDiagnosticModel *m_diagnosticModel = nullptr;
    ProjectExplorer::RunControl *m_runControl = nullptr;
    ClangToolRunWorker *m_runWorker = nullptr;

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

    Utils::Perspective m_perspective{ClangTidyClazyPerspectiveId, tr("Clang-Tidy and Clazy")};

private:
    const QString m_name;
};

} // namespace Internal
} // namespace ClangTools
