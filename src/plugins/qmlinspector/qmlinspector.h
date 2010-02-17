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
#ifndef QMLINSPECTORMODE_H
#define QMLINSPECTORMODE_H

#include "qmlinspector_global.h"
#include <coreplugin/basemode.h>

#include <QtGui/QAction>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

class QDockWidget;
class QToolButton;
class QLineEdit;
class QSpinBox;
class QLabel;

class QmlEngineDebug;
class QmlDebugConnection;
class QmlDebugEnginesQuery;
class QmlDebugRootContextQuery;
class QmlDebugObjectReference;
class ObjectTree;
class WatchTableModel;
class WatchTableView;
class ObjectPropertiesView;
class CanvasFrameRate;
class ExpressionQueryWidget;

QT_END_NAMESPACE

namespace Qml {
    class EngineSpinBox;

class QMLINSPECTOR_EXPORT QmlInspector : public QObject
{
    Q_OBJECT

public:
    QmlInspector(QObject *parent = 0);

    bool connectToViewer(); // using host, port from widgets

signals:
    void statusMessage(const QString &text);

public slots:
    void disconnectFromViewer();
    void setSimpleDockWidgetArrangement();

private slots:
    void connectionStateChanged();
    void connectionError();
    void reloadEngines();
    void enginesChanged();
    void queryEngineContext(int);
    void contextChanged();
    void treeObjectActivated(const QmlDebugObjectReference &obj);

private:

    void initWidgets();

    QmlDebugConnection *m_conn;
    QmlEngineDebug *m_client;

    QmlDebugEnginesQuery *m_engineQuery;
    QmlDebugRootContextQuery *m_contextQuery;

    ObjectTree *m_objectTreeWidget;
    ObjectPropertiesView *m_propertiesWidget;
    WatchTableModel *m_watchTableModel;
    WatchTableView *m_watchTableView;
    CanvasFrameRate *m_frameRateWidget;
    ExpressionQueryWidget *m_expressionWidget;

    EngineSpinBox *m_engineSpinBox;

    QDockWidget *m_objectTreeDock;
    QDockWidget *m_frameRateDock;
    QDockWidget *m_propertyWatcherDock;
    QList<QDockWidget*> m_dockWidgets;

};

}

#endif
