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

#include "clangstaticanalyzerdiagnosticview.h"

#include "clangstaticanalyzerdiagnosticmodel.h"
#include "clangstaticanalyzerprojectsettings.h"
#include "clangstaticanalyzerprojectsettingsmanager.h"
#include "clangstaticanalyzerutils.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QDebug>

using namespace Analyzer;

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerDiagnosticView::ClangStaticAnalyzerDiagnosticView(QWidget *parent)
    : Analyzer::DetailedErrorView(parent)
{
    m_suppressAction = new QAction(tr("Suppress this diagnostic"), this);
    connect(m_suppressAction, &QAction::triggered, [this](bool) { suppressCurrentDiagnostic(); });
}

void ClangStaticAnalyzerDiagnosticView::suppressCurrentDiagnostic()
{
    const QModelIndexList indexes = selectionModel()->selectedRows();
    QTC_ASSERT(indexes.count() == 1, return);
    const Diagnostic diag = model()->data(indexes.first(),
                                          ClangStaticAnalyzerDiagnosticModel::DiagnosticRole)
            .value<Diagnostic>();
    QTC_ASSERT(diag.isValid(), return);

    // If the original project was closed, we work directly on the filter model, otherwise
    // we go via the project settings.
    auto * const filterModel = static_cast<ClangStaticAnalyzerDiagnosticFilterModel *>(model());
    ProjectExplorer::Project * const project = filterModel->project();
    if (project) {
        Utils::FileName filePath = Utils::FileName::fromString(diag.location.filePath);
        const Utils::FileName relativeFilePath
                = filePath.relativeChildPath(project->projectDirectory());
        if (!relativeFilePath.isEmpty())
            filePath = relativeFilePath;
        const SuppressedDiagnostic supDiag(filePath, diag.description, diag.issueContextKind,
                                           diag.issueContext, diag.explainingSteps.count());
        ProjectSettingsManager::getSettings(project)->addSuppressedDiagnostic(supDiag);
    } else {
        filterModel->addSuppressedDiagnostic(SuppressedDiagnostic(diag));
    }
}

QList<QAction *> ClangStaticAnalyzerDiagnosticView::customActions() const
{
    return QList<QAction *>() << m_suppressAction;
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
