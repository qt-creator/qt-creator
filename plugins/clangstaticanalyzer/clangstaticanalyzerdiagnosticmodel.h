/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
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

#ifndef CLANGSTATICANALYZERDIAGNOSTICMODEL_H
#define CLANGSTATICANALYZERDIAGNOSTICMODEL_H

#include "clangstaticanalyzerdiagnostic.h"
#include "clangstaticanalyzerprojectsettings.h"

#include <analyzerbase/detailederrorview.h>
#include <utils/fileutils.h>
#include <utils/treemodel.h>

#include <QPointer>
#include <QSortFilterProxyModel>

namespace ProjectExplorer { class Project; }

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerDiagnosticModel : public Utils::TreeModel
{
    Q_OBJECT

public:
    ClangStaticAnalyzerDiagnosticModel(QObject *parent = 0);

    void addDiagnostics(const QList<Diagnostic> &diagnostics);
    QList<Diagnostic> diagnostics() const;

    enum ItemRole {
        DiagnosticRole = Analyzer::DetailedErrorView::FullTextRole + 1
    };
};

class ClangStaticAnalyzerDiagnosticFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    ClangStaticAnalyzerDiagnosticFilterModel(QObject *parent = 0);

    void setProject(ProjectExplorer::Project *project);
    void addSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    ProjectExplorer::Project *project() const { return m_project; }

private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    void handleSuppressedDiagnosticsChanged();

    QPointer<ProjectExplorer::Project> m_project;
    Utils::FileName m_lastProjectDirectory;
    SuppressedDiagnosticsList m_suppressedDiagnostics;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERDIAGNOSTICMODEL_H
