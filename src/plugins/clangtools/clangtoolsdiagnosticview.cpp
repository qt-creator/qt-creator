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

#include <coreplugin/editormanager/editormanager.h>

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
    ClickableFixItHeader(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QHeaderView(orientation, parent)
    {
        setDefaultAlignment(Qt::AlignLeft);
    }

    void setState(QFlags<QStyle::StateFlag> newState)
    {
        state = newState;
    }

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override
    {
        painter->save();
        QHeaderView::paintSection(painter, rect, logicalIndex);
        painter->restore();
        if (logicalIndex == DiagnosticView::FixItColumn) {
            QStyleOptionButton option;
            const int side = sizeHint().height();
            option.rect = QRect(rect.left() + 1, 1, side - 3, side - 3);
            option.state = state;
            painter->save();
            const int shift = side - 2;
            painter->translate(QPoint(shift, 0));
            QHeaderView::paintSection(painter, rect.adjusted(0, 0, -shift, 0), logicalIndex);
            painter->restore();
            style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter);
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        const int x = event->localPos().x();
        const int fixItColumnX = sectionPosition(DiagnosticView::FixItColumn);
        const bool isWithinFixitCheckBox = x > fixItColumnX
                                           && x < fixItColumnX + sizeHint().height() - 3;
        if (isWithinFixitCheckBox) {
            state = (state != QStyle::State_On) ? QStyle::State_On : QStyle::State_Off;
            viewport()->update();
            emit fixItColumnClicked(state == QStyle::State_On);
            return; // Avoid changing sort order
        }
        QHeaderView::mouseReleaseEvent(event);
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

void DiagnosticView::goNext()
{
    const QModelIndex currentIndex = selectionModel()->currentIndex();
    selectIndex(getIndex(currentIndex, Next));
}

void DiagnosticView::goBack()
{
    const QModelIndex currentIndex = selectionModel()->currentIndex();
    selectIndex(getIndex(currentIndex, Previous));
}

QModelIndex DiagnosticView::getIndex(const QModelIndex &index, Direction direction) const
{
    const QModelIndex parentIndex = index.parent();
    QModelIndex followingTopIndex = index;

    if (parentIndex.isValid()) {
        // Use direct sibling for level 2 and 3 items
        const QModelIndex followingIndex = index.sibling(index.row() + direction, 0);
        if (followingIndex.isValid())
            return followingIndex;

        // First/Last level 3 item? Continue on level 2 item.
        if (parentIndex.parent().isValid())
            return direction == Previous ? parentIndex : getIndex(parentIndex, direction);

        followingTopIndex = getTopLevelIndex(parentIndex, direction);
    }

    // Skip top level items without children
    while (!model()->hasChildren(followingTopIndex))
        followingTopIndex = getTopLevelIndex(followingTopIndex, direction);

    // Select first/last level 2 item
    const int row = direction == Next ? 0 : model()->rowCount(followingTopIndex) - 1;
    return model()->index(row, 0, followingTopIndex);
}

QModelIndex DiagnosticView::getTopLevelIndex(const QModelIndex &index, Direction direction) const
{
    QModelIndex following = index.sibling(index.row() + direction, 0);
    if (following.isValid())
        return following;
    const int row = direction == Next ? 0 : model()->rowCount() - 1;
    return model()->index(row, 0);
}

QList<QAction *> DiagnosticView::customActions() const
{
    return {m_suppressAction};
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
            openEditorForCurrentIndex();
        }
        return true;
    }
    default:
        return QObject::eventFilter(watched, event);
    }
}

void DiagnosticView::mouseDoubleClickEvent(QMouseEvent *event)
{
    openEditorForCurrentIndex();
    Debugger::DetailedErrorView::mouseDoubleClickEvent(event);
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

void DiagnosticView::openEditorForCurrentIndex()
{
    const QVariant v = model()->data(currentIndex(), Debugger::DetailedErrorView::LocationRole);
    const auto loc = v.value<Debugger::DiagnosticLocation>();
    if (loc.isValid())
        Core::EditorManager::openEditorAt(loc.filePath, loc.line, loc.column - 1);
}

void DiagnosticView::setModel(QAbstractItemModel *theProxyModel)
{
    const auto proxyModel = static_cast<QSortFilterProxyModel *>(theProxyModel);
    const auto sourceModel = static_cast<ClangToolsDiagnosticModel *>(proxyModel->sourceModel());

    Debugger::DetailedErrorView::setModel(proxyModel);
    auto *clickableFixItHeader = new ClickableFixItHeader(Qt::Horizontal, this);
    connect(clickableFixItHeader, &ClickableFixItHeader::fixItColumnClicked, this, [=](bool checked) {
        m_ignoreSetSelectedFixItsCount = true;
        sourceModel->rootItem()->forChildrenAtLevel(2, [&](::Utils::TreeItem *item) {
            auto diagnosticItem = static_cast<DiagnosticItem *>(item);
            diagnosticItem->setData(FixItColumn,
                                    checked ? Qt::Checked : Qt::Unchecked,
                                    Qt::CheckStateRole);
        });
        m_ignoreSetSelectedFixItsCount = false;
    });
    setHeader(clickableFixItHeader);
    clickableFixItHeader->setStretchLastSection(false);
    clickableFixItHeader->setSectionResizeMode(0, QHeaderView::Stretch);
    clickableFixItHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    const int fixitColumnWidth = clickableFixItHeader->sectionSizeHint(DiagnosticView::FixItColumn);
    const int checkboxWidth = clickableFixItHeader->height();
    clickableFixItHeader->setMinimumSectionSize(fixitColumnWidth + 1.2 * checkboxWidth);
}

} // namespace Internal
} // namespace ClangTools

#include "clangtoolsdiagnosticview.moc"
