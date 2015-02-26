/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "detailederrorview.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/qtcassert.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QMenu>
#include <QPainter>
#include <QScrollBar>

namespace Analyzer {

DetailedErrorDelegate::DetailedErrorDelegate(QListView *parent)
    : QStyledItemDelegate(parent),
      m_detailsWidget(0)
{
    connect(parent->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &DetailedErrorDelegate::onVerticalScroll);
}

QSize DetailedErrorDelegate::sizeHint(const QStyleOptionViewItem &opt,
                                      const QModelIndex &index) const
{
    if (!index.isValid())
        return QStyledItemDelegate::sizeHint(opt, index);

    const QListView *view = qobject_cast<const QListView *>(parent());
    const int viewportWidth = view->viewport()->width();
    const bool isSelected = view->selectionModel()->currentIndex() == index;
    const int dy = 2 * s_itemMargin;

    if (!isSelected) {
        QFontMetrics fm(opt.font);
        return QSize(viewportWidth, fm.height() + dy);
    }

    if (m_detailsWidget && m_detailsIndex != index) {
        m_detailsWidget->deleteLater();
        m_detailsWidget = 0;
    }

    if (!m_detailsWidget) {
        m_detailsWidget = createDetailsWidget(opt.font, index, view->viewport());
        QTC_ASSERT(m_detailsWidget->parent() == view->viewport(),
                   m_detailsWidget->setParent(view->viewport()));
        m_detailsIndex = index;
    } else {
        QTC_ASSERT(m_detailsIndex == index, /**/);
    }
    const int widthExcludingMargins = viewportWidth - 2 * s_itemMargin;
    m_detailsWidget->setFixedWidth(widthExcludingMargins);

    m_detailsWidgetHeight = m_detailsWidget->heightForWidth(widthExcludingMargins);
    // HACK: it's a bug in QLabel(?) that we have to force the widget to have the size it said
    //       it would have.
    m_detailsWidget->setFixedHeight(m_detailsWidgetHeight);
    return QSize(viewportWidth, dy + m_detailsWidget->heightForWidth(widthExcludingMargins));
}

void DetailedErrorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &basicOption,
                                  const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt(basicOption);
    initStyleOption(&opt, index);

    const QListView *const view = qobject_cast<const QListView *>(parent());
    const bool isSelected = view->selectionModel()->currentIndex() == index;

    QFontMetrics fm(opt.font);
    QPoint pos = opt.rect.topLeft();

    painter->save();

    const QColor bgColor = isSelected
            ? opt.palette.highlight().color()
            : opt.palette.background().color();
    painter->setBrush(bgColor);

    // clear background
    painter->setPen(Qt::NoPen);
    painter->drawRect(opt.rect);

    pos.rx() += s_itemMargin;
    pos.ry() += s_itemMargin;

    if (isSelected) {
        // only show detailed widget and let it handle everything
        QTC_ASSERT(m_detailsIndex == index, /**/);
        QTC_ASSERT(m_detailsWidget, return); // should have been set in sizeHint()
        m_detailsWidget->move(pos);
        // when scrolling quickly, the widget can get stuck in a visible part of the scroll area
        // even though it should not be visible. therefore we hide it every time the scroll value
        // changes and un-hide it when the item with details widget is paint()ed, i.e. visible.
        m_detailsWidget->show();

        const int viewportWidth = view->viewport()->width();
        const int widthExcludingMargins = viewportWidth - 2 * s_itemMargin;
        QTC_ASSERT(m_detailsWidget->width() == widthExcludingMargins, /**/);
        QTC_ASSERT(m_detailsWidgetHeight == m_detailsWidget->height(), /**/);
    } else {
        // the reference coordinate for text drawing is the text baseline; move it inside the view rect.
        pos.ry() += fm.ascent();

        const QColor textColor = opt.palette.text().color();
        painter->setPen(textColor);
        // draw only text + location

        const SummaryLineInfo info = summaryInfo(index);
        const QString errorText = info.errorText;
        painter->drawText(pos, errorText);

        const int whatWidth = QFontMetrics(opt.font).width(errorText);
        const int space = 10;
        const int widthLeft = opt.rect.width() - (pos.x() + whatWidth + space + s_itemMargin);
        if (widthLeft > 0) {
            QFont monospace = opt.font;
            monospace.setFamily(QLatin1String("monospace"));
            QFontMetrics metrics(monospace);
            QColor nameColor = textColor;
            nameColor.setAlphaF(0.7);

            painter->setFont(monospace);
            painter->setPen(nameColor);

            QPoint namePos = pos;
            namePos.rx() += whatWidth + space;
            painter->drawText(namePos, metrics.elidedText(info.errorLocation, Qt::ElideLeft,
                                                          widthLeft));
        }
    }

