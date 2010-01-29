/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef PROPERTIESTABLEMODEL_H
#define PROPERTIESTABLEMODEL_H

#include <private/qmldebug_p.h>

#include <QtGui/qwidget.h>

QT_BEGIN_NAMESPACE

class QTreeWidget;
class QTreeWidgetItem;
class QmlDebugConnection;
class PropertiesViewItem;

class ObjectPropertiesView : public QWidget
{
    Q_OBJECT
public:
    ObjectPropertiesView(QmlEngineDebug *client = 0, QWidget *parent = 0);

    void setEngineDebug(QmlEngineDebug *client);
    void clear();

signals:
    void activated(const QmlDebugObjectReference &, const QmlDebugPropertyReference &);

public slots:
    void reload(const QmlDebugObjectReference &);
    void watchCreated(QmlDebugWatch *);

private slots:
    void queryFinished();
    void watchStateChanged();
    void valueChanged(const QByteArray &name, const QVariant &value);
    void itemActivated(QTreeWidgetItem *i);

private:
    void setObject(const QmlDebugObjectReference &object);
    void setWatched(const QString &property, bool watched);
    void setPropertyValue(PropertiesViewItem *item, const QVariant &value, bool makeGray);

    QmlEngineDebug *m_client;
    QmlDebugObjectQuery *m_query;
    QmlDebugWatch *m_watch;

    QTreeWidget *m_tree;
    QmlDebugObjectReference m_object;
};


QT_END_NAMESPACE

#endif
