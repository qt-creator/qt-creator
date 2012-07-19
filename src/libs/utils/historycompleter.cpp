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

#include <QAbstractListModel>
#include <QSettings>

#include <QLineEdit>
#include <QKeyEvent>
#include <QItemDelegate>
#include <QListView>
#include <QPainter>
#include <QStyle>

static const char SETTINGS_PREFIX[] = "CompleterHistory/";

namespace Utils {
namespace Internal {

class HistoryListModel : public QAbstractListModel
{
public:
    HistoryListModel(HistoryCompleter *parent);
    void fetchHistory();
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    void clearHistory();
    void saveEntry(const QString &str);
    bool eventFilter(QObject *obj, QEvent *event);

    QStringList list;
    HistoryCompleter *completer;
    QWidget *lastSeenWidget;
    QSettings *settings;
    int maxLines;
};

class HistoryCompleterPrivate
{
public:
    HistoryCompleterPrivate(HistoryCompleter *parent);
    HistoryCompleter *q;
    HistoryListModel *model;
};

class HistoryLineDelegate : public QItemDelegate
{
public:
    HistoryLineDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QPixmap pixmap;
};

class HistoryLineView : public QListView
{
public:
    HistoryCompleterPrivate *d;
    int pixmapWidth;
    HistoryLineView(HistoryCompleterPrivate *d_, int pixmapWith_);
    virtual void mousePressEvent(QMouseEvent *event);
};

} // namespace Internal
} // namespace Utils

using namespace Utils;
using namespace Utils::Internal;

HistoryListModel::HistoryListModel(HistoryCompleter *parent)
    : QAbstractListModel(parent)
    , completer(parent)
    , lastSeenWidget(0)
    , settings(0)
    , maxLines(30)
{
}

void HistoryListModel::fetchHistory()
{
    if (!completer->widget() || !settings) {
        list.clear();
        reset();
        return;
    }
    QString objectName = completer->widget()->objectName();
    if (objectName.isEmpty())
        return;
    list = settings->value(QLatin1String(SETTINGS_PREFIX) + objectName).toStringList();
    reset();
}

int HistoryListModel::rowCount(const QModelIndex &parent) const
{
    if (lastSeenWidget != completer->widget()) {
        if (lastSeenWidget)
            const_cast<QWidget*>(lastSeenWidget)->removeEventFilter(const_cast<HistoryListModel *>(this));
        const_cast<QWidget*>(completer->widget())->installEventFilter(const_cast<HistoryListModel *>(this));
        if (qobject_cast<QLineEdit *>(lastSeenWidget))
            // this will result in spamming the history with garbage in some corner cases.
            // not my idea.
            disconnect(lastSeenWidget, SIGNAL(editingFinished()), completer, SLOT(saveHistory()));
        HistoryListModel *that = const_cast<HistoryListModel *>(this);
        that->lastSeenWidget = completer->widget();
        that->fetchHistory();
        if (qobject_cast<QLineEdit *>(lastSeenWidget))
            connect(lastSeenWidget, SIGNAL(editingFinished()), completer, SLOT(saveHistory()));
    }
    if (parent.isValid())
        return 0;
    return list.count();
}

QVariant HistoryListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= list.count() || index.column() != 0)
        return QVariant();
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return list.at(index.row());
    return QVariant();
}

bool HistoryListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows (parent, row, row + count);
    list.removeAt(row);
    if (settings) {
        QString objectName = completer->widget()->objectName();
        settings->setValue(QLatin1String(SETTINGS_PREFIX) + objectName, list);
    }

    endRemoveRows();
    return true;
}

void HistoryListModel::clearHistory()
{
    list.clear();
    reset();
}

void HistoryListModel::saveEntry(const QString &str)
{
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
    QString objectName = completer->widget()->objectName();
    if (objectName.isEmpty())
        return;
    beginInsertRows (QModelIndex(), list.count(), list.count());
    list.prepend(str);
    list = list.mid(0, maxLines);
    endInsertRows();
    if (settings)
        settings->setValue(QLatin1String(SETTINGS_PREFIX) + objectName, list);
}

bool HistoryListModel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Down) {
        completer->setCompletionPrefix(QString());
        completer->complete();
    }
    return QAbstractListModel::eventFilter(obj,event);
}

HistoryCompleter::HistoryCompleter(QSettings *settings, QObject *parent)
    : QCompleter(parent)
    , d(new HistoryCompleterPrivate(this))
{
    d->model->settings = settings;
    // make an assumption to allow pressing of the down
    // key, before the first model run:
    // parent is likely the lineedit
    QWidget *p = qobject_cast<QWidget *>(parent);
    if (p) {
        p->installEventFilter(d->model);
        QString objectName = p->objectName();
        if (objectName.isEmpty())
            return;
        if (d->model->settings) {
            d->model->list = d->model->settings->value(
                        QLatin1String(SETTINGS_PREFIX) + objectName).toStringList();
        }
    }

    QLineEdit *l = qobject_cast<QLineEdit *>(parent);
    if (l && d->model->list.count())
        l->setText(d->model->list.at(0));

    setModel(d->model);
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
    return d->model->rowCount();
}

int HistoryCompleter::maximalHistorySize() const
{
    return d->model->maxLines;
}

void HistoryCompleter::setMaximalHistorySize(int numberOfEntries)
{
    d->model->maxLines = numberOfEntries;
}

void HistoryCompleter::clearHistory()
{
    d->model->clearHistory();
}

void HistoryCompleter::saveHistory()
{
    d->model->saveEntry(completionPrefix());
}

HistoryCompleterPrivate::HistoryCompleterPrivate(HistoryCompleter *parent)
     : q(parent)
     , model(new HistoryListModel(parent))
{
}

HistoryLineDelegate::HistoryLineDelegate()
{
     pixmap = QPixmap(QLatin1String(":/core/images/editclear.png"));
}

void HistoryLineDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QItemDelegate::paint(painter,option,index);
    QRect r = QStyle::alignedRect(option.direction, Qt::AlignRight | Qt::AlignVCenter , pixmap.size(), option.rect);
    painter->drawPixmap(r, pixmap);
}


HistoryLineView::HistoryLineView(HistoryCompleterPrivate *d_, int pixmapWith_)
  : d(d_)
  , pixmapWidth(pixmapWith_)
{
}

void HistoryLineView::mousePressEvent(QMouseEvent *event)
{
    int rr= event->x();
    if (layoutDirection() == Qt::LeftToRight)
        rr = viewport()->width() - event->x();
    if (rr < pixmapWidth) {
        d->model->removeRow(indexAt(event->pos()).row());
        return;
    }
    QListView::mousePressEvent(event);
}

