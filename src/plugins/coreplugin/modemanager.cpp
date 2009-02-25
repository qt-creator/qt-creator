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
#include <coreplugin/uniqueidmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QtCore/QObject>
#include <QtCore/QDebug>
#include <QtCore/QSignalMapper>

#include <QtGui/QAction>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>

using namespace Core;
using namespace Core::Internal;

ModeManager *ModeManager::m_instance = 0;

ModeManager::ModeManager(Internal::MainWindow *mainWindow, FancyTabWidget *modeStack) :
    m_mainWindow(mainWindow),
    m_modeStack(modeStack),
    m_signalMapper(new QSignalMapper(this))
{
    m_instance = this;

    m_actionBar = new FancyActionBar(modeStack);
    m_modeStack->addCornerWidget(m_actionBar);

    connect(m_modeStack, SIGNAL(currentAboutToShow(int)), SLOT(currentTabAboutToChange(int)));
    connect(m_modeStack, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));
    connect(m_signalMapper, SIGNAL(mapped(QString)), this, SLOT(activateMode(QString)));
}

void ModeManager::init()
{
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject*)),
                     this, SLOT(objectAdded(QObject*)));
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(aboutToRemoveObject(QObject*)),
                     this, SLOT(aboutToRemoveObject(QObject*)));
}

void ModeManager::addWidget(QWidget *widget)
{
    // We want the actionbar to stay on the bottom
    // so m_modeStack->cornerWidgetCount() -1 inserts it at the position immediately above
    // the actionbar
    m_modeStack->insertCornerWidget(m_modeStack->cornerWidgetCount() -1, widget);
}

IMode *ModeManager::currentMode() const
{
    return m_modes.at(m_modeStack->currentIndex());
}

int ModeManager::indexOf(const QString &id) const
{
    for (int i = 0; i < m_modes.count(); ++i) {
        if (m_modes.at(i)->uniqueModeName() == id)
            return i;
    }
    qDebug() << "Warning, no such mode:" << id;
    return -1;
}

IMode *ModeManager::mode(const QString &id) const
{
    const int index = indexOf(id);
    if (index >= 0)
        return m_modes.at(index);
    return 0;
}

void ModeManager::activateMode(const QString &id)
{
    const int index = indexOf(id);
    if (index >= 0)
        m_modeStack->setCurrentIndex(index);
}

void ModeManager::objectAdded(QObject *obj)
{
    IMode *mode = Aggregation::query<IMode>(obj);
    if (!mode)
        return;

    m_mainWindow->addContextObject(mode);

    // Count the number of modes with a higher priority
    int index = 0;
    foreach (const IMode *m, m_modes)
        if (m->priority() > mode->priority())
            ++index;

    m_modes.insert(index, mode);
    m_modeStack->insertTab(index, mode->widget(), mode->icon(), mode->name());

    // Register mode shortcut
    ActionManager *am = m_mainWindow->actionManager();
    const QString shortcutId = QLatin1String("QtCreator.Mode.") + mode->uniqueModeName();
    QShortcut *shortcut = new QShortcut(m_mainWindow);
    shortcut->setWhatsThis(tr("Switch to %1 mode").arg(mode->name()));
    Command *cmd = am->registerShortcut(shortcut, shortcutId, QList<int>() << Constants::C_GLOBAL_ID);

    m_modeShortcuts.insert(index, cmd);
    connect(cmd, SIGNAL(keySequenceChanged()), this, SLOT(updateModeToolTip()));
    for (int i = 0; i < m_modeShortcuts.size(); ++i) {
#ifdef Q_OS_MAC
        m_modeShortcuts.at(i)->setDefaultKeySequence(QKeySequence(QString("Meta+%1").arg(i+1)));
#else
        m_modeShortcuts.at(i)->setDefaultKeySequence(QKeySequence(QString("Ctrl+%1").arg(i+1)));
#endif
    }

    m_signalMapper->setMapping(shortcut, mode->uniqueModeName());
    connect(shortcut, SIGNAL(activated()), m_signalMapper, SLOT(map()));
}

void ModeManager::updateModeToolTip()
{
    Command *cmd = qobject_cast<Command *>(sender());
    if (cmd) {
        int index = m_modeShortcuts.indexOf(cmd);
        if (index != -1)
            m_modeStack->setTabToolTip(index, cmd->stringWithAppendedShortcut(cmd->shortcut()->whatsThis()));
    }
}

void ModeManager::aboutToRemoveObject(QObject *obj)
{
    IMode *mode = Aggregation::query<IMode>(obj);
    if (!mode)
        return;

    const int index = m_modes.indexOf(mode);
    m_modes.remove(index);
    m_modeShortcuts.remove(index);
    m_modeStack->removeTab(index);

    m_mainWindow->removeContextObject(mode);
}

void ModeManager::addAction(Command *command, int priority, QMenu *menu)
{
    m_actions.insert(command, priority);

    // Count the number of commands with a higher priority
    int index = 0;
    foreach (int p, m_actions.values())
        if (p > priority)
            ++index;

    m_actionBar->insertAction(index, command->action(), menu);
}

void ModeManager::currentTabAboutToChange(int index)
{
    if (index >= 0) {
        IMode *mode = m_modes.at(index);
        if (mode)
            emit currentModeAboutToChange(mode);
    }
}

void ModeManager::currentTabChanged(int index)
{
    // Tab index changes to -1 when there is no tab left.
    if (index >= 0) {
        IMode *mode = m_modes.at(index);

        // FIXME: This hardcoded context update is required for the Debug and Edit modes, since
        // they use the editor widget, which is already a context widget so the main window won't
        // go further up the parent tree to find the mode context.
        ICore *core = ICore::instance();
        foreach (const int context, m_addedContexts)
            core->removeAdditionalContext(context);

        m_addedContexts = mode->context();
        foreach (const int context, m_addedContexts)
            core->addAdditionalContext(context);
        emit currentModeChanged(mode);
        core->updateContext();
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
