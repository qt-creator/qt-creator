/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "viewmanager.h"

#include "coreconstants.h"
#include "mainwindow.h"
#include "uniqueidmanager.h"
#include "iview.h"

#include <QtCore/QSettings>
#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtGui/QDockWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenu>
#include <QtGui/QStatusBar>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QStackedWidget>
#include <QtGui/QToolButton>

#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/actionmanager/icommand.h>
#include <extensionsystem/ExtensionSystemInterfaces>
#include <aggregation/aggregate.h>

#include <QtGui/QHBoxLayout>

using namespace Core;
using namespace Core::Internal;

ViewManager::ViewManager(MainWindow *mainWnd)  :
    ViewManagerInterface(mainWnd),
    m_mainWnd(mainWnd)
{
    for(int i = 0; i< 3; ++i) {
        QWidget *w = new QWidget();
        m_mainWnd->statusBar()->insertPermanentWidget(i, w);
        w->setLayout(new QHBoxLayout);
        w->setVisible(true);
        w->layout()->setMargin(0);
        m_statusBarWidgets.append(w);
    }
    QLabel *l = new QLabel();
    m_mainWnd->statusBar()->insertPermanentWidget(3, l, 1);
}

ViewManager::~ViewManager()
{
}

void ViewManager::init()
{
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject*)),
            this, SLOT(objectAdded(QObject*)));
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(aboutToRemoveObject(QObject*)),
            this, SLOT(aboutToRemoveObject(QObject*)));
}

void ViewManager::objectAdded(QObject *obj)
{
    IView * view = Aggregation::query<IView>(obj);
    if (!view)
        return;

    QWidget *viewWidget = 0;
    viewWidget = view->widget();
    m_statusBarWidgets.at(view->defaultPosition())->layout()->addWidget(viewWidget);

    m_viewMap.insert(view, viewWidget);
    viewWidget->setObjectName(view->uniqueViewName());
    m_mainWnd->addContextObject(view);
}

void ViewManager::aboutToRemoveObject(QObject *obj)
{
    IView * view = Aggregation::query<IView>(obj);
    if(!view)
        return;
    m_mainWnd->removeContextObject(view);
}

void ViewManager::extensionsInitalized()
{
    QSettings *settings = m_mainWnd->settings();
    m_mainWnd->restoreState(settings->value(QLatin1String("ViewGroup_Default"), QByteArray()).toByteArray());
}

void ViewManager::saveSettings(QSettings *settings)
{
    settings->setValue(QLatin1String("ViewGroup_Default"), m_mainWnd->saveState());
}

IView * ViewManager::view(const QString & id)
{
    QList<IView *> list = m_mainWnd->pluginManager()->getObjects<IView>();
    foreach (IView * view, list) {
        if (view->uniqueViewName() == id)
            return view;
    }
    return 0;
}
