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

#include "modemanager.h"

#include "fancytabwidget.h"
#include "fancyactionbar.h"
#include "icore.h"
#include "mainwindow.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/imode.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QAction>
#include <QDebug>
#include <QMap>
#include <QMouseEvent>
#include <QVector>

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
    void showMenu(int index, QMouseEvent *event);

    Internal::MainWindow *m_mainWindow;
    Internal::FancyTabWidget *m_modeStack;
    Internal::FancyActionBar *m_actionBar;
    QMap<QAction*, int> m_actions;
    QVector<IMode*> m_modes;
    QVector<Command*> m_modeCommands;
    Context m_addedContexts;
    int m_oldCurrent;
    bool m_modeSelectorVisible;
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

void ModeManagerPrivate::showMenu(int index, QMouseEvent *event)
{
    QTC_ASSERT(m_modes.at(index)->menu(), return);
    m_modes.at(index)->menu()->popup(event->globalPos());
}

ModeManager::ModeManager(Internal::MainWindow *mainWindow,
                         Internal::FancyTabWidget *modeStack)
{
    m_instance = this;
    d = new ModeManagerPrivate();
    d->m_mainWindow = mainWindow;
    d->m_modeStack = modeStack;
    d->m_oldCurrent = -1;
    d->m_actionBar = new Internal::FancyActionBar(modeStack);
    d->m_modeStack->addCornerWidget(d->m_actionBar);
    d->m_modeSelectorVisible = true;
    d->m_modeStack->setSelectionWidgetVisible(d->m_modeSelectorVisible);

    connect(d->m_modeStack, &Internal::FancyTabWidget::currentAboutToShow,
            this, &ModeManager::currentTabAboutToChange);
    connect(d->m_modeStack, &Internal::FancyTabWidget::currentChanged,
            this, &ModeManager::currentTabChanged);
    connect(d->m_modeStack, &Internal::FancyTabWidget::menuTriggered,
            this, [](int index, QMouseEvent *e) { d->showMenu(index, e); });
}

void ModeManager::init()
{
    QObject::connect(ExtensionSystem::PluginManager::instance(), &ExtensionSystem::PluginManager::objectAdded,
                     m_instance, &ModeManager::objectAdded);
    QObject::connect(ExtensionSystem::PluginManager::instance(), &ExtensionSystem::PluginManager::aboutToRemoveObject,
                     m_instance, &ModeManager::aboutToRemoveObject);
}

ModeManager::~ModeManager()
{
    delete d;
    d = 0;
    m_instance = 0;
}

Id ModeManager::currentMode()
{
    int currentIndex = d->m_modeStack->currentIndex();
    if (currentIndex < 0)
        return Id();
    return d->m_modes.at(currentIndex)->id();
}

static IMode *findMode(Id id)
{
    const int index = indexOf(id);
    if (index >= 0)
        return d->m_modes.at(index);
    return 0;
}

void ModeManager::activateMode(Id id)
{
    const int currentIndex = d->m_modeStack->currentIndex();
    const int newIndex = indexOf(id);
    if (newIndex != currentIndex && newIndex >= 0)
        d->m_modeStack->setCurrentIndex(newIndex);
}

void ModeManager::objectAdded(QObject *obj)
{
    IMode *mode = qobject_cast<IMode *>(obj);
    if (!mode)
        return;

    d->m_mainWindow->addContextObject(mode);

    // Count the number of modes with a higher priority
    int index = 0;
    foreach (const IMode *m, d->m_modes)
        if (m->priority() > mode->priority())
            ++index;

    d->m_modes.insert(index, mode);
    d->m_modeStack->insertTab(index, mode->widget(), mode->icon(), mode->displayName(),
                              mode->menu() != nullptr);
    d->m_modeStack->setTabEnabled(index, mode->isEnabled());

    // Register mode shortcut
    const Id actionId = mode->id().withPrefix("QtCreator.Mode.");
    QAction *action = new QAction(tr("Switch to <b>%1</b> mode").arg(mode->displayName()), this);
    Command *cmd = ActionManager::registerAction(action, actionId);

    d->m_modeCommands.insert(index, cmd);
    connect(cmd, &Command::keySequenceChanged, m_instance, &ModeManager::updateModeToolTip);
    for (int i = 0; i < d->m_modeCommands.size(); ++i) {
        Command *currentCmd = d->m_modeCommands.at(i);
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

    Id id = mode->id();
    connect(action, &QAction::triggered, [id] {
        m_instance->activateMode(id);
        ICore::raiseWindow(d->m_modeStack);
    });

    connect(mode, &IMode::enabledStateChanged,
            m_instance, &ModeManager::enabledStateChanged);
}

void ModeManager::updateModeToolTip()
{
    Command *cmd = qobject_cast<Command *>(sender());
    if (cmd) {
        int index = d->m_modeCommands.indexOf(cmd);
        if (index != -1)
            d->m_modeStack->setTabToolTip(index, cmd->action()->toolTip());
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
    if (mode->id() == currentMode() && !mode->isEnabled()) {
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

void ModeManager::aboutToRemoveObject(QObject *obj)
{
    IMode *mode = qobject_cast<IMode *>(obj);
    if (!mode)
        return;

    const int index = d->m_modes.indexOf(mode);
    d->m_modes.remove(index);
    d->m_modeCommands.remove(index);
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
        if (mode)
            emit currentModeAboutToChange(mode->id());
    }
}

void ModeManager::currentTabChanged(int index)
{
    // Tab index changes to -1 when there is no tab left.
    if (index < 0)
        return;

    IMode *mode = d->m_modes.at(index);
    if (!mode)
        return;

    // FIXME: This hardcoded context update is required for the Debug and Edit modes, since
    // they use the editor widget, which is already a context widget so the main window won't
    // go further up the parent tree to find the mode context.
    ICore::updateAdditionalContexts(d->m_addedContexts, mode->context());
    d->m_addedContexts = mode->context();

    IMode *oldMode = nullptr;
    if (d->m_oldCurrent >= 0)
        oldMode = d->m_modes.at(d->m_oldCurrent);
    d->m_oldCurrent = index;
    emit currentModeChanged(mode->id(), oldMode ? oldMode->id() : Id());
}

void ModeManager::setFocusToCurrentMode()
{
    IMode *mode = findMode(currentMode());
    QTC_ASSERT(mode, return);
    QWidget *widget = mode->widget();
    if (widget) {
        QWidget *focusWidget = widget->focusWidget();
        if (!focusWidget)
            focusWidget = widget;
        focusWidget->setFocus();
    }
}

void ModeManager::setModeSelectorVisible(bool visible)
{
    d->m_modeSelectorVisible = visible;
    d->m_modeStack->setSelectionWidgetVisible(visible);
}

bool ModeManager::isModeSelectorVisible()
{
    return d->m_modeSelectorVisible;
}

ModeManager *ModeManager::instance()
{
    return m_instance;
}

} // namespace Core
