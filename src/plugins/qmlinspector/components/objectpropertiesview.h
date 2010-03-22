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
#ifndef PROPERTIESTABLEMODEL_H
#define PROPERTIESTABLEMODEL_H

#include <private/qdeclarativedebug_p.h>

#include <QtGui/qwidget.h>

QT_BEGIN_NAMESPACE

class QTreeWidget;
class QTreeWidgetItem;
class QDeclarativeDebugConnection;

QT_END_NAMESPACE

namespace Qml {
namespace Internal {

class PropertiesViewItem;

class ObjectPropertiesView : public QWidget
{
    Q_OBJECT
public:
    ObjectPropertiesView(QDeclarativeEngineDebug *client = 0, QWidget *parent = 0);

    void setEngineDebug(QDeclarativeEngineDebug *client);
    void clear();

signals:
    void activated(const QDeclarativeDebugObjectReference &, const QDeclarativeDebugPropertyReference &);
    void contextHelpIdChanged(const QString &contextHelpId);

public slots:
    void reload(const QDeclarativeDebugObjectReference &);
    void watchCreated(QDeclarativeDebugWatch *);

private slots:
    void changeItemSelection();
    void queryFinished();
    void watchStateChanged();
    void valueChanged(const QByteArray &name, const QVariant &value);
    void itemActivated(QTreeWidgetItem *i);

private:
    void setObject(const QDeclarativeDebugObjectReference &object);
    void setWatched(const QString &property, bool watched);
    void setPropertyValue(PropertiesViewItem *item, const QVariant &value, bool makeGray);

    QDeclarativeEngineDebug *m_client;
    QDeclarativeDebugObjectQuery *m_query;
    QDeclarativeDebugWatch *m_watch;

    QTreeWidget *m_tree;
    QDeclarativeDebugObjectReference m_object;
};

} // Internal
} // Qml


#endif
