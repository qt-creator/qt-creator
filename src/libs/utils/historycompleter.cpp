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

#include "historycompleter.h"
#include "fancylineedit.h"
#include "theme/theme.h"
#include "utilsicons.h"

#include "qtcassert.h"

#include <QSettings>

#include <QItemDelegate>
#include <QKeyEvent>
#include <QListView>
#include <QPainter>
#include <QWindow>

namespace Utils {
namespace Internal {

static QSettings *theSettings = nullptr;

class HistoryCompleterPrivate : public QAbstractListModel
{
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    void clearHistory();
    void addEntry(const QString &str);

    QStringList list;
    QString historyKey;
    QString historyKeyIsLastItemEmpty;
    int maxLines = 6;
    bool isLastItemEmpty = false;
};

class HistoryLineDelegate : public QItemDelegate
{
public:
    HistoryLineDelegate(QAbstractItemView *parent)
        : QItemDelegate(parent)
        , view(parent)
        , icon(Icons::EDIT_CLEAR.icon())
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        // from QHistoryCompleter
        QStyleOptionViewItem optCopy = option;
        optCopy.showDecorationSelected = true;
        if (view->currentIndex() == index)
            optCopy.state |= QStyle::State_HasFocus;
        QItemDelegate::paint(painter,option,index);
        // add remove button
        QWindow *window = view->window()->windowHandle();
        const QPixmap iconPixmap = icon.pixmap(window, option.rect.size());
        QRect pixmapRect = QStyle::alignedRect(option.direction,
                                               Qt::AlignRight | Qt::AlignVCenter,
                                               iconPixmap.size() / window->devicePixelRatio(),
                                               option.rect);
        if (!clearIconSize.isValid())
            clearIconSize = pixmapRect.size();
        painter->drawPixmap(pixmapRect, iconPixmap);
    }

    QAbstractItemView *view;
    QIcon icon;
    mutable QSize clearIconSize;
};

class HistoryLineView : public QListView
{
public:
    HistoryLineView(HistoryCompleterPrivate *model_)
        : model(model_)
    {
    }

    void installDelegate()
    {
        delegate = new HistoryLineDelegate(this);
        setItemDelegate(delegate);
    }

private:
    void mousePressEvent(QMouseEvent *event) override
    {
        const QSize clearButtonSize = delegate->clearIconSize;
        if (clearButtonSize.isValid()) {
            int rr = event->x();
            if (layoutDirection() == Qt::LeftToRight)
                rr = viewport()->width() - event->x();
            if (rr < clearButtonSize.width()) {
                const QModelIndex index = indexAt(event->pos());
                if (index.isValid()) {
                    model->removeRow(indexAt(event->pos()).row());
                    return;
                }
            }
        }
        QListView::mousePressEvent(event);
    }

    HistoryCompleterPrivate *model;
    HistoryLineDelegate *delegate;
};

} // namespace Internal

using namespace Internal;

int HistoryCompleterPrivate::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : list.count();
}

QVariant HistoryCompleterPrivate::data(const QModelIndex &index, int role) const
{
    if (index.row() >= list.count() || index.column() != 0)
        return QVariant();
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return list.at(index.row());
    return QVariant();
}

bool HistoryCompleterPrivate::removeRows(int row, int count, const QModelIndex &parent)
{
    QTC_ASSERT(theSettings, return false);
    if (row + count > list.count())
        return false;
    beginRemoveRows(parent, row, row + count -1);
    for (int i = 0; i < count; ++i)
        list.removeAt(row);
    theSettings->setValue(historyKey, list);
    endRemoveRows();
    return true;
}

void HistoryCompleterPrivate::clearHistory()
{
    beginResetModel();
    list.clear();
    endResetModel();
}

void HistoryCompleterPrivate::addEntry(const QString &str)
{
    QTC_ASSERT(theSettings, return);
    const QString entry = str.trimmed();
    if (entry.isEmpty()) {
        isLastItemEmpty = true;
        theSettings->setValue(historyKeyIsLastItemEmpty, isLastItemEmpty);
        return;
    }
    int removeIndex = list.indexOf(entry);
    beginResetModel();
    if (removeIndex != -1)
        list.removeAt(removeIndex);
    list.prepend(entry);
    list = list.mid(0, maxLines - 1);
    endResetModel();
    theSettings->setValue(historyKey, list);
    isLastItemEmpty = false;
    theSettings->setValue(historyKeyIsLastItemEmpty, isLastItemEmpty);
}

HistoryCompleter::HistoryCompleter(const QString &historyKey, QObject *parent)
    : QCompleter(parent),
      d(new HistoryCompleterPrivate)
{
    QTC_ASSERT(!historyKey.isEmpty(), return);
    QTC_ASSERT(theSettings, return);

    d->historyKey = QLatin1String("CompleterHistory/") + historyKey;
    d->list = theSettings->value(d->historyKey).toStringList();
    d->historyKeyIsLastItemEmpty = QLatin1String("CompleterHistory/")
        + historyKey + QLatin1String(".IsLastItemEmpty");
    d->isLastItemEmpty = theSettings->value(d->historyKeyIsLastItemEmpty, false).toBool();

    setModel(d);
    auto popup = new HistoryLineView(d);
    setPopup(popup);
    // setPopup unconditionally sets a delegate on the popup,
    // so we need to set our delegate afterwards
    popup->installDelegate();
}

bool HistoryCompleter::removeHistoryItem(int index)
{
    return d->removeRow(index);
}

QString HistoryCompleter::historyItem() const
{
    if (historySize() == 0 || d->isLastItemEmpty)
        return QString();
    return d->list.at(0);
}

bool HistoryCompleter::historyExistsFor(const QString &historyKey)
{
    QTC_ASSERT(theSettings, return false);
    const QString fullKey = QLatin1String("CompleterHistory/") + historyKey;
    return theSettings->value(fullKey).isValid();
}

HistoryCompleter::~HistoryCompleter()
{
    delete d;
}

int HistoryCompleter::historySize() const
{
    return d->rowCount();
}

int HistoryCompleter::maximalHistorySize() const
{
    return d->maxLines;
}

void HistoryCompleter::setMaximalHistorySize(int numberOfEntries)
{
    d->maxLines = numberOfEntries;
}

void HistoryCompleter::clearHistory()
{
    d->clearHistory();
}

void HistoryCompleter::addEntry(const QString &str)
{
    d->addEntry(str);
}

void HistoryCompleter::setSettings(QSettings *settings)
{
    Internal::theSettings = settings;
}

} // namespace Utils
