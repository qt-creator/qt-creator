/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "historycompleter.h"
#include "qtcassert.h"

#include <QAbstractListModel>
#include <QSettings>

#include <QItemDelegate>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QStyle>

namespace Utils {
namespace Internal {

static QSettings *theSettings = 0;

class HistoryCompleterPrivate : public QAbstractListModel
{
public:
    HistoryCompleterPrivate(HistoryCompleter *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

    void fetchHistory();
    void clearHistory();
    void saveEntry(const QString &str);
    bool eventFilter(QObject *obj, QEvent *event);

    QStringList list;
    QString historyKey;
    HistoryCompleter *completer;
    QWidget *lastSeenWidget;
    int maxLines;
};

class HistoryLineDelegate : public QItemDelegate
{
public:
    HistoryLineDelegate()
        : pixmap(QLatin1String(":/core/images/editclear.png"))
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QItemDelegate::paint(painter,option,index);
        QRect r = QStyle::alignedRect(option.direction, Qt::AlignRight | Qt::AlignVCenter , pixmap.size(), option.rect);
        painter->drawPixmap(r, pixmap);
    }

    QPixmap pixmap;
};

class HistoryLineView : public QListView
{
public:
    HistoryLineView(HistoryCompleterPrivate *model_, int pixmapWith_)
        : model(model_) , pixmapWidth(pixmapWith_)
    {}

private:
    void mousePressEvent(QMouseEvent *event)
    {
        int rr= event->x();
        if (layoutDirection() == Qt::LeftToRight)
            rr = viewport()->width() - event->x();
        if (rr < pixmapWidth) {
            model->removeRow(indexAt(event->pos()).row());
            return;
        }
        QListView::mousePressEvent(event);
    }

    HistoryCompleterPrivate *model;
    int pixmapWidth;
};

} // namespace Internal

using namespace Internal;

HistoryCompleterPrivate::HistoryCompleterPrivate(HistoryCompleter *parent)
    : QAbstractListModel(parent)
    , completer(parent)
    , lastSeenWidget(0)
    , maxLines(30)
{
}

void HistoryCompleterPrivate::fetchHistory()
{
    QTC_ASSERT(theSettings, return);
    if (!completer->widget()) {
        list.clear();
        reset();
        return;
    }
    list = theSettings->value(historyKey).toStringList();
    reset();
}

int HistoryCompleterPrivate::rowCount(const QModelIndex &parent) const
{
    if (lastSeenWidget != completer->widget()) {
        if (lastSeenWidget)
            lastSeenWidget->removeEventFilter(const_cast<HistoryCompleterPrivate *>(this));
        completer->widget()->installEventFilter(const_cast<HistoryCompleterPrivate *>(this));
        if (qobject_cast<QLineEdit *>(lastSeenWidget))
            // this will result in spamming the history with garbage in some corner cases.
            // not my idea.
            disconnect(lastSeenWidget, SIGNAL(editingFinished()), completer, SLOT(saveHistory()));
        HistoryCompleterPrivate *that = const_cast<HistoryCompleterPrivate *>(this);
        that->lastSeenWidget = completer->widget();
        that->fetchHistory();
        if (qobject_cast<QLineEdit *>(lastSeenWidget))
            connect(lastSeenWidget, SIGNAL(editingFinished()), completer, SLOT(saveHistory()));
    }
    if (parent.isValid())
        return 0;
    return list.count();
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
    beginRemoveRows (parent, row, row + count);
    list.removeAt(row);
    theSettings->setValue(historyKey, list);
    endRemoveRows();
    return true;
}

void HistoryCompleterPrivate::clearHistory()
{
    list.clear();
    reset();
}

void HistoryCompleterPrivate::saveEntry(const QString &str)
{
    QTC_ASSERT(theSettings, return);
    if (str.isEmpty())
        return;
    if (list.contains(str))
        return;
    if (!completer->widget())
        return;
    if (lastSeenWidget != completer->widget()) {
        if (lastSeenWidget)
            lastSeenWidget->removeEventFilter(this);
        completer->widget()->installEventFilter(this);
        fetchHistory();
        lastSeenWidget = completer->widget();
    }
    beginInsertRows (QModelIndex(), list.count(), list.count());
    list.prepend(str);
    list = list.mid(0, maxLines);
    endInsertRows();
    theSettings->setValue(historyKey, list);
}

bool HistoryCompleterPrivate::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Down) {
        completer->setCompletionPrefix(QString());
        completer->complete();
    }
    return QAbstractListModel::eventFilter(obj,event);
}

HistoryCompleter::HistoryCompleter(QObject *parent, const QString &historyKey)
    : QCompleter(parent)
    , d(new HistoryCompleterPrivate(this))
{
    QTC_ASSERT(!historyKey.isEmpty(), return);

    // make an assumption to allow pressing of the down
    // key, before the first model run:
    // parent is likely the lineedit

    d->historyKey = QLatin1String("CompleterHistory/") + historyKey;

    parent->installEventFilter(d);
    QTC_ASSERT(theSettings, return);
    d->list = theSettings->value(d->historyKey).toStringList();

    QLineEdit *l = qobject_cast<QLineEdit *>(parent);
    if (l && d->list.count())
        l->setText(d->list.at(0));

    setModel(d);
    HistoryLineDelegate *delegate = new HistoryLineDelegate;
    HistoryLineView *view = new HistoryLineView(d, delegate->pixmap.width());
    setPopup(view);
    view->setItemDelegate(delegate);
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

void HistoryCompleter::saveHistory()
{
    d->saveEntry(completionPrefix());
}

void HistoryCompleter::setSettings(QSettings *settings)
{
    Internal::theSettings = settings;
}

} // namespace Utils
