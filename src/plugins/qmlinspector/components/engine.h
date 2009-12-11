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
#ifndef ENGINE_H
#define ENGINE_H

#include <private/qmldebug_p.h>

#include <QtDeclarative/qmlengine.h>
#include <QtDeclarative/qmlcontext.h>
#include <QtDeclarative/qmlview.h>

#include <QtCore/qpointer.h>
#include <QtGui/qwidget.h>

QT_BEGIN_NAMESPACE

class ObjectPropertiesView;
class QmlDebugConnection;
class QmlDebugPropertyReference;
class QmlDebugWatch;
class ObjectTree;
class WatchTableModel;
class WatchTableView;
class ExpressionQueryWidget;

class QTabWidget;

class EnginePane : public QWidget
{
Q_OBJECT
public:
    EnginePane(QmlDebugConnection *, QWidget *parent = 0);

public slots:
    void refreshEngines();

private slots:
    void enginesChanged();

    void queryContext(int);
    void contextChanged();

    void engineSelected(int);

private:
    QmlEngineDebug *m_client;
    QmlDebugEnginesQuery *m_engines;
    QmlDebugRootContextQuery *m_context;

    ObjectTree *m_objTree;
    QTabWidget *m_tabs;
    WatchTableView *m_watchTableView;
    WatchTableModel *m_watchTableModel;
    ExpressionQueryWidget *m_exprQueryWidget;

    QmlView *m_engineView;
    QList<QObject *> m_engineItems;

    ObjectPropertiesView *m_propertiesView;
};

QT_END_NAMESPACE

#endif // ENGINE_H

