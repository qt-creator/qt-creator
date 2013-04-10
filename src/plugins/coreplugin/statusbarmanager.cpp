/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "statusbarmanager.h"

#include "mainwindow.h"
#include "statusbarwidget.h"

#include <extensionsystem/pluginmanager.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QStatusBar>

using namespace Core;
using namespace Core::Internal;

static QWidget *createWidget(QWidget *parent = 0)
{
    QWidget *w = new QWidget(parent);
    w->setLayout(new QHBoxLayout);
    w->setVisible(true);
    w->layout()->setMargin(0);
    return w;
}

StatusBarManager::StatusBarManager(MainWindow *mainWnd)
  : QObject(mainWnd),
    m_mainWnd(mainWnd)
{
    QStatusBar *bar = m_mainWnd->statusBar();
    for (int i = 0; i <= StatusBarWidget::LastLeftAligned; ++i) {
        QWidget *w = createWidget(bar);
        bar->insertPermanentWidget(i, w);
        m_statusBarWidgets.append(w);
    }
    m_mainWnd->statusBar()->insertPermanentWidget(StatusBarWidget::LastLeftAligned + 1,
                                                  new QLabel(), 1);
    QWidget *rightCornerWidget = createWidget(bar);
    m_mainWnd->statusBar()->insertPermanentWidget(StatusBarWidget::LastLeftAligned + 2,
                                                  rightCornerWidget);
    m_statusBarWidgets.append(rightCornerWidget);
}

StatusBarManager::~StatusBarManager()
{
}

void StatusBarManager::init()
{
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject*)),
            this, SLOT(objectAdded(QObject*)));
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(aboutToRemoveObject(QObject*)),
            this, SLOT(aboutToRemoveObject(QObject*)));
}

void StatusBarManager::objectAdded(QObject *obj)
{
    StatusBarWidget *view = Aggregation::query<StatusBarWidget>(obj);
    if (!view)
        return;

    QWidget *viewWidget = 0;
    viewWidget = view->widget();
    m_statusBarWidgets.at(view->position())->layout()->addWidget(viewWidget);

    m_mainWnd->addContextObject(view);
}

void StatusBarManager::aboutToRemoveObject(QObject *obj)
{
    StatusBarWidget *view = Aggregation::query<StatusBarWidget>(obj);
    if (!view)
        return;
    m_mainWnd->removeContextObject(view);
}

void StatusBarManager::extensionsInitalized()
{
}
