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
#include <QtGui/QDockWidget>
#include <QtCore/QSettings>

namespace Utils {

struct FancyMainWindowPrivate {
    FancyMainWindowPrivate();

    QList<QDockWidget *> m_dockWidgets;
    QList<bool> m_dockWidgetActiveState;
    bool m_locked;
    bool m_handleDockVisibilityChanges; //todo
};

FancyMainWindowPrivate::FancyMainWindowPrivate() :
    m_locked(true), m_handleDockVisibilityChanges(true)
{
}

FancyMainWindow::FancyMainWindow(QWidget *parent) :
    QMainWindow(parent), d(new FancyMainWindowPrivate)
{
}

FancyMainWindow::~FancyMainWindow()
{
    delete d;
}

QDockWidget *FancyMainWindow::addDockForWidget(QWidget *widget)
{
    QDockWidget *dockWidget = new QDockWidget(widget->windowTitle(), this);
    dockWidget->setObjectName(widget->windowTitle());
    dockWidget->setWidget(widget);
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

void FancyMainWindow::restoreSettings(QSettings *settings)
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
    settings.insert(QLatin1String("State"), saveState());
    settings.insert(QLatin1String("Locked"), d->m_locked);
    for (int i = 0; i < d->m_dockWidgetActiveState.count(); ++i) {
        settings.insert(d->m_dockWidgets.at(i)->objectName(),
                           d->m_dockWidgetActiveState.at(i));
    }
    return settings;
}

void FancyMainWindow::restoreSettings(const QHash<QString, QVariant> &settings)
{
    QByteArray ba = settings.value("State", QByteArray()).toByteArray();
    if (!ba.isEmpty())
        restoreState(ba);
    d->m_locked = settings.value("Locked", true).toBool();
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

} // namespace Utils