    // Separator lines (like Issues pane)
    painter->setPen(QColor::fromRgb(150,150,150));
    painter->drawLine(0, opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());

    painter->restore();
}

void DetailedErrorDelegate::onCurrentSelectionChanged(const QModelIndex &now,
                                                      const QModelIndex &previous)
{
    if (m_detailsWidget) {
        m_detailsWidget->deleteLater();
        m_detailsWidget = 0;
    }

    m_detailsIndex = QModelIndex();
    if (now.isValid())
        emit sizeHintChanged(now);
    if (previous.isValid())
        emit sizeHintChanged(previous);
}

void DetailedErrorDelegate::onLayoutChanged()
{
    if (m_detailsWidget) {
        m_detailsWidget->deleteLater();
        m_detailsWidget = 0;
        m_detailsIndex = QModelIndex();
    }
}

void DetailedErrorDelegate::onViewResized()
{
    const QListView *view = qobject_cast<const QListView *>(parent());
    if (m_detailsWidget)
        emit sizeHintChanged(view->selectionModel()->currentIndex());
}

void DetailedErrorDelegate::onVerticalScroll()
{
    if (m_detailsWidget)
        m_detailsWidget->hide();
}

// Expects "file://some/path[:line[:column]]" - the line/column part is optional
void DetailedErrorDelegate::openLinkInEditor(const QString &link)
{
    const QString linkWithoutPrefix = link.mid(int(strlen("file://")));
    const QChar separator = QLatin1Char(':');
    const int lineColon = linkWithoutPrefix.indexOf(separator, /*after drive letter + colon =*/ 2);
    const QString path = linkWithoutPrefix.left(lineColon);
    const QString lineColumn = linkWithoutPrefix.mid(lineColon + 1);
    const int line = lineColumn.section(separator, 0, 0).toInt();
    const int column = lineColumn.section(separator, 1, 1).toInt();
    Core::EditorManager::openEditorAt(path, qMax(line, 0), qMax(column, 0));
}

void DetailedErrorDelegate::copyToClipboard()
{
    QApplication::clipboard()->setText(textualRepresentation());
}

DetailedErrorView::DetailedErrorView(QWidget *parent) :
    QListView(parent),
    m_copyAction(0)
{
}

DetailedErrorView::~DetailedErrorView()
{
    itemDelegate()->deleteLater();
}

void DetailedErrorView::setItemDelegate(QAbstractItemDelegate *delegate)
{
    QListView::setItemDelegate(delegate);

    DetailedErrorDelegate *myDelegate = qobject_cast<DetailedErrorDelegate *>(itemDelegate());
    connect(this, &DetailedErrorView::resized, myDelegate, &DetailedErrorDelegate::onViewResized);

    m_copyAction = new QAction(this);
    m_copyAction->setText(tr("Copy"));
    m_copyAction->setIcon(QIcon(QLatin1String(Core::Constants::ICON_COPY)));
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_copyAction, &QAction::triggered, myDelegate, &DetailedErrorDelegate::copyToClipboard);
    addAction(m_copyAction);
}

void DetailedErrorView::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);

    DetailedErrorDelegate *delegate = qobject_cast<DetailedErrorDelegate *>(itemDelegate());
    QTC_ASSERT(delegate, return);

    connect(selectionModel(), &QItemSelectionModel::currentChanged,
            delegate, &DetailedErrorDelegate::onCurrentSelectionChanged);
    connect(model, &QAbstractItemModel::layoutChanged,
            delegate, &DetailedErrorDelegate::onLayoutChanged);
}

void DetailedErrorView::resizeEvent(QResizeEvent *e)
{
    emit resized();
    QListView::resizeEvent(e);
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

void DetailedErrorView::updateGeometries()
{
    if (model()) {
        QModelIndex index = model()->index(0, modelColumn(), rootIndex());
        QStyleOptionViewItem option = viewOptions();
        // delegate for row / column
        QSize step = itemDelegate()->sizeHint(option, index);
        horizontalScrollBar()->setSingleStep(step.width() + spacing());
        verticalScrollBar()->setSingleStep(step.height() + spacing());
    }
    QListView::updateGeometries();
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
    scrollTo(index);
}

} // namespace Analyzer
