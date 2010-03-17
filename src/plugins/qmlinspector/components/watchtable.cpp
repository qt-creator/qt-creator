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
#include "watchtable.h"

#include <QtCore/qdebug.h>
#include <QtGui/qevent.h>
#include <QtGui/qaction.h>
#include <QtGui/qmenu.h>

#include <private/qdeclarativedebug_p.h>
#include <private/qdeclarativemetatype_p.h>

QT_BEGIN_NAMESPACE

const int C_NAME = 0;
const int C_VALUE = 1;
const int C_COLUMNS = 2;

WatchTableModel::WatchTableModel(QDeclarativeEngineDebug *client, QObject *parent)
    : QAbstractTableModel(parent),
      m_client(client)
{

}

WatchTableModel::~WatchTableModel()
{
    for (int i=0; i < m_entities.count(); ++i)
        delete m_entities[i].watch;
}

void WatchTableModel::setEngineDebug(QDeclarativeEngineDebug *client)
{
    m_client = client;
}

void WatchTableModel::addWatch(QDeclarativeDebugWatch *watch, const QString &title)
{
    QString property;
    if (qobject_cast<QDeclarativeDebugPropertyWatch *>(watch))
        property = qobject_cast<QDeclarativeDebugPropertyWatch *>(watch)->name();

    connect(watch, SIGNAL(valueChanged(QByteArray,QVariant)),
            SLOT(watchedValueChanged(QByteArray,QVariant)));

    connect(watch, SIGNAL(stateChanged(QDeclarativeDebugWatch::State)), SLOT(watchStateChanged()));

    int row = rowCount(QModelIndex());
    beginInsertRows(QModelIndex(), row, row);

    WatchedEntity e;
    e.title = title;
    e.property = property;
    e.watch = watch;

    m_entities.append(e);

    endInsertRows();
}

void WatchTableModel::removeWatch(QDeclarativeDebugWatch *watch)
{
    int column = rowForWatch(watch);
    if (column == -1)
        return;

    m_entities.takeAt(column);

    reset();
}

void WatchTableModel::updateWatch(QDeclarativeDebugWatch *watch, const QVariant &value)
{
    int row = rowForWatch(watch);
    if (row == -1)
        return;

    m_entities[row].value = value;
    const QModelIndex &idx = index(row, C_VALUE);
    emit dataChanged(idx, idx);

}

QDeclarativeDebugWatch *WatchTableModel::findWatch(int row) const
{
    if (row < m_entities.count())
        return m_entities.at(row).watch;
    return 0;
}

QDeclarativeDebugWatch *WatchTableModel::findWatch(int objectDebugId, const QString &property) const
{
    for (int i=0; i < m_entities.count(); ++i) {
        if (m_entities[i].watch->objectDebugId() == objectDebugId
                && m_entities[i].property == property) {
            return m_entities[i].watch;
        }
    }
    return 0;
}

int WatchTableModel::rowCount(const QModelIndex &) const
{
    return m_entities.count();
}

int WatchTableModel::columnCount(const QModelIndex &) const
{
    return C_COLUMNS;
}

QVariant WatchTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == C_NAME)
            return tr("Name");
        else if (section == C_VALUE)
            return tr("Value");
    }
    return QVariant();
}

QVariant WatchTableModel::data(const QModelIndex &idx, int role) const
{
    if (idx.column() == C_NAME) {
        if (role == Qt::DisplayRole)
            return QVariant(m_entities.at(idx.row()).title);
    } else if (idx.column() == C_VALUE) {

        if (role == Qt::DisplayRole) {
            return QVariant(m_entities.at(idx.row()).value);

        } else if(role == Qt::BackgroundRole) {
            //return QColor(Qt::green);
        }
    }

    return QVariant();
}

void WatchTableModel::watchStateChanged()
{
    QDeclarativeDebugWatch *watch = qobject_cast<QDeclarativeDebugWatch*>(sender());

    if (watch && watch->state() == QDeclarativeDebugWatch::Inactive) {
        removeWatch(watch);
        watch->deleteLater();
    }
}

