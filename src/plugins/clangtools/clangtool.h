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
#include "clangtoolslogfilereader.h"

#include <debugger/debuggermainwindow.h>

#include <projectexplorer/runconfiguration.h>
#include <cpptools/projectinfo.h>

QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

namespace CppTools {
class ClangDiagnosticConfig;
}
namespace Debugger {
class DetailedErrorView;
}
namespace Utils {
class FilePath;
class FancyLineEdit;
} // namespace Utils

namespace ClangTools {
namespace Internal {

class ClangToolsDiagnosticModel;
class Diagnostic;
class DiagnosticFilterModel;
class RunSettings;

const char ClangTidyClazyPerspectiveId[] = "ClangTidyClazy.Perspective";

class ClangTool : public QObject
{
    Q_OBJECT

public:
    static ClangTool *instance();

    ClangTool();
    ~ClangTool() override;

    void selectPerspective();

    enum class FileSelection {
        AllFiles,
        CurrentFile,
        AskUser,
    };
    void startTool(FileSelection fileSelection);
    void startTool(FileSelection fileSelection,
                   const RunSettings &runSettings,
                   const CppTools::ClangDiagnosticConfig &diagnosticConfig);

    Diagnostics read(OutputFileFormat outputFileFormat,
                     const QString &logFilePath,
                     const QString &mainFilePath,
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
    void finished(bool success); // For testing.

private:
    void updateRunActions();
    void handleStateUpdate();

    void setToolBusy(bool busy);

    void initDiagnosticView();
    void loadDiagnosticsFromFiles();

    FileInfoProviders fileInfoProviders(ProjectExplorer::Project *project,
                                        const FileInfos &allFileInfos);

    ClangToolsDiagnosticModel *m_diagnosticModel = nullptr;
    QPointer<Debugger::DetailedErrorView> m_diagnosticView;

    QAction *m_startAction = nullptr;
    QAction *m_startOnCurrentFileAction = nullptr;
    QAction *m_stopAction = nullptr;
    bool m_running = false;
    bool m_toolBusy = false;

    DiagnosticFilterModel *m_diagnosticFilterModel = nullptr;

    Utils::FancyLineEdit *m_filterLineEdit = nullptr;
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
