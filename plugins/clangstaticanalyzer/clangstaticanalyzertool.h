/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef CLANGSTATICANALYZERTOOL_H
#define CLANGSTATICANALYZERTOOL_H

#include <analyzerbase/ianalyzertool.h>
#include <cpptools/cppprojects.h>

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
    Analyzer::AnalyzerRunControl *createRunControl(const Analyzer::AnalyzerStartParameters &sp,
                                            ProjectExplorer::RunConfiguration *runConfiguration);
    void startTool();

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
    Analyzer::DetailedErrorView *m_diagnosticView;

    QAction *m_goBack;
    QAction *m_goNext;
    QHash<ProjectExplorer::Target *, DummyRunConfiguration *> m_runConfigs;
    bool m_running;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERTOOL_H
