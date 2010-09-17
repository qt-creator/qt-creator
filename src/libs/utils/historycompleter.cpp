/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "historycompleter.h"
#include <QLineEdit>
#include <QCompleter>
#include <QAbstractListModel>
#include <QSettings>

namespace Utils {

class HistoryListModel : public QAbstractListModel
{
public:
    HistoryListModel(HistoryCompleter *parent);
    void fetchHistory();
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole) const;
    void clearHistory();
    void saveEntry(const QString &str);

    QStringList list;
    HistoryCompleter *q;
    QWidget *lastSeenWidget;
    QSettings *settings;
    int maxLines;
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

void HistoryListModel::clearHistory()
{
    list.clear();
    reset();
}

void HistoryListModel::saveEntry(const QString &str)
{
    if (list.contains(str))
        return;
    if (!q->widget())
        return;
    if (lastSeenWidget != q->widget()) {
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


class HistoryCompleterPrivate
{
public:
    HistoryCompleterPrivate(HistoryCompleter *parent)
        : q_ptr(parent)
        , model(new HistoryListModel(parent))
    {
        Q_Q(HistoryCompleter);
        q->setModel(model);
    }
    HistoryCompleter *q_ptr;
    HistoryListModel *model;
    Q_DECLARE_PUBLIC(HistoryCompleter);
};


HistoryCompleter::HistoryCompleter(QObject *parent)
    : QCompleter(parent)
    , d_ptr(new HistoryCompleterPrivate(this))
{

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

} // namespace Utils
