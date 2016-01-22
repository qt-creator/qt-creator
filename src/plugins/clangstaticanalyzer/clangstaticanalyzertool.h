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

#ifndef CLANGSTATICANALYZERTOOL_H
#define CLANGSTATICANALYZERTOOL_H

#include <analyzerbase/ianalyzertool.h>
#include <cpptools/projectinfo.h>

#include <QHash>

namespace Analyzer { class DetailedErrorView; }
namespace ProjectExplorer { class Target; }

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerDiagnosticFilterModel;
class ClangStaticAnalyzerDiagnosticModel;
class ClangStaticAnalyzerDiagnosticView;
class Diagnostic;
class DummyRunConfiguration;

const char ClangStaticAnalyzerToolId[] = "ClangStaticAnalyzer";

class ClangStaticAnalyzerTool : public QObject
{
    Q_OBJECT

public:
    explicit ClangStaticAnalyzerTool(QObject *parent = 0);
    CppTools::ProjectInfo projectInfoBeforeBuild() const;
    void resetCursorAndProjectInfoBeforeBuild();

    // For testing.
    bool isRunning() const { return m_running; }
    QList<Diagnostic> diagnostics() const;

    QWidget *createWidgets();
    Analyzer::AnalyzerRunControl *createRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
                                                   Core::Id runMode);
    void startTool(ProjectExplorer::RunConfiguration *rc);

signals:
    void finished(bool success); // For testing.

private:
    void onEngineIsStarting();
    void onNewDiagnosticsAvailable(const QList<Diagnostic> &diagnostics);
    void onEngineFinished();

    void setBusyCursor(bool busy);
    void handleStateUpdate();

private:
    CppTools::ProjectInfo m_projectInfoBeforeBuild;

    ClangStaticAnalyzerDiagnosticModel *m_diagnosticModel;
    ClangStaticAnalyzerDiagnosticFilterModel *m_diagnosticFilterModel;
    ClangStaticAnalyzerDiagnosticView *m_diagnosticView;

    QAction *m_goBack;
    QAction *m_goNext;
    bool m_running;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERTOOL_H
