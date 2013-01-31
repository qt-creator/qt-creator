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

#include "modemanager.h"

#include "fancytabwidget.h"
#include "fancyactionbar.h"
#include "icore.h"
#include "mainwindow.h"

#include <aggregation/aggregate.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/imode.h>
#include <coreplugin/id.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QList>
#include <QMap>
#include <QVector>

#include <QSignalMapper>
#include <QShortcut>
#include <QAction>

namespace Core {

/*!
    \class Core::ModeManager

    The mode manager handles everything related to the instances of IMode
    that were added to the plugin manager's object pool as well as their
    buttons and the tool bar with the round buttons in the lower left
    corner of Qt Creator.
*/

struct ModeManagerPrivate
{
    Internal::MainWindow *m_mainWindow;
    Internal::FancyTabWidget *m_modeStack;
    Internal::FancyActionBar *m_actionBar;
    QMap<QAction*, int> m_actions;
    QVector<IMode*> m_modes;
    QVector<Command*> m_modeShortcuts;
    QSignalMapper *m_signalMapper;
    Context m_addedContexts;
    int m_oldCurrent;
    bool m_saveSettingsOnModeChange;
};

static ModeManagerPrivate *d;
static ModeManager *m_instance = 0;

static int indexOf(Id id)
{
    for (int i = 0; i < d->m_modes.count(); ++i) {
        if (d->m_modes.at(i)->id() == id)
            return i;
    }
    qDebug() << "Warning, no such mode:" << id.toString();
    return -1;
}

ModeManager::ModeManager(Internal::MainWindow *mainWindow,
                         Internal::FancyTabWidget *modeStack)
{
    m_instance = this;
    d = new ModeManagerPrivate();
    d->m_mainWindow = mainWindow;
    d->m_modeStack = modeStack;
    d->m_signalMapper = new QSignalMapper(this);
    d->m_oldCurrent = -1;
    d->m_actionBar = new Internal::FancyActionBar(modeStack);
    d->m_modeStack->addCornerWidget(d->m_actionBar);
    d->m_saveSettingsOnModeChange = false;

    connect(d->m_modeStack, SIGNAL(currentAboutToShow(int)), SLOT(currentTabAboutToChange(int)));
    connect(d->m_modeStack, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));
    connect(d->m_signalMapper, SIGNAL(mapped(int)), this, SLOT(slotActivateMode(int)));
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(initializationDone()), this, SLOT(handleStartup()));
    connect(ICore::instance(), SIGNAL(coreAboutToClose()), this, SLOT(handleShutdown()));
}

void ModeManager::init()
{
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject*)),
                     m_instance, SLOT(objectAdded(QObject*)));
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(aboutToRemoveObject(QObject*)),
                     m_instance, SLOT(aboutToRemoveObject(QObject*)));
}

ModeManager::~ModeManager()
{
    delete d;
    d = 0;
    m_instance = 0;
}

void ModeManager::addWidget(QWidget *widget)
{
    // We want the actionbar to stay on the bottom
    // so d->m_modeStack->cornerWidgetCount() -1 inserts it at the position immediately above
    // the actionbar
    d->m_modeStack->insertCornerWidget(d->m_modeStack->cornerWidgetCount() -1, widget);
}

IMode *ModeManager::currentMode()
{
    int currentIndex = d->m_modeStack->currentIndex();
    if (currentIndex < 0)
        return 0;
    return d->m_modes.at(currentIndex);
}

IMode *ModeManager::mode(Id id)
{
    const int index = indexOf(id);
    if (index >= 0)
        return d->m_modes.at(index);
    return 0;
}

void ModeManager::activateModeType(Id type)
{
    if (currentMode() && currentMode()->type() == type)
        return;
    int index = -1;
    for (int i = 0; i < d->m_modes.count(); ++i) {
        if (d->m_modes.at(i)->type() == type) {
            index = i;
            break;
        }
    }
    if (index != -1)
        d->m_modeStack->setCurrentIndex(index);
}

void ModeManager::slotActivateMode(int id)
{
    m_instance->activateMode(Id::fromUniqueIdentifier(id));
}

void ModeManager::activateMode(Id id)
{
    const int index = indexOf(id);
    if (index >= 0)
        d->m_modeStack->setCurrentIndex(index);
}

