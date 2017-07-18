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

#include <QHash>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerDiagnosticFilterModel;
class ClangStaticAnalyzerDiagnosticModel;
class ClangStaticAnalyzerDiagnosticView;
class Diagnostic;
class DummyRunConfiguration;

const char ClangStaticAnalyzerPerspectiveId[] = "ClangStaticAnalyzer.Perspective";
const char ClangStaticAnalyzerDockId[]        = "ClangStaticAnalyzer.Dock";

class ClangStaticAnalyzerTool : public QObject
{
    Q_OBJECT

public:
    ClangStaticAnalyzerTool();
    ~ClangStaticAnalyzerTool();

    static ClangStaticAnalyzerTool *instance();
    QAction *stopAction() { return m_stopAction; }

    CppTools::ProjectInfo projectInfoBeforeBuild() const;
    void resetCursorAndProjectInfoBeforeBuild();

    // For testing.
    QList<Diagnostic> diagnostics() const;
    void startTool();

    void handleWorkerStart(ProjectExplorer::RunWorker *runWorker);

    void onEngineIsStarting();
    void onNewDiagnosticsAvailable(const QList<Diagnostic> &diagnostics);
    void onEngineFinished(bool success);

signals:
    void finished(bool success); // For testing.

private:
    void setBusyCursor(bool busy);
    void handleStateUpdate();
    void updateRunActions();

private:
    CppTools::ProjectInfo m_projectInfoBeforeBuild;

    ClangStaticAnalyzerDiagnosticModel *m_diagnosticModel = nullptr;
    ClangStaticAnalyzerDiagnosticFilterModel *m_diagnosticFilterModel = nullptr;
    ClangStaticAnalyzerDiagnosticView *m_diagnosticView = nullptr;

    QAction *m_startAction = nullptr;
    QAction *m_stopAction = nullptr;
    QAction *m_goBack = nullptr;
    QAction *m_goNext = nullptr;
    QHash<ProjectExplorer::Target *, DummyRunConfiguration *> m_runConfigs;
    bool m_running = false;
    bool m_toolBusy = false;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer
