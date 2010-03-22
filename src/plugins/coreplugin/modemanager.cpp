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

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QVector>

#include <QtCore/QSignalMapper>
#include <QtGui/QShortcut>
#include <QtGui/QAction>

namespace Core {

struct ModeManagerPrivate {
    explicit ModeManagerPrivate(Internal::MainWindow *mainWindow,
                                Internal::FancyTabWidget *modeStack,
                                ModeManager *q);

    static ModeManager *m_instance;
    Internal::MainWindow *m_mainWindow;
    Internal::FancyTabWidget *m_modeStack;
    Internal::FancyActionBar *m_actionBar;
    QMap<Command*, int> m_actions;
    QVector<IMode*> m_modes;
    QVector<Command*> m_modeShortcuts;
    QSignalMapper *m_signalMapper;
    QList<int> m_addedContexts;
    int m_oldCurrent;
};

ModeManager *ModeManagerPrivate::m_instance = 0;

ModeManagerPrivate::ModeManagerPrivate(Internal::MainWindow *mainWindow,
                                       Internal::FancyTabWidget *modeStack,
                                       ModeManager *q) :
    m_mainWindow(mainWindow),
    m_modeStack(modeStack),
    m_signalMapper(new QSignalMapper(q)),
    m_oldCurrent(-1)
{
}

ModeManager::ModeManager(Internal::MainWindow *mainWindow,
                         Internal::FancyTabWidget *modeStack) :
        d(new ModeManagerPrivate(mainWindow, modeStack, this))
{
    ModeManagerPrivate::m_instance = this;

    d->m_actionBar = new Internal::FancyActionBar(modeStack);
    d->m_modeStack->addCornerWidget(d->m_actionBar);

    connect(d->m_modeStack, SIGNAL(currentAboutToShow(int)), SLOT(currentTabAboutToChange(int)));
    connect(d->m_modeStack, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));
    connect(d->m_signalMapper, SIGNAL(mapped(QString)), this, SLOT(activateMode(QString)));
}

void ModeManager::init()
{
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject*)),
                     this, SLOT(objectAdded(QObject*)));
    QObject::connect(ExtensionSystem::PluginManager::instance(), SIGNAL(aboutToRemoveObject(QObject*)),
                     this, SLOT(aboutToRemoveObject(QObject*)));
}

ModeManager::~ModeManager()
{
    delete d;
    ModeManagerPrivate::m_instance = 0;
}

void ModeManager::addWidget(QWidget *widget)
{
    // We want the actionbar to stay on the bottom
    // so d->m_modeStack->cornerWidgetCount() -1 inserts it at the position immediately above
    // the actionbar
    d->m_modeStack->insertCornerWidget(d->m_modeStack->cornerWidgetCount() -1, widget);
}

IMode *ModeManager::currentMode() const
{
    int currentIndex = d->m_modeStack->currentIndex();
    if (currentIndex < 0)
        return 0;
    return d->m_modes.at(currentIndex);
}

int ModeManager::indexOf(const QString &id) const
{
    for (int i = 0; i < d->m_modes.count(); ++i) {
        if (d->m_modes.at(i)->id() == id)
            return i;
    }
    qDebug() << "Warning, no such mode:" << id;
    return -1;
}

IMode *ModeManager::mode(const QString &id) const
{
    const int index = indexOf(id);
    if (index >= 0)
        return d->m_modes.at(index);
    return 0;
}

void ModeManager::activateMode(const QString &id)
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
    ActionManager *am = d->m_mainWindow->actionManager();
    const QString shortcutId = QLatin1String("QtCreator.Mode.") + mode->id();
    QShortcut *shortcut = new QShortcut(d->m_mainWindow);
    shortcut->setWhatsThis(tr("Switch to %1 mode").arg(mode->displayName()));
    Command *cmd = am->registerShortcut(shortcut, shortcutId, QList<int>() << Constants::C_GLOBAL_ID);

    d->m_modeShortcuts.insert(index, cmd);
    connect(cmd, SIGNAL(keySequenceChanged()), this, SLOT(updateModeToolTip()));
    for (int i = 0; i < d->m_modeShortcuts.size(); ++i) {
        Command *currentCmd = d->m_modeShortcuts.at(i);
        bool currentlyHasDefaultSequence = (currentCmd->keySequence()
                                            == currentCmd->defaultKeySequence());
#ifdef Q_WS_MAC
        currentCmd->setDefaultKeySequence(QKeySequence(QString("Meta+%1").arg(i+1)));
#else
        currentCmd->setDefaultKeySequence(QKeySequence(QString("Ctrl+%1").arg(i+1)));
#endif
        if (currentlyHasDefaultSequence)
            currentCmd->setKeySequence(currentCmd->defaultKeySequence());
    }

    d->m_signalMapper->setMapping(shortcut, mode->id());
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
}

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

void ModeManager::addAction(Command *command, int priority)
{
    d->m_actions.insert(command, priority);

    // Count the number of commands with a higher priority
    int index = 0;
    foreach (int p, d->m_actions) {
        if (p > priority)
            ++index;
    }

    d->m_actionBar->insertAction(index, command->action());
}

void ModeManager::addProjectSelector(QAction *action)
{
    d->m_actionBar->addProjectSelector(action);
}

void ModeManager::currentTabAboutToChange(int index)
{
    if (index >= 0) {
        IMode *mode = d->m_modes.at(index);
        if (mode)
            emit currentModeAboutToChange(mode);
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
        ICore::instance()->updateAdditionalContexts(d->m_addedContexts, mode->context());
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

ModeManager *ModeManager::instance()
{
    return ModeManagerPrivate::m_instance;
}

} // namespace Core
