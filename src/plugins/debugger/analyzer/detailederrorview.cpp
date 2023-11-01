// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "detailederrorview.h"

#include "../debuggertr.h"
#include "diagnosticlocation.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>

namespace Debugger {

DetailedErrorView::DetailedErrorView(QWidget *parent) :
    QTreeView(parent),
    m_copyAction(new QAction(this))
{
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    m_copyAction->setText(Tr::tr("Copy"));
    m_copyAction->setIcon(Utils::Icons::COPY.icon());
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_copyAction, &QAction::triggered, this, [this] {
        const QModelIndexList selectedRows = selectionModel()->selectedRows();
        QStringList data;
        for (const QModelIndex &index : selectedRows)
            data << model()->data(index, FullTextRole).toString();
        Utils::setClipboardAndSelection(data.join('\n'));
    });
    connect(this, &QAbstractItemView::clicked, [](const QModelIndex &index) {
        if (index.column() == LocationColumn) {
            const auto loc = index.model()->data(index, DetailedErrorView::LocationRole)
                    .value<DiagnosticLocation>();
            if (loc.isValid())
                Core::EditorManager::openEditorAt(Utils::Link(loc.filePath, loc.line, loc.column - 1));
        }
    });

    addAction(m_copyAction);
}

DetailedErrorView::~DetailedErrorView() = default;

void DetailedErrorView::contextMenuEvent(QContextMenuEvent *e)
{
    if (selectionModel()->selectedRows().isEmpty())
        return;

    QMenu menu;
    menu.addActions(commonActions());
    const QList<QAction *> custom = customActions();
    if (!custom.isEmpty()) {
        menu.addSeparator();
        menu.addActions(custom);
    }
    menu.exec(e->globalPos());
}

void DetailedErrorView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous);
    scrollTo(current);
}

void DetailedErrorView::goNext()
{
    QTC_ASSERT(rowCount(), return);
    setCurrentRow((currentRow() + 1) % rowCount());
}

void DetailedErrorView::goBack()
{
    QTC_ASSERT(rowCount(), return);
    const int prevRow = currentRow() - 1;
    setCurrentRow(prevRow >= 0 ? prevRow : rowCount() - 1);
}

void DetailedErrorView::selectIndex(const QModelIndex &index)
{
    selectionModel()->setCurrentIndex(index,
                                      QItemSelectionModel::ClearAndSelect
                                          | QItemSelectionModel::Rows);
}

QVariant DetailedErrorView::locationData(int role, const DiagnosticLocation &location)
{
    switch (role) {
    case Debugger::DetailedErrorView::LocationRole:
        return QVariant::fromValue(location);
    case Qt::DisplayRole:
        return location.isValid() ? QString::fromLatin1("%1:%2:%3")
                               .arg(location.filePath.fileName())
                               .arg(location.line)
                               .arg(location.column)
                         : QString();
    case Qt::ToolTipRole:
        return location.filePath.isEmpty() ? QVariant() : QVariant(location.filePath.toUserOutput());
    case Qt::FontRole: {
        QFont font = QApplication::font();
        font.setUnderline(true);
        return font;
    }
    case Qt::ForegroundRole:
        return QApplication::palette().link().color();
    default:
        return QVariant();
    }
}

int DetailedErrorView::rowCount() const
{
    return model() ? model()->rowCount() : 0;
}

QList<QAction *> DetailedErrorView::commonActions() const
{
    QList<QAction *> actions;
    actions << m_copyAction;
    return actions;
}

QList<QAction *> DetailedErrorView::customActions() const
{
    return {};
}

int DetailedErrorView::currentRow() const
{
    const QModelIndex index = selectionModel()->currentIndex();
    return index.row();
}

void DetailedErrorView::setCurrentRow(int row)
{
    selectIndex(model()->index(row, 0));
}

} // Debugger
