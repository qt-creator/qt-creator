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

#include "detailederrorview.h"

#include "diagnosticlocation.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAbstractTextDocumentLayout>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QSharedPointer>
#include <QTextDocument>

namespace Debugger {
namespace Internal {

class DetailedErrorDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    DetailedErrorDelegate(QTreeView *parent) : QStyledItemDelegate(parent) { }

private:
    QString actualText(const QModelIndex &index) const
    {
        const auto location = index.model()->data(index, DetailedErrorView::LocationRole)
                .value<DiagnosticLocation>();
        return location.isValid()
                ? QString::fromLatin1("<a href=\"file://%1\">%2:%3")
                      .arg(location.filePath, QFileInfo(location.filePath).fileName())
                      .arg(location.line)
                : QString();
    }

    using DocConstPtr = QSharedPointer<const QTextDocument>;
    DocConstPtr document(const QStyleOptionViewItem &option) const
    {
        const auto doc = QSharedPointer<QTextDocument>::create();
        doc->setHtml(option.text);
        doc->setTextWidth(option.rect.width());
        doc->setDocumentMargin(0);
        return doc;
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        opt.text = actualText(index);
        initStyleOption(&opt, index);

        const DocConstPtr doc = document(opt);
        return QSize(doc->idealWidth(), doc->size().height());
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QStyle *style = opt.widget? opt.widget->style() : QApplication::style();

        // Painting item without text
        opt.text.clear();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter);
        opt.text = actualText(index);

        QAbstractTextDocumentLayout::PaintContext ctx;

        // Highlighting text if item is selected
        if (opt.state & QStyle::State_Selected) {
            ctx.palette.setColor(QPalette::Text, opt.palette.color(QPalette::Active,
                                                                   QPalette::HighlightedText));
        }

        QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt);
        painter->save();
        painter->translate(textRect.topLeft());
        painter->setClipRect(textRect.translated(-textRect.topLeft()));
        document(opt)->documentLayout()->draw(painter, ctx);
        painter->restore();
    }
};

} // namespace Internal


DetailedErrorView::DetailedErrorView(QWidget *parent) :
    QTreeView(parent),
    m_copyAction(new QAction(this))
{
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setItemDelegateForColumn(LocationColumn, new Internal::DetailedErrorDelegate(this));

    m_copyAction->setText(tr("Copy"));
    m_copyAction->setIcon(Utils::Icons::COPY.icon());
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_copyAction, &QAction::triggered, [this] {
        const QModelIndexList selectedRows = selectionModel()->selectedRows();
        QTC_ASSERT(selectedRows.count() == 1, return);
        QApplication::clipboard()->setText(model()->data(selectedRows.first(),
                                                         FullTextRole).toString());
    });
    connect(this, &QAbstractItemView::clicked, [](const QModelIndex &index) {
        if (index.column() == LocationColumn) {
            const auto loc = index.model()->data(index, DetailedErrorView::LocationRole)
                    .value<DiagnosticLocation>();
            if (loc.isValid())
                Core::EditorManager::openEditorAt(loc.filePath, loc.line, loc.column - 1);
        }
    });

    addAction(m_copyAction);
}

DetailedErrorView::~DetailedErrorView()
{
}

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
    return QList<QAction *>();
}

int DetailedErrorView::currentRow() const
{
    const QModelIndex index = selectionModel()->currentIndex();
    return index.row();
}

void DetailedErrorView::setCurrentRow(int row)
{
    const QModelIndex index = model()->index(row, 0);
    selectionModel()->setCurrentIndex(index,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

} // namespace Debugger

#include "detailederrorview.moc"
