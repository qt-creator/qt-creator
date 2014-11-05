/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef CLANGSTATICANALYZERTOOL_H
#define CLANGSTATICANALYZERTOOL_H

#include <analyzerbase/ianalyzertool.h>
#include <cpptools/cppprojects.h>

namespace Analyzer { class DetailedErrorView; }

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerDiagnosticModel;
class ClangStaticAnalyzerDiagnosticView;
class Diagnostic;

class ClangStaticAnalyzerTool : public Analyzer::IAnalyzerTool
{
    Q_OBJECT

public:
    explicit ClangStaticAnalyzerTool(QObject *parent = 0);
    CppTools::ProjectInfo projectInfo() const;
    void resetCursorAndProjectInfo();

private:
    QWidget *createWidgets();
    Analyzer::AnalyzerRunControl *createRunControl(const Analyzer::AnalyzerStartParameters &sp,
                                            ProjectExplorer::RunConfiguration *runConfiguration);
    void startTool(Analyzer::StartMode mode);

    void onEngineIsStarting();
    void onNewDiagnosticsAvailable(const QList<Diagnostic> &diagnostics);
    void onEngineFinished();

    void setBusyCursor(bool busy);

private:
    CppTools::ProjectInfo m_projectInfo;

    ClangStaticAnalyzerDiagnosticModel *m_diagnosticModel;
    Analyzer::DetailedErrorView *m_diagnosticView;

    QAction *m_goBack;
    QAction *m_goNext;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERTOOL_H
