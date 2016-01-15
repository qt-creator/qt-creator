/****************************************************************************
**
** Copyright (C) 2016 Digia Plc and/or its subsidiary(-ies).
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

#include "externaltoolmanager.h"
#include "externaltool.h"
#include "coreconstants.h"
#include "icontext.h"
#include "icore.h"
#include "messagemanager.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/actioncontainer.h"
#include "actionmanager/command.h"

#include <utils/qtcassert.h>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QMenu>

using namespace Core::Internal;

namespace Core {

const char kSpecialUncategorizedSetting[] = "SpecialEmptyCategoryForUncategorizedTools";

struct ExternalToolManagerPrivate
{
    QMap<QString, ExternalTool *> m_tools;
    QMap<QString, QList<ExternalTool *> > m_categoryMap;
    QMap<QString, QAction *> m_actions;
    QMap<QString, ActionContainer *> m_containers;
    QAction *m_configureSeparator;
    QAction *m_configureAction;
};

static ExternalToolManager *m_instance = 0;
static ExternalToolManagerPrivate *d = 0;

static void writeSettings();
static void readSettings(const QMap<QString, ExternalTool *> &tools,
                  QMap<QString, QList<ExternalTool*> > *categoryPriorityMap);

static void parseDirectory(const QString &directory,
                     QMap<QString, QMultiMap<int, ExternalTool*> > *categoryMenus,
                     QMap<QString, ExternalTool *> *tools,
                     bool isPreset = false);

ExternalToolManager::ExternalToolManager()
    : QObject(ICore::instance())
{
    m_instance = this;
    d = new ExternalToolManagerPrivate;

    d->m_configureSeparator = new QAction(this);
    d->m_configureSeparator->setSeparator(true);
    d->m_configureAction = new QAction(ICore::msgShowOptionsDialog(), this);
    connect(d->m_configureAction, &QAction::triggered, [this] {
        ICore::showOptionsDialog(Constants::SETTINGS_ID_TOOLS);
    });

    // add the external tools menu
    ActionContainer *mexternaltools = ActionManager::createMenu(Id(Constants::M_TOOLS_EXTERNAL));
    mexternaltools->menu()->setTitle(ExternalToolManager::tr("&External"));
    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    mtools->addMenu(mexternaltools, Constants::G_DEFAULT_THREE);

    QMap<QString, QMultiMap<int, ExternalTool*> > categoryPriorityMap;
    QMap<QString, ExternalTool *> tools;
    parseDirectory(ICore::userResourcePath() + QLatin1String("/externaltools"),
                   &categoryPriorityMap,
                   &tools);
    parseDirectory(ICore::resourcePath() + QLatin1String("/externaltools"),
                   &categoryPriorityMap,
                   &tools,
                   true);

    QMap<QString, QList<ExternalTool *> > categoryMap;
    QMapIterator<QString, QMultiMap<int, ExternalTool*> > it(categoryPriorityMap);
    while (it.hasNext()) {
        it.next();
        categoryMap.insert(it.key(), it.value().values());
    }

    // read renamed categories and custom order
    readSettings(tools, &categoryMap);
    setToolsByCategory(categoryMap);
}

ExternalToolManager::~ExternalToolManager()
{
    writeSettings();
    // TODO kill running tools
    qDeleteAll(d->m_tools);
    delete d;
}

ExternalToolManager *ExternalToolManager::instance()
{
    return m_instance;
}

static void parseDirectory(const QString &directory,
                           QMap<QString, QMultiMap<int, ExternalTool*> > *categoryMenus,
                           QMap<QString, ExternalTool *> *tools,
                           bool isPreset)
{
    QTC_ASSERT(categoryMenus, return);
    QTC_ASSERT(tools, return);
    QDir dir(directory, QLatin1String("*.xml"), QDir::Unsorted, QDir::Files | QDir::Readable);
    foreach (const QFileInfo &info, dir.entryInfoList()) {
        const QString &fileName = info.absoluteFilePath();
        QString error;
        ExternalTool *tool = ExternalTool::createFromFile(fileName, &error, ICore::userInterfaceLanguage());
        if (!tool) {
            qWarning() << ExternalTool::tr("Error while parsing external tool %1: %2").arg(fileName, error);
            continue;
        }
        if (tools->contains(tool->id())) {
            if (isPreset) {
                // preset that was changed
                ExternalTool *other = tools->value(tool->id());
                other->setPreset(QSharedPointer<ExternalTool>(tool));
            } else {
                qWarning() << ExternalToolManager::tr("Error: External tool in %1 has duplicate id").arg(fileName);
                delete tool;
            }
            continue;
        }
        if (isPreset) {
            // preset that wasn't changed --> save original values
            tool->setPreset(QSharedPointer<ExternalTool>(new ExternalTool(tool)));
        }
        tools->insert(tool->id(), tool);
        (*categoryMenus)[tool->displayCategory()].insert(tool->order(), tool);
    }
}

QMap<QString, QList<ExternalTool *> > ExternalToolManager::toolsByCategory()
{
    return d->m_categoryMap;
}

QMap<QString, ExternalTool *> ExternalToolManager::toolsById()
{
    return d->m_tools;
}

void ExternalToolManager::setToolsByCategory(const QMap<QString, QList<ExternalTool *> > &tools)
{
    // clear menu
    ActionContainer *mexternaltools = ActionManager::actionContainer(Id(Constants::M_TOOLS_EXTERNAL));
    mexternaltools->clear();

    // delete old tools and create list of new ones
    QMap<QString, ExternalTool *> newTools;
    QMap<QString, QAction *> newActions;
    QMapIterator<QString, QList<ExternalTool *> > it(tools);
    while (it.hasNext()) {
        it.next();
        foreach (ExternalTool *tool, it.value()) {
            const QString id = tool->id();
            if (d->m_tools.value(id) == tool) {
                newActions.insert(id, d->m_actions.value(id));
                // remove from list to prevent deletion
                d->m_tools.remove(id);
                d->m_actions.remove(id);
            }
            newTools.insert(id, tool);
        }
    }
    qDeleteAll(d->m_tools);
    QMapIterator<QString, QAction *> remainingActions(d->m_actions);
    const Id externalToolsPrefix = "Tools.External.";
    while (remainingActions.hasNext()) {
        remainingActions.next();
        ActionManager::unregisterAction(remainingActions.value(),
            externalToolsPrefix.withSuffix(remainingActions.key()));
        delete remainingActions.value();
    }
    d->m_actions.clear();
    // assign the new stuff
    d->m_tools = newTools;
    d->m_actions = newActions;
    d->m_categoryMap = tools;
    // create menu structure and remove no-longer used containers
    // add all the category menus, QMap is nicely sorted
    QMap<QString, ActionContainer *> newContainers;
    it.toFront();
    while (it.hasNext()) {
        it.next();
        ActionContainer *container = 0;
        const QString &containerName = it.key();
        if (containerName.isEmpty()) { // no displayCategory, so put into external tools menu directly
            container = mexternaltools;
        } else {
            if (d->m_containers.contains(containerName))
                container = d->m_containers.take(containerName); // remove to avoid deletion below
            else
                container = ActionManager::createMenu(Id("Tools.External.Category.").withSuffix(containerName));
            newContainers.insert(containerName, container);
            mexternaltools->addMenu(container, Constants::G_DEFAULT_ONE);
            container->menu()->setTitle(containerName);
        }
        foreach (ExternalTool *tool, it.value()) {
            const QString &toolId = tool->id();
            // tool action and command
            QAction *action = 0;
            Command *command = 0;
            if (d->m_actions.contains(toolId)) {
                action = d->m_actions.value(toolId);
                command = ActionManager::command(externalToolsPrefix.withSuffix(toolId));
            } else {
                action = new QAction(tool->displayName(), m_instance);
                d->m_actions.insert(toolId, action);
                connect(action, &QAction::triggered, [tool] {
                    ExternalToolRunner *runner = new ExternalToolRunner(tool);
                    if (runner->hasError())
                        MessageManager::write(runner->errorString());
                });

                command = ActionManager::registerAction(action, externalToolsPrefix.withSuffix(toolId));
                command->setAttribute(Command::CA_UpdateText);
            }
            action->setText(tool->displayName());
            action->setToolTip(tool->description());
            action->setWhatsThis(tool->description());
            container->addAction(command, Constants::G_DEFAULT_TWO);
        }
    }

    // delete the unused containers
    qDeleteAll(d->m_containers);
    // remember the new containers
    d->m_containers = newContainers;

    // (re)add the configure menu item
    mexternaltools->menu()->addAction(d->m_configureSeparator);
    mexternaltools->menu()->addAction(d->m_configureAction);
}

static void readSettings(const QMap<QString, ExternalTool *> &tools,
                                       QMap<QString, QList<ExternalTool *> > *categoryMap)
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("ExternalTools"));

    if (categoryMap) {
        settings->beginGroup(QLatin1String("OverrideCategories"));
        foreach (const QString &settingsCategory, settings->childGroups()) {
            QString displayCategory = settingsCategory;
            if (displayCategory == QLatin1String(kSpecialUncategorizedSetting))
                displayCategory = QLatin1String("");
            int count = settings->beginReadArray(settingsCategory);
            for (int i = 0; i < count; ++i) {
                settings->setArrayIndex(i);
                const QString &toolId = settings->value(QLatin1String("Tool")).toString();
                if (tools.contains(toolId)) {
                    ExternalTool *tool = tools.value(toolId);
                    // remove from old category
                    (*categoryMap)[tool->displayCategory()].removeAll(tool);
                    if (categoryMap->value(tool->displayCategory()).isEmpty())
                        categoryMap->remove(tool->displayCategory());
                    // add to new category
                    (*categoryMap)[displayCategory].append(tool);
                }
            }
            settings->endArray();
        }
        settings->endGroup();
    }

    settings->endGroup();
}

static void writeSettings()
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("ExternalTools"));
    settings->remove(QLatin1String(""));

    settings->beginGroup(QLatin1String("OverrideCategories"));
    QMapIterator<QString, QList<ExternalTool *> > it(d->m_categoryMap);
    while (it.hasNext()) {
        it.next();
        QString category = it.key();
        if (category.isEmpty())
            category = QLatin1String(kSpecialUncategorizedSetting);
        settings->beginWriteArray(category, it.value().count());
        int i = 0;
        foreach (ExternalTool *tool, it.value()) {
            settings->setArrayIndex(i);
            settings->setValue(QLatin1String("Tool"), tool->id());
            ++i;
        }
        settings->endArray();
    }
    settings->endGroup();

    settings->endGroup();
}

void ExternalToolManager::emitReplaceSelectionRequested(const QString &output)
{
    emit m_instance->replaceSelectionRequested(output);
}

} // namespace Core
