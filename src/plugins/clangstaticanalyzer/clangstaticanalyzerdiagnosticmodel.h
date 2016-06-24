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

#include "clangstaticanalyzerdiagnostic.h"
#include "clangstaticanalyzerprojectsettings.h"

#include <debugger/analyzer/detailederrorview.h>
#include <utils/fileutils.h>
#include <utils/treemodel.h>

#include <QPointer>
#include <QSortFilterProxyModel>

namespace ProjectExplorer { class Project; }

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerDiagnosticModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    ClangStaticAnalyzerDiagnosticModel(QObject *parent = 0);

    void addDiagnostics(const QList<Diagnostic> &diagnostics);
    QList<Diagnostic> diagnostics() const;

    enum ItemRole {
        DiagnosticRole = Debugger::DetailedErrorView::FullTextRole + 1
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
