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
#include <QPainter>

using namespace Debugger;

namespace ClangTools {
namespace Internal {

class ClickableFixItHeader : public QHeaderView
{
    Q_OBJECT

public:
    ClickableFixItHeader(Qt::Orientation orientation, QWidget *parent = 0)
        : QHeaderView(orientation, parent)
    {
    }

    void setState(QFlags<QStyle::StateFlag> newState)
    {
        state = newState;
    }

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
    {
        painter->save();
        QHeaderView::paintSection(painter, rect, logicalIndex);
        painter->restore();
        if (logicalIndex == DiagnosticView::FixItColumn) {
            QStyleOptionButton option;
            const int side = sizeHint().height();
            option.rect = QRect(rect.left() + 1, 1, side - 3, side - 3);
            option.state = state;
            style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter);
        }
    }

    void mousePressEvent(QMouseEvent *event)
    {
        if (event->localPos().x() > sectionPosition(DiagnosticView::FixItColumn)) {
            state = (state != QStyle::State_On) ? QStyle::State_On : QStyle::State_Off;
            viewport()->update();
            emit fixItColumnClicked(state == QStyle::State_On);
        }
        QHeaderView::mousePressEvent(event);
    }

signals:
    void fixItColumnClicked(bool checked);

private:
    QFlags<QStyle::StateFlag> state = QStyle::State_Off;
};

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

void DiagnosticView::setSelectedFixItsCount(int fixItsCount)
{
    if (m_ignoreSetSelectedFixItsCount)
        return;
    auto *clickableFixItHeader = static_cast<ClickableFixItHeader *>(header());
    clickableFixItHeader->setState(fixItsCount
                                   ? (QStyle::State_NoChange | QStyle::State_On | QStyle::State_Off)
                                   : QStyle::State_Off);
    clickableFixItHeader->viewport()->update();
}

void DiagnosticView::setModel(QAbstractItemModel *model)
{
    Debugger::DetailedErrorView::setModel(model);
    auto *clickableFixItHeader = new ClickableFixItHeader(Qt::Horizontal, this);
    connect(clickableFixItHeader, &ClickableFixItHeader::fixItColumnClicked,
            this, [=](bool checked) {
        m_ignoreSetSelectedFixItsCount = true;
        for (int row = 0; row < model->rowCount(); ++row) {
            QModelIndex index = model->index(row, FixItColumn, QModelIndex());
            model->setData(index, checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
        }
        m_ignoreSetSelectedFixItsCount = false;
    });
    setHeader(clickableFixItHeader);
    clickableFixItHeader->setStretchLastSection(false);
    clickableFixItHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    clickableFixItHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    clickableFixItHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
}

} // namespace Internal
} // namespace ClangTools

#include "clangtoolsdiagnosticview.moc"