void ModeManager::objectAdded(QObject *obj)
{
    IMode *mode = Aggregation::query<IMode>(obj);
    if (!mode)
        return;

    d->m_mainWindow->addContextObject(mode);

    // Count the number of modes with a higher priority
    int index = 0;
    foreach (const IMode *m, d->m_modes)
        if (m->priority() > mode->priority())
            ++index;

    d->m_modes.insert(index, mode);
    d->m_modeStack->insertTab(index, mode->widget(), mode->icon(), mode->displayName());
    d->m_modeStack->setTabEnabled(index, mode->isEnabled());

    // Register mode shortcut
    const Id shortcutId = mode->id().withPrefix("QtCreator.Mode.");
    QShortcut *shortcut = new QShortcut(d->m_mainWindow);
    shortcut->setWhatsThis(tr("Switch to <b>%1</b> mode").arg(mode->displayName()));
    Command *cmd = ActionManager::registerShortcut(shortcut, shortcutId, Context(Constants::C_GLOBAL));

    d->m_modeShortcuts.insert(index, cmd);
    connect(cmd, SIGNAL(keySequenceChanged()), this, SLOT(updateModeToolTip()));
    for (int i = 0; i < d->m_modeShortcuts.size(); ++i) {
        Command *currentCmd = d->m_modeShortcuts.at(i);
        // we need this hack with currentlyHasDefaultSequence
        // because we call setDefaultShortcut multiple times on the same cmd
        // and still expect the current shortcut to change with it
        bool currentlyHasDefaultSequence = (currentCmd->keySequence()
                                            == currentCmd->defaultKeySequence());
        currentCmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? QString::fromLatin1("Meta+%1").arg(i+1)
                                                                       : QString::fromLatin1("Ctrl+%1").arg(i+1)));
        if (currentlyHasDefaultSequence)
            currentCmd->setKeySequence(currentCmd->defaultKeySequence());
    }

    d->m_signalMapper->setMapping(shortcut, mode->id().uniqueIdentifier());
    connect(shortcut, SIGNAL(activated()), d->m_signalMapper, SLOT(map()));
    connect(mode, SIGNAL(enabledStateChanged(bool)),
            this, SLOT(enabledStateChanged()));
}

void ModeManager::updateModeToolTip()
{
    Command *cmd = qobject_cast<Command *>(sender());
    if (cmd) {
        int index = d->m_modeShortcuts.indexOf(cmd);
        if (index != -1)
            d->m_modeStack->setTabToolTip(index, cmd->stringWithAppendedShortcut(cmd->shortcut()->whatsThis()));
    }
}

void ModeManager::enabledStateChanged()
{
    IMode *mode = qobject_cast<IMode *>(sender());
    QTC_ASSERT(mode, return);
    int index = d->m_modes.indexOf(mode);
    QTC_ASSERT(index >= 0, return);
    d->m_modeStack->setTabEnabled(index, mode->isEnabled());

    // Make sure we leave any disabled mode to prevent possible crashes:
    if (mode == currentMode() && !mode->isEnabled()) {
        // This assumes that there is always at least one enabled mode.
        for (int i = 0; i < d->m_modes.count(); ++i) {
            if (d->m_modes.at(i) != mode &&
                d->m_modes.at(i)->isEnabled()) {
                activateMode(d->m_modes.at(i)->id());
                break;
            }
        }
    }
}

void ModeManager::handleStartup()
{ d->m_saveSettingsOnModeChange = true; }

void ModeManager::handleShutdown()
{ d->m_saveSettingsOnModeChange = false; }

void ModeManager::aboutToRemoveObject(QObject *obj)
{
    IMode *mode = Aggregation::query<IMode>(obj);
    if (!mode)
        return;

    const int index = d->m_modes.indexOf(mode);
    d->m_modes.remove(index);
    d->m_modeShortcuts.remove(index);
    d->m_modeStack->removeTab(index);

    d->m_mainWindow->removeContextObject(mode);
}

void ModeManager::addAction(QAction *action, int priority)
{
    d->m_actions.insert(action, priority);

    // Count the number of commands with a higher priority
    int index = 0;
    foreach (int p, d->m_actions) {
        if (p > priority)
            ++index;
    }

    d->m_actionBar->insertAction(index, action);
}

void ModeManager::addProjectSelector(QAction *action)
{
    d->m_actionBar->addProjectSelector(action);
    d->m_actions.insert(0, INT_MAX);
}

void ModeManager::currentTabAboutToChange(int index)
{
    if (index >= 0) {
        IMode *mode = d->m_modes.at(index);
        if (mode) {
            if (d->m_saveSettingsOnModeChange)
                ICore::saveSettings();
            emit currentModeAboutToChange(mode);
        }
    }
}

void ModeManager::currentTabChanged(int index)
{
    // Tab index changes to -1 when there is no tab left.
    if (index >= 0) {
        IMode *mode = d->m_modes.at(index);

        // FIXME: This hardcoded context update is required for the Debug and Edit modes, since
        // they use the editor widget, which is already a context widget so the main window won't
        // go further up the parent tree to find the mode context.
        ICore::updateAdditionalContexts(d->m_addedContexts, mode->context());
        d->m_addedContexts = mode->context();

        IMode *oldMode = 0;
        if (d->m_oldCurrent >= 0)
            oldMode = d->m_modes.at(d->m_oldCurrent);
        d->m_oldCurrent = index;
        emit currentModeChanged(mode, oldMode);
    }
}

void ModeManager::setFocusToCurrentMode()
{
    IMode *mode = currentMode();
    QTC_ASSERT(mode, return);
    QWidget *widget = mode->widget();
    if (widget) {
        QWidget *focusWidget = widget->focusWidget();
        if (focusWidget)
            focusWidget->setFocus();
        else
            widget->setFocus();
    }
}

void ModeManager::setModeBarHidden(bool hidden)
{
    d->m_modeStack->setSelectionWidgetHidden(hidden);
}

ModeManager *ModeManager::instance()
{
    return m_instance;
}

} // namespace Core
