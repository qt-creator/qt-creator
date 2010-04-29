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

#include "fancymainwindow.h"

#include <QtCore/QList>
#include <QtCore/QHash>

#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QDockWidget>
#include <QtCore/QSettings>

static const char lockedKeyC[] = "Locked";
static const char stateKeyC[] = "State";
static const int settingsVersion = 1;

namespace Utils {

struct FancyMainWindowPrivate {
    explicit FancyMainWindowPrivate(FancyMainWindow *q);

    QList<QDockWidget *> m_dockWidgets;
    QList<bool> m_dockWidgetActiveState;
    bool m_locked;
    bool m_handleDockVisibilityChanges; //todo

    QAction *m_menuSeparator1;
    QAction *m_toggleLockedAction;
    QAction *m_menuSeparator2;
    QAction *m_resetLayoutAction;
};

FancyMainWindowPrivate::FancyMainWindowPrivate(FancyMainWindow *q) :
    m_locked(true), m_handleDockVisibilityChanges(true),
    m_menuSeparator1(new QAction(q)),
    m_toggleLockedAction(new QAction(FancyMainWindow::tr("Locked"), q)),
    m_menuSeparator2(new QAction(q)),
    m_resetLayoutAction(new QAction(FancyMainWindow::tr("Reset to Default Layout") ,q))
{
    m_toggleLockedAction->setCheckable(true);
    m_toggleLockedAction->setChecked(m_locked);
    m_menuSeparator1->setSeparator(true);
    m_menuSeparator2->setSeparator(true);
}

FancyMainWindow::FancyMainWindow(QWidget *parent) :
    QMainWindow(parent), d(new FancyMainWindowPrivate(this))
{
    connect(d->m_toggleLockedAction, SIGNAL(toggled(bool)),
            this, SLOT(setLocked(bool)));
    connect(d->m_resetLayoutAction, SIGNAL(triggered()),
            this, SIGNAL(resetLayout()));
}

FancyMainWindow::~FancyMainWindow()
{
    delete d;
}

QDockWidget *FancyMainWindow::addDockForWidget(QWidget *widget)
{
    QDockWidget *dockWidget = new QDockWidget(widget->windowTitle(), this);
    dockWidget->setWidget(widget);
    // Set an object name to be used in settings, derive from widget name
    const QString objectName = widget->objectName();
    if (objectName.isEmpty()) {
        dockWidget->setObjectName(QLatin1String("dockWidget") + QString::number(d->m_dockWidgets.size() + 1));
    } else {
        dockWidget->setObjectName(objectName + QLatin1String("DockWidget"));
    }
    connect(dockWidget->toggleViewAction(), SIGNAL(triggered()),
        this, SLOT(onDockActionTriggered()), Qt::QueuedConnection);
    connect(dockWidget, SIGNAL(visibilityChanged(bool)),
            this, SLOT(onDockVisibilityChange(bool)));
    connect(dockWidget, SIGNAL(topLevelChanged(bool)),
            this, SLOT(onTopLevelChanged()));
    d->m_dockWidgets.append(dockWidget);
    d->m_dockWidgetActiveState.append(true);
    updateDockWidget(dockWidget);
    return dockWidget;
}

void FancyMainWindow::updateDockWidget(QDockWidget *dockWidget)
{
    const QDockWidget::DockWidgetFeatures features =
            (d->m_locked) ? QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable
                       : QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable;
    QWidget *titleBarWidget = dockWidget->titleBarWidget();
    if (d->m_locked && !titleBarWidget && !dockWidget->isFloating())
        titleBarWidget = new QWidget(dockWidget);
    else if ((!d->m_locked || dockWidget->isFloating()) && titleBarWidget) {
        delete titleBarWidget;
        titleBarWidget = 0;
    }
    dockWidget->setTitleBarWidget(titleBarWidget);
    dockWidget->setFeatures(features);
}

void FancyMainWindow::onDockActionTriggered()
{
    QDockWidget *dw = qobject_cast<QDockWidget *>(sender()->parent());
    if (dw) {
        if (dw->isVisible())
            dw->raise();
    }
}

void FancyMainWindow::onDockVisibilityChange(bool visible)
{
    if (!d->m_handleDockVisibilityChanges)
        return;
    QDockWidget *dockWidget = qobject_cast<QDockWidget *>(sender());
    int index = d->m_dockWidgets.indexOf(dockWidget);
    d->m_dockWidgetActiveState[index] = visible;
}

void FancyMainWindow::onTopLevelChanged()
{
    updateDockWidget(qobject_cast<QDockWidget*>(sender()));
}

void FancyMainWindow::setTrackingEnabled(bool enabled)
{
    if (enabled) {
        d->m_handleDockVisibilityChanges = true;
        for (int i = 0; i < d->m_dockWidgets.size(); ++i)
            d->m_dockWidgetActiveState[i] = d->m_dockWidgets[i]->isVisible();
    } else {
        d->m_handleDockVisibilityChanges = false;
    }
}

void FancyMainWindow::setLocked(bool locked)
{
    d->m_locked = locked;
    foreach (QDockWidget *dockWidget, d->m_dockWidgets) {
        updateDockWidget(dockWidget);
    }
}

void FancyMainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)
    handleVisibilityChanged(false);
}

void FancyMainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    handleVisibilityChanged(true);
}

void FancyMainWindow::handleVisibilityChanged(bool visible)
{
    d->m_handleDockVisibilityChanges = false;
    for (int i = 0; i < d->m_dockWidgets.size(); ++i) {
        QDockWidget *dockWidget = d->m_dockWidgets.at(i);
        if (dockWidget->isFloating()) {
            dockWidget->setVisible(visible && d->m_dockWidgetActiveState.at(i));
        }
    }
    if (visible)
        d->m_handleDockVisibilityChanges = true;
}

void FancyMainWindow::saveSettings(QSettings *settings) const
{
    QHash<QString, QVariant> hash = saveSettings();
    QHashIterator<QString, QVariant> it(hash);
    while (it.hasNext()) {
        it.next();
        settings->setValue(it.key(), it.value());
    }
}

void FancyMainWindow::restoreSettings(const QSettings *settings)
{
    QHash<QString, QVariant> hash;
    foreach (const QString &key, settings->childKeys()) {
        hash.insert(key, settings->value(key));
    }
    restoreSettings(hash);
}

QHash<QString, QVariant> FancyMainWindow::saveSettings() const
{
    QHash<QString, QVariant> settings;
    settings.insert(QLatin1String(stateKeyC), saveState(settingsVersion));
    settings.insert(QLatin1String(lockedKeyC), d->m_locked);
    for (int i = 0; i < d->m_dockWidgetActiveState.count(); ++i) {
        settings.insert(d->m_dockWidgets.at(i)->objectName(),
                           d->m_dockWidgetActiveState.at(i));
    }
    return settings;
}

void FancyMainWindow::restoreSettings(const QHash<QString, QVariant> &settings)
{
    QByteArray ba = settings.value(QLatin1String(stateKeyC), QByteArray()).toByteArray();
    if (!ba.isEmpty())
        restoreState(ba, settingsVersion);
    d->m_locked = settings.value(QLatin1String("Locked"), true).toBool();
    d->m_toggleLockedAction->setChecked(d->m_locked);
    for (int i = 0; i < d->m_dockWidgetActiveState.count(); ++i) {
        d->m_dockWidgetActiveState[i] = settings.value(d->m_dockWidgets.at(i)->objectName(), false).toBool();
    }
}

QList<QDockWidget *> FancyMainWindow::dockWidgets() const
{
    return d->m_dockWidgets;
}

bool FancyMainWindow::isLocked() const
{
    return d->m_locked;
}

QMenu *FancyMainWindow::createPopupMenu()
{
    QMenu *menu = QMainWindow::createPopupMenu();
    menu->addAction(d->m_menuSeparator1);
    menu->addAction(d->m_toggleLockedAction);
    menu->addAction(d->m_menuSeparator2);
    menu->addAction(d->m_resetLayoutAction);
    return menu;
}

QAction *FancyMainWindow::menuSeparator1() const
{
    return d->m_menuSeparator1;
}

QAction *FancyMainWindow::toggleLockedAction() const
{
    return d->m_toggleLockedAction;
}

QAction *FancyMainWindow::menuSeparator2() const
{
    return d->m_menuSeparator2;
}

QAction *FancyMainWindow::resetLayoutAction() const
{
    return d->m_resetLayoutAction;
}

void FancyMainWindow::setDockActionsVisible(bool v)
{
    foreach(const QDockWidget *dockWidget, d->m_dockWidgets)
        dockWidget->toggleViewAction()->setVisible(v);
    d->m_toggleLockedAction->setVisible(v);
    d->m_menuSeparator1->setVisible(v);
    d->m_menuSeparator2->setVisible(v);
    d->m_resetLayoutAction->setVisible(v);
}

} // namespace Utils
