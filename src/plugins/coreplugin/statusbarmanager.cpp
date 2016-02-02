/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "statusbarmanager.h"

#include "mainwindow.h"
#include "statusbarwidget.h"

#include <extensionsystem/pluginmanager.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QSplitter>
#include <QStatusBar>

static const char kSettingsGroup[] = "StatusBar";
static const char kLeftSplitWidthKey[] = "LeftSplitWidth";

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
    m_splitter = new NonResizingSplitter(bar);
    bar->insertPermanentWidget(0, m_splitter, 10);
    m_splitter->setChildrenCollapsible(false);
    // first
    QWidget *w = createWidget(m_splitter);
    w->layout()->setContentsMargins(0, 0, 3, 0);
    m_splitter->addWidget(w);
    m_statusBarWidgets.append(w);

    QWidget *w2 = createWidget(m_splitter);
    w2->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_splitter->addWidget(w2);
    // second
    w = createWidget(w2);
    w2->layout()->addWidget(w);
    m_statusBarWidgets.append(w);
    // third
    w = createWidget(w2);
    w2->layout()->addWidget(w);
    m_statusBarWidgets.append(w);

    static_cast<QBoxLayout *>(w2->layout())->addStretch(1);

    QWidget *rightCornerWidget = createWidget(bar);
    bar->insertPermanentWidget(1, rightCornerWidget);
    m_statusBarWidgets.append(rightCornerWidget);
}

StatusBarManager::~StatusBarManager()
{
}

void StatusBarManager::init()
{
    connect(ExtensionSystem::PluginManager::instance(), &ExtensionSystem::PluginManager::objectAdded,
            this, &StatusBarManager::objectAdded);
    connect(ExtensionSystem::PluginManager::instance(), &ExtensionSystem::PluginManager::aboutToRemoveObject,
            this, &StatusBarManager::aboutToRemoveObject);
    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &StatusBarManager::saveSettings);
}

void StatusBarManager::objectAdded(QObject *obj)
{
    StatusBarWidget *view = qobject_cast<StatusBarWidget *>(obj);
    if (!view)
        return;

    QWidget *viewWidget = 0;
    viewWidget = view->widget();
    m_statusBarWidgets.at(view->position())->layout()->addWidget(viewWidget);

    m_mainWnd->addContextObject(view);
}

void StatusBarManager::aboutToRemoveObject(QObject *obj)
{
    StatusBarWidget *view = qobject_cast<StatusBarWidget *>(obj);
    if (!view)
        return;
    m_mainWnd->removeContextObject(view);
}

void StatusBarManager::saveSettings()
{
    QSettings *s = ICore::settings();
    s->beginGroup(QLatin1String(kSettingsGroup));
    s->setValue(QLatin1String(kLeftSplitWidthKey), m_splitter->sizes().at(0));
    s->endGroup();
}

void StatusBarManager::extensionsInitalized()
{
}

void StatusBarManager::restoreSettings()
{
    QSettings *s = ICore::settings();
    s->beginGroup(QLatin1String(kSettingsGroup));
    int leftSplitWidth = s->value(QLatin1String(kLeftSplitWidthKey), -1).toInt();
    s->endGroup();
    if (leftSplitWidth < 0) {
        // size first split after its sizeHint + a bit of buffer
        leftSplitWidth = m_splitter->widget(0)->sizeHint().width();
    }
    int sum = 0;
    foreach (int w, m_splitter->sizes())
        sum += w;
    m_splitter->setSizes(QList<int>() << leftSplitWidth << (sum - leftSplitWidth));
}

NonResizingSplitter::NonResizingSplitter(QWidget *parent)
    : MiniSplitter(parent, Light)
{
}

void NonResizingSplitter::resizeEvent(QResizeEvent *ev)
{
    // bypass QSplitter magic
    int leftSplitWidth = qMin(sizes().at(0), ev->size().width());
    int rightSplitWidth = qMax(0, ev->size().width() - leftSplitWidth);
    setSizes(QList<int>() << leftSplitWidth << rightSplitWidth);
    return QWidget::resizeEvent(ev);
}
