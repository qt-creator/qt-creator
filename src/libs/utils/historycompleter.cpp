/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "historycompleter.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QSettings>

#include <QtGui/QLineEdit>
#include <QtGui/QKeyEvent>
#include <QtGui/QItemDelegate>
#include <QtGui/QListView>
#include <QtGui/QPainter>
#include <QtGui/QStyle>

namespace Utils {

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
    HistoryCompleter *q;
    QWidget *lastSeenWidget;
    QSettings *settings;
    int maxLines;
};

class HistoryCompleterPrivate
{
public:
    HistoryCompleterPrivate(HistoryCompleter *parent);
    HistoryCompleter *q_ptr;
    HistoryListModel *model;
    Q_DECLARE_PUBLIC(HistoryCompleter)
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


HistoryListModel::HistoryListModel(HistoryCompleter *parent)
    : QAbstractListModel(parent)
    , q(parent)
    , lastSeenWidget(0)
    , settings(new QSettings(parent))
    , maxLines(30)
{
    settings->beginGroup(QLatin1String("CompleterHistory"));
}

void HistoryListModel::fetchHistory()
{
    if (!q->widget()) {
        list.clear();
        reset();
        return;
    }
    QString objectName = q->widget()->objectName();
    if (objectName.isEmpty())
        return;
    list = settings->value(objectName).toStringList();
    reset();
}

int HistoryListModel::rowCount(const QModelIndex &parent) const
{
    if (lastSeenWidget != q->widget()) {
        if (lastSeenWidget)
            const_cast<QWidget*>(lastSeenWidget)->removeEventFilter(const_cast<HistoryListModel *>(this));
        const_cast<QWidget*>(q->widget())->installEventFilter(const_cast<HistoryListModel *>(this));
        if (qobject_cast<QLineEdit *>(lastSeenWidget))
            // this will result in spamming the history with garbage in some corner cases.
            // not my idea.
            disconnect(lastSeenWidget, SIGNAL(editingFinished ()), q, SLOT(saveHistory()));
        HistoryListModel *that = const_cast<HistoryListModel *>(this);
        that->lastSeenWidget = q->widget();
        that->fetchHistory();
        if (qobject_cast<QLineEdit *>(lastSeenWidget))
            connect(lastSeenWidget, SIGNAL(editingFinished ()), q, SLOT(saveHistory()));
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
    QString objectName = q->widget()->objectName();
    settings->setValue(objectName, list);
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
    if (!q->widget())
        return;
    if (lastSeenWidget != q->widget()) {
        if (lastSeenWidget)
            lastSeenWidget->removeEventFilter(this);
        q->widget()->installEventFilter(this);
        fetchHistory();
        lastSeenWidget = q->widget();
    }
    QString objectName = q->widget()->objectName();
    if (objectName.isEmpty())
        return;
    beginInsertRows (QModelIndex(), list.count(), list.count());
    list.prepend(str);
    list = list.mid(0, maxLines);
    endInsertRows();
    settings->setValue(objectName, list);
}

bool HistoryListModel::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Down) {
        q->setCompletionPrefix(QString());
        q->complete();
    }
    return QAbstractListModel::eventFilter(obj,event);
}


HistoryCompleter::HistoryCompleter(QObject *parent)
    : QCompleter(parent)
    , d_ptr(new HistoryCompleterPrivate(this))
{
    // make an assumption to allow pressing of the down
    // key, before the first model run:
    // parent is likely the lineedit
    QWidget *p = qobject_cast<QWidget *>(parent);
    if (p) {
        p->installEventFilter(d_ptr->model);
        QString objectName = p->objectName();
        if (objectName.isEmpty())
            return;
        d_ptr->model->list = d_ptr->model->settings->value(objectName).toStringList();
    }

    QLineEdit *l = qobject_cast<QLineEdit *>(parent);
    if (l && d_ptr->model->list.count())
        l->setText(d_ptr->model->list.at(0));

    setModel(d_ptr->model);
    HistoryLineDelegate *delegate = new HistoryLineDelegate;
    HistoryLineView *view = new HistoryLineView(d_ptr, delegate->pixmap.width());
    setPopup(view);
    view->setItemDelegate(delegate);
}

QSettings *HistoryCompleter::settings() const
{
    Q_D(const HistoryCompleter);
    return d->model->settings;
}

int HistoryCompleter::historySize() const
{
    Q_D(const HistoryCompleter);
    return d->model->rowCount();
}

int HistoryCompleter::maximalHistorySize() const
{
    Q_D(const HistoryCompleter);
    return d->model->maxLines;
}

void HistoryCompleter::setMaximalHistorySize(int numberOfEntries)
{
    Q_D(const HistoryCompleter);
    d->model->maxLines = numberOfEntries;
}

void HistoryCompleter::clearHistory()
{
    Q_D(const HistoryCompleter);
    d->model->clearHistory();
}

void HistoryCompleter::saveHistory()
{
    Q_D(HistoryCompleter);
    d->model->saveEntry(completionPrefix());
}


HistoryCompleterPrivate::HistoryCompleterPrivate(HistoryCompleter *parent)
     : q_ptr(parent)
     , model(new HistoryListModel(parent))
{
}


HistoryLineDelegate::HistoryLineDelegate()
{
     pixmap = QPixmap(":/core/images/editclear.png");
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

} // namespace Utils