int WatchTableModel::rowForWatch(QDeclarativeDebugWatch *watch) const
{
    for (int i=0; i < m_entities.count(); ++i) {
        if (m_entities.at(i).watch == watch)
            return i;
    }
    return -1;
}

void WatchTableModel::togglePropertyWatch(const QDeclarativeDebugObjectReference &object, const QDeclarativeDebugPropertyReference &property)
{
    if (!m_client || !property.hasNotifySignal())
        return;

    QDeclarativeDebugWatch *watch = findWatch(object.debugId(), property.name());
    if (watch) {
        // watch will be deleted in watchStateChanged()
        m_client->removeWatch(watch);
        return;
    }

    watch = m_client->addWatch(property, this);
    if (watch->state() == QDeclarativeDebugWatch::Dead) {
        delete watch;
        watch = 0;
    } else {
        QString desc = (object.name().isEmpty() ? QLatin1String("<unnamed object>") : object.name())
                       + QLatin1String(".") + property.name()
                       + object.className();

        addWatch(watch, desc);
        emit watchCreated(watch);
    }
}

void WatchTableModel::watchedValueChanged(const QByteArray &propertyName, const QVariant &value)
{
    Q_UNUSED(propertyName);
    QDeclarativeDebugWatch *watch = qobject_cast<QDeclarativeDebugWatch*>(sender());
    if (watch)
        updateWatch(watch, value);
}

void WatchTableModel::expressionWatchRequested(const QDeclarativeDebugObjectReference &obj, const QString &expr)
{
    if (!m_client)
        return;

    QDeclarativeDebugWatch *watch = m_client->addWatch(obj, expr, this);

    if (watch->state() == QDeclarativeDebugWatch::Dead) {
        delete watch;
        watch = 0;
    } else {
        addWatch(watch, expr);
        emit watchCreated(watch);
    }
}

void WatchTableModel::removeWatchAt(int row)
{
    if (!m_client)
        return;

    QDeclarativeDebugWatch *watch = findWatch(row);
    if (watch) {
        m_client->removeWatch(watch);
        delete watch;
        watch = 0;
    }
}

void WatchTableModel::removeAllWatches()
{
    for (int i=0; i<m_entities.count(); ++i) {
        if (m_client)
            m_client->removeWatch(m_entities[i].watch);
        else
            delete m_entities[i].watch;
    }
    m_entities.clear();
    reset();
}

//----------------------------------------------

WatchTableHeaderView::WatchTableHeaderView(WatchTableModel *model, QWidget *parent)
    : QHeaderView(Qt::Horizontal, parent),
      m_model(model)
{
    setClickable(true);
    setResizeMode(QHeaderView::ResizeToContents);
    setMinimumSectionSize(100);
    setStretchLastSection(true);
}

//----------------------------------------------

WatchTableView::WatchTableView(WatchTableModel *model, QWidget *parent)
    : QTableView(parent),
      m_model(model)
{
    setAlternatingRowColors(true);

    connect(model, SIGNAL(watchCreated(QDeclarativeDebugWatch*)), SLOT(watchCreated(QDeclarativeDebugWatch*)));
    connect(this, SIGNAL(activated(QModelIndex)), SLOT(indexActivated(QModelIndex)));
}

void WatchTableView::indexActivated(const QModelIndex &index)
{
    QDeclarativeDebugWatch *watch = m_model->findWatch(index.column());
    if (watch)
        emit objectActivated(watch->objectDebugId());
}

void WatchTableView::watchCreated(QDeclarativeDebugWatch *watch)
{
    int column = m_model->rowForWatch(watch);
    resizeColumnToContents(column);
}

void WatchTableView::mousePressEvent(QMouseEvent *me)
{
    QTableView::mousePressEvent(me);

    if (me->button() == Qt::RightButton && me->type() == QEvent::MouseButtonPress) {
        int row = rowAt(me->pos().y());
        if (row >= 0) {
            QAction action(tr("Stop watching"), 0);
            QList<QAction *> actions;
            actions << &action;
            if (QMenu::exec(actions, me->globalPos()))
                m_model->removeWatchAt(row);
        }
    }
}

QT_END_NAMESPACE
