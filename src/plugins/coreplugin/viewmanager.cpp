/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "viewmanager.h"

#include "coreconstants.h"
#include "mainwindow.h"
#include "uniqueidmanager.h"
#include "iview.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <aggregation/aggregate.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QSettings>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QStatusBar>

using namespace Core;
using namespace Core::Internal;

ViewManager::ViewManager(MainWindow *mainWnd)  :
    ViewManagerInterface(mainWnd),
    m_mainWnd(mainWnd)
{
    for (int i = 0; i < 3; ++i) {
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
    IView *view = Aggregation::query<IView>(obj);
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
    IView *view = Aggregation::query<IView>(obj);
    if (!view)
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

IView *ViewManager::view(const QString &id)
{
    QList<IView *> list =
        ExtensionSystem::PluginManager::instance()->getObjects<IView>();
    foreach (IView *view, list) {
        if (view->uniqueViewName() == id)
            return view;
    }
    return 0;
}
