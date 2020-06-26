/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
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

#include "cmaketoolmanager.h"

#include "cmaketoolsettingsaccessor.h"

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace Utils;

namespace CMakeProjectManager {

// --------------------------------------------------------------------
// CMakeToolManagerPrivate:
// --------------------------------------------------------------------

class CMakeToolManagerPrivate
{
public:
    Id m_defaultCMake;
    std::vector<std::unique_ptr<CMakeTool>> m_cmakeTools;
    Internal::CMakeToolSettingsAccessor m_accessor;
};
static CMakeToolManagerPrivate *d = nullptr;

// --------------------------------------------------------------------
// CMakeToolManager:
// --------------------------------------------------------------------

CMakeToolManager *CMakeToolManager::m_instance = nullptr;

CMakeToolManager::CMakeToolManager()
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    d = new CMakeToolManagerPrivate;
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &CMakeToolManager::saveCMakeTools);

    connect(this, &CMakeToolManager::cmakeAdded, this, &CMakeToolManager::cmakeToolsChanged);
    connect(this, &CMakeToolManager::cmakeRemoved, this, &CMakeToolManager::cmakeToolsChanged);
    connect(this, &CMakeToolManager::cmakeUpdated, this, &CMakeToolManager::cmakeToolsChanged);
}

CMakeToolManager::~CMakeToolManager()
{
    delete d;
}

CMakeToolManager *CMakeToolManager::instance()
{
    return m_instance;
}

QList<CMakeTool *> CMakeToolManager::cmakeTools()
{
    return Utils::toRawPointer<QList>(d->m_cmakeTools);
}

bool CMakeToolManager::registerCMakeTool(std::unique_ptr<CMakeTool> &&tool)
{
    if (!tool || Utils::contains(d->m_cmakeTools, tool.get()))
        return true;

    const Utils::Id toolId = tool->id();
    QTC_ASSERT(toolId.isValid(),return false);

    //make sure the same id was not used before
    QTC_ASSERT(!Utils::contains(d->m_cmakeTools, [toolId](const std::unique_ptr<CMakeTool> &known) {
        return toolId == known->id();
    }), return false);

    d->m_cmakeTools.emplace_back(std::move(tool));

    emit CMakeToolManager::m_instance->cmakeAdded(toolId);

    ensureDefaultCMakeToolIsValid();

    updateDocumentation();

    return true;
}

void CMakeToolManager::deregisterCMakeTool(const Id &id)
{
    auto toRemove = Utils::take(d->m_cmakeTools, Utils::equal(&CMakeTool::id, id));
    if (toRemove.has_value()) {
        ensureDefaultCMakeToolIsValid();

        updateDocumentation();

        emit m_instance->cmakeRemoved(id);
    }
}

CMakeTool *CMakeToolManager::defaultCMakeTool()
{
    return findById(d->m_defaultCMake);
}

void CMakeToolManager::setDefaultCMakeTool(const Id &id)
{
    if (d->m_defaultCMake != id && findById(id)) {
        d->m_defaultCMake = id;
        emit m_instance->defaultCMakeChanged();
        return;
    }

    ensureDefaultCMakeToolIsValid();
}

CMakeTool *CMakeToolManager::findByCommand(const Utils::FilePath &command)
{
    return Utils::findOrDefault(d->m_cmakeTools, Utils::equal(&CMakeTool::cmakeExecutable, command));
}

CMakeTool *CMakeToolManager::findById(const Id &id)
{
    return Utils::findOrDefault(d->m_cmakeTools, Utils::equal(&CMakeTool::id, id));
}

void CMakeToolManager::restoreCMakeTools()
{
    Internal::CMakeToolSettingsAccessor::CMakeTools tools
            = d->m_accessor.restoreCMakeTools(ICore::dialogParent());
    d->m_cmakeTools = std::move(tools.cmakeTools);
    setDefaultCMakeTool(tools.defaultToolId);

    updateDocumentation();

    emit m_instance->cmakeToolsLoaded();
}

void CMakeToolManager::updateDocumentation()
{
    const QList<CMakeTool *> tools = cmakeTools();
    QStringList docs;
    for (const auto tool : tools) {
        if (!tool->qchFilePath().isEmpty())
            docs.append(tool->qchFilePath().toString());
    }
    Core::HelpManager::registerDocumentation(docs);
}

void CMakeToolManager::notifyAboutUpdate(CMakeTool *tool)
{
    if (!tool || !Utils::contains(d->m_cmakeTools, tool))
        return;
    emit m_instance->cmakeUpdated(tool->id());
}

void CMakeToolManager::saveCMakeTools()
{
    d->m_accessor.saveCMakeTools(cmakeTools(), d->m_defaultCMake, ICore::dialogParent());
}

void CMakeToolManager::ensureDefaultCMakeToolIsValid()
{
    const Utils::Id oldId = d->m_defaultCMake;
    if (d->m_cmakeTools.size() == 0) {
        d->m_defaultCMake = Utils::Id();
    } else {
        if (findById(d->m_defaultCMake))
            return;
        d->m_defaultCMake = d->m_cmakeTools.at(0)->id();
    }

    // signaling:
    if (oldId != d->m_defaultCMake)
        emit m_instance->defaultCMakeChanged();
}

} // namespace CMakeProjectManager
