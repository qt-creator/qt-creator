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
#ifndef WATCHTABLEMODEL_H
#define WATCHTABLEMODEL_H

#include <QtCore/qpointer.h>
#include <QtCore/qlist.h>
#include <QtCore/qabstractitemmodel.h>

#include <QtGui/qwidget.h>
#include <QtGui/qheaderview.h>
#include <QtGui/qtableview.h>

QT_BEGIN_NAMESPACE

class QDeclarativeDebugWatch;
class QDeclarativeEngineDebug;
class QDeclarativeDebugConnection;
class QDeclarativeDebugPropertyReference;
class QDeclarativeDebugObjectReference;

QT_END_NAMESPACE

namespace Qml {
namespace Internal {

class WatchTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    WatchTableModel(QDeclarativeEngineDebug *client = 0, QObject *parent = 0);
    ~WatchTableModel();

    void setEngineDebug(QDeclarativeEngineDebug *client);

    QDeclarativeDebugWatch *findWatch(int column) const;
    int rowForWatch(QDeclarativeDebugWatch *watch) const;

    void removeWatchAt(int row);
    void removeAllWatches();

    Qt::ItemFlags flags (const QModelIndex & index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData (const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

signals:
    void watchCreated(QDeclarativeDebugWatch *watch);

public slots:
    void togglePropertyWatch(const QDeclarativeDebugObjectReference &obj, const QDeclarativeDebugPropertyReference &prop);
    void expressionWatchRequested(const QDeclarativeDebugObjectReference &, const QString &);

private slots:
    void watchStateChanged();
    void watchedValueChanged(const QByteArray &propertyName, const QVariant &value);

private:
    void addWatch(QDeclarativeDebugWatch *watch, const QString &title);
    void removeWatch(QDeclarativeDebugWatch *watch);
    void updateWatch(QDeclarativeDebugWatch *watch, const QVariant &value);

    QDeclarativeDebugWatch *findWatch(int objectDebugId, const QString &property) const;

    struct WatchedEntity
    {
        QString title;
        QString property;
        QPointer<QDeclarativeDebugWatch> watch;
        QVariant value;
    };

    QDeclarativeEngineDebug *m_client;
    QList<WatchedEntity> m_entities;
};


class WatchTableHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    WatchTableHeaderView(WatchTableModel *model, QWidget *parent = 0);

private:
    WatchTableModel *m_model;
};


class WatchTableView : public QTableView
{
    Q_OBJECT
public:
    WatchTableView(WatchTableModel *model, QWidget *parent = 0);

signals:
    void objectActivated(int objectDebugId);
    void contextHelpIdChanged(const QString &contextHelpId);

protected:
    void mousePressEvent(QMouseEvent *me);

private slots:
    void indexActivated(const QModelIndex &index);
    void watchCreated(QDeclarativeDebugWatch *watch);

private:
    WatchTableModel *m_model;
};


} // Internal
} // Qml

#endif // WATCHTABLEMODEL_H
