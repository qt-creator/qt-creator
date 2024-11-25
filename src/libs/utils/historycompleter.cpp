// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "historycompleter.h"

#include "qtcassert.h"
#include "qtcsettings.h"
#include "utilsicons.h"

#include <QItemDelegate>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QWindow>

namespace Utils {
namespace Internal {

static QtcSettings *theSettings = nullptr;
const bool isLastItemEmptyDefault = false;

class HistoryCompleterPrivate : public QAbstractListModel
{
public:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    void clearHistory();
    void addEntry(const QString &str);

    QStringList list;
    Key historyKey;
    Key historyKeyIsLastItemEmpty;
    int maxLines;
    bool isLastItemEmpty = isLastItemEmptyDefault;
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
        const qreal devicePixelRatio = painter->device()->devicePixelRatio();
        const QPixmap iconPixmap = icon.pixmap(option.rect.size(), devicePixelRatio);
        QRect pixmapRect = QStyle::alignedRect(option.direction,
                                               Qt::AlignRight | Qt::AlignVCenter,
                                               iconPixmap.size() / devicePixelRatio,
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
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setSelectionMode(QAbstractItemView::SingleSelection);
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
            int rr = event->position().x();
            if (layoutDirection() == Qt::LeftToRight)
                rr = viewport()->width() - event->position().x();
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
    theSettings->setValueWithDefault(historyKey, list);
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
        theSettings->setValueWithDefault(historyKeyIsLastItemEmpty,
                                         isLastItemEmpty,
                                         isLastItemEmptyDefault);
        return;
    }
    int removeIndex = list.indexOf(entry);
    beginResetModel();
    if (removeIndex != -1)
        list.removeAt(removeIndex);
    list.prepend(entry);
    list = list.mid(0, maxLines - 1);
    endResetModel();
    theSettings->setValueWithDefault(historyKey, list);
    isLastItemEmpty = false;
    theSettings->setValueWithDefault(historyKeyIsLastItemEmpty,
                                     isLastItemEmpty,
                                     isLastItemEmptyDefault);
}

HistoryCompleter::HistoryCompleter(const Key &historyKey, int maxLines, QObject *parent)
    : QCompleter(parent)
    , d(new HistoryCompleterPrivate)
{
    QTC_ASSERT(!historyKey.isEmpty(), return);
    QTC_ASSERT(theSettings, return);

    d->maxLines = maxLines;
    d->historyKey = "CompleterHistory/" + historyKey;
    d->list = theSettings->value(d->historyKey).toStringList();
    d->historyKeyIsLastItemEmpty = "CompleterHistory/" + historyKey + ".IsLastItemEmpty";
    d->isLastItemEmpty = theSettings->value(d->historyKeyIsLastItemEmpty, isLastItemEmptyDefault)
                             .toBool();

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

bool HistoryCompleter::historyExistsFor(const Key &historyKey)
{
    QTC_ASSERT(theSettings, return false);
    const Key fullKey = "CompleterHistory/" + historyKey;
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

void HistoryCompleter::clearHistory()
{
    d->clearHistory();
}

void HistoryCompleter::addEntry(const QString &str)
{
    d->addEntry(str);
}

void HistoryCompleter::setSettings(QtcSettings *settings)
{
    Internal::theSettings = settings;
}

} // namespace Utils
