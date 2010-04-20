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

#include "inspectorsettings.h"

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
class WatchTableModel;

class ObjectPropertiesView : public QWidget
{
    Q_OBJECT
public:
    ObjectPropertiesView(WatchTableModel *watchTableModel, QDeclarativeEngineDebug *client = 0, QWidget *parent = 0);
    void readSettings(const InspectorSettings &settings);
    void saveSettings(InspectorSettings &settings);

    void setEngineDebug(QDeclarativeEngineDebug *client);
    void clear();

signals:
    void watchToggleRequested(const QDeclarativeDebugObjectReference &, const QDeclarativeDebugPropertyReference &);
    void contextHelpIdChanged(const QString &contextHelpId);

public slots:
    void reload(const QDeclarativeDebugObjectReference &);
    void watchCreated(QDeclarativeDebugWatch *);

protected:
    void contextMenuEvent(QContextMenuEvent *);

private slots:
    void changeItemSelection();
    void queryFinished();
    void watchStateChanged();
    void valueChanged(const QByteArray &name, const QVariant &value);
    void itemDoubleClicked(QTreeWidgetItem *i, int column);

    void addWatch();
    void removeWatch();
    void toggleUnwatchableProperties();
    void toggleGroupingByItemType();

private:
    void sortBaseClassItems();
    bool styleWatchedItem(PropertiesViewItem *item, const QString &property, bool isWatched) const;
    QString propertyBaseClass(const QDeclarativeDebugObjectReference &object, const QDeclarativeDebugPropertyReference &property, int &depth);
    void toggleWatch(QTreeWidgetItem *item);
    void setObject(const QDeclarativeDebugObjectReference &object);
    bool isWatched(QTreeWidgetItem *item);
    void setWatched(const QString &property, bool watched);
    void setPropertyValue(PropertiesViewItem *item, const QVariant &value, bool makeGray);

    QDeclarativeEngineDebug *m_client;
    QDeclarativeDebugObjectQuery *m_query;
    QDeclarativeDebugWatch *m_watch;

    QAction *m_addWatchAction;
    QAction *m_removeWatchAction;
    QAction *m_toggleUnwatchablePropertiesAction;
    QAction *m_toggleGroupByItemTypeAction;
    QTreeWidgetItem *m_clickedItem;

    bool m_groupByItemType;
    bool m_showUnwatchableProperties;
    QTreeWidget *m_tree;
    QWeakPointer<WatchTableModel> m_watchTableModel;

    QDeclarativeDebugObjectReference m_object;
};

} // Internal
} // Qml


#endif
