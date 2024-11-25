// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "filegenerator.h"

#include "../buildsystem/qmlbuildsystem.h"
#include "../qmlprojectconstants.h"
#include "../qmlprojectmanagertr.h"

#include <projectexplorer/projectmanager.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <QMenu>

namespace QmlProjectManager {
namespace QmlProjectExporter {

QAction *FileGenerator::createMenuAction(QObject *parent, const QString &name, const Utils::Id &id)
{
    Core::ActionContainer *fileMenu = Core::ActionManager::actionContainer(Core::Constants::M_FILE);
    Core::ActionContainer *exportMenu = Core::ActionManager::createMenu(
        QmlProjectManager::Constants::EXPORT_MENU);

    exportMenu->menu()->setTitle(Tr::tr("Export Project"));
    exportMenu->appendGroup(QmlProjectManager::Constants::G_EXPORT_GENERATE);
    fileMenu->addMenu(exportMenu, Core::Constants::G_FILE_EXPORT);

    auto action = new QAction(name, parent);
    action->setEnabled(false);
    action->setCheckable(true);

    Core::Command *cmd = Core::ActionManager::registerAction(action, id);
    exportMenu->addAction(cmd, QmlProjectManager::Constants::G_EXPORT_GENERATE);

    return action;
}

void FileGenerator::logIssue(ProjectExplorer::Task::TaskType type,
                             const QString &text,
                             const Utils::FilePath &file)
{
    ProjectExplorer::BuildSystemTask task(type, text, file);
    ProjectExplorer::TaskHub::addTask(task);
    ProjectExplorer::TaskHub::requestPopup();
}

FileGenerator::FileGenerator(QmlBuildSystem *bs)
    : QObject(bs)
    , m_buildSystem(bs)
{}

const QmlProject *FileGenerator::qmlProject() const
{
    if (m_buildSystem)
        return m_buildSystem->qmlProject();
    return nullptr;
}

const QmlBuildSystem *FileGenerator::buildSystem() const
{
    return m_buildSystem;
}

bool FileGenerator::isEnabled() const
{
    return m_enabled;
}

void FileGenerator::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void FileGenerator::updateMenuAction(const Utils::Id &id, std::function<bool(void)> isEnabled)
{
    Core::Command *cmd = Core::ActionManager::command(id);
    if (!cmd)
        return;

    QAction *action = cmd->action();
    if (!action)
        return;

    bool enabled = isEnabled();
    if (enabled != action->isChecked())
        action->setChecked(enabled);
}

} // namespace QmlProjectExporter.
} // namespace QmlProjectManager.
