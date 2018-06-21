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

#include "clangtoolsdiagnosticview.h"

#include "clangtoolsdiagnosticmodel.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolsutils.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QDebug>
#include <QHeaderView>

using namespace Debugger;

namespace ClangTools {
namespace Internal {

DiagnosticView::DiagnosticView(QWidget *parent)
    : Debugger::DetailedErrorView(parent)
{
    m_suppressAction = new QAction(tr("Suppress This Diagnostic"), this);
    connect(m_suppressAction, &QAction::triggered,
            this, &DiagnosticView::suppressCurrentDiagnostic);
    installEventFilter(this);
}

void DiagnosticView::suppressCurrentDiagnostic()
{
    const QModelIndexList indexes = selectionModel()->selectedRows();
    QTC_ASSERT(indexes.count() == 1, return);
    const Diagnostic diag = model()->data(indexes.first(),
                                          ClangToolsDiagnosticModel::DiagnosticRole)
            .value<Diagnostic>();
    QTC_ASSERT(diag.isValid(), return);

    // If the original project was closed, we work directly on the filter model, otherwise
    // we go via the project settings.
    auto * const filterModel = static_cast<DiagnosticFilterModel *>(model());
    ProjectExplorer::Project * const project = filterModel->project();
    if (project) {
        Utils::FileName filePath = Utils::FileName::fromString(diag.location.filePath);
        const Utils::FileName relativeFilePath
                = filePath.relativeChildPath(project->projectDirectory());
        if (!relativeFilePath.isEmpty())
            filePath = relativeFilePath;
        const SuppressedDiagnostic supDiag(filePath, diag.description, diag.issueContextKind,
                                           diag.issueContext, diag.explainingSteps.count());
        ClangToolsProjectSettingsManager::getSettings(project)->addSuppressedDiagnostic(supDiag);
    } else {
        filterModel->addSuppressedDiagnostic(SuppressedDiagnostic(diag));
    }
}

QList<QAction *> DiagnosticView::customActions() const
{
    return QList<QAction *>() << m_suppressAction;
}

bool DiagnosticView::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyRelease: {
        const int key = static_cast<QKeyEvent *>(event)->key();
        switch (key) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            const QModelIndex current = currentIndex();
            const QModelIndex location = model()->index(current.row(),
                                                        LocationColumn,
                                                        current.parent());
            emit clicked(location);
        }
        return true;
    }
    default:
        return QObject::eventFilter(watched, event);
    }
}

void DiagnosticView::setModel(QAbstractItemModel *model)
{
    Debugger::DetailedErrorView::setModel(model);
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

} // namespace Internal
} // namespace ClangTools
