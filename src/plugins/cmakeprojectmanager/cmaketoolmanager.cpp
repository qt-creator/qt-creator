// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmaketoolmanager.h"

#include "cmakekitaspect.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "cmaketoolsettingsaccessor.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

#include <nanotrace/nanotrace.h>

using namespace Core;
using namespace Utils;

namespace CMakeProjectManager {

class CMakeToolManagerPrivate
{
public:
    Id m_defaultCMake;
    std::vector<std::unique_ptr<CMakeTool>> m_cmakeTools;
    Internal::CMakeToolSettingsAccessor m_accessor;
};

static CMakeToolManagerPrivate *d = nullptr;
CMakeToolManager *CMakeToolManager::m_instance = nullptr;

CMakeToolManager::CMakeToolManager()
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    qRegisterMetaType<QString *>();

    d = new CMakeToolManagerPrivate;
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &CMakeToolManager::saveCMakeTools);

    connect(this, &CMakeToolManager::cmakeAdded, this, &CMakeToolManager::cmakeToolsChanged);
    connect(this, &CMakeToolManager::cmakeRemoved, this, &CMakeToolManager::cmakeToolsChanged);
    connect(this, &CMakeToolManager::cmakeUpdated, this, &CMakeToolManager::cmakeToolsChanged);

    setObjectName("CMakeToolManager");
    ExtensionSystem::PluginManager::addObject(this);

    CMakeKitAspect::createFactories();
}

CMakeToolManager::~CMakeToolManager()
{
    ExtensionSystem::PluginManager::removeObject(this);
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
    NANOTRACE_SCOPE("CMakeProjectManager", "CMakeToolManager::restoreCMakeTools");
    Internal::CMakeToolSettingsAccessor::CMakeTools tools
            = d->m_accessor.restoreCMakeTools(ICore::dialogParent());
    d->m_cmakeTools = std::move(tools.cmakeTools);
    setDefaultCMakeTool(tools.defaultToolId);

    updateDocumentation();

    emit m_instance->cmakeToolsLoaded();

    // Store the default CMake tool "Autorun CMake" value globally
    // TODO: Remove in Qt Creator 13
    Internal::CMakeSpecificSettings &s = Internal::settings();
    if (s.autorunCMake() == s.autorunCMake.defaultValue()) {
        CMakeTool *cmake = defaultCMakeTool();
        s.autorunCMake.setValue(cmake ? cmake->isAutoRun() : true);
        s.writeSettings();
    }
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

QList<Id> CMakeToolManager::autoDetectCMakeForDevice(const FilePaths &searchPaths,
                                                const QString &detectionSource,
                                                QString *logMessage)
{
    QList<Id> result;
    QStringList messages{Tr::tr("Searching CMake binaries...")};
    for (const FilePath &path : searchPaths) {
        const FilePath cmake = path.pathAppended("cmake").withExecutableSuffix();
        if (cmake.isExecutableFile()) {
            const Id currentId = registerCMakeByPath(cmake, detectionSource);
            if (currentId.isValid())
                result.push_back(currentId);
            messages.append(Tr::tr("Found \"%1\"").arg(cmake.toUserOutput()));
        }
    }
    if (logMessage)
        *logMessage = messages.join('\n');

    return result;
}


Id CMakeToolManager::registerCMakeByPath(const FilePath &cmakePath, const QString &detectionSource)
{
    Id id = Id::fromString(cmakePath.toUserOutput());

    CMakeTool *cmakeTool = findById(id);
    if (cmakeTool)
        return cmakeTool->id();

    auto newTool = std::make_unique<CMakeTool>(CMakeTool::ManualDetection, id);
    newTool->setFilePath(cmakePath);
    newTool->setDetectionSource(detectionSource);
    newTool->setDisplayName(cmakePath.toUserOutput());
    id = newTool->id();
    registerCMakeTool(std::move(newTool));

    return id;
}

void CMakeToolManager::removeDetectedCMake(const QString &detectionSource, QString *logMessage)
{
    QStringList logMessages{Tr::tr("Removing CMake entries...")};
    while (true) {
        auto toRemove = Utils::take(d->m_cmakeTools, Utils::equal(&CMakeTool::detectionSource, detectionSource));
        if (!toRemove.has_value())
            break;
        logMessages.append(Tr::tr("Removed \"%1\"").arg((*toRemove)->displayName()));
        emit m_instance->cmakeRemoved((*toRemove)->id());
    }

    ensureDefaultCMakeToolIsValid();
    updateDocumentation();
    if (logMessage)
        *logMessage = logMessages.join('\n');
}

void CMakeToolManager::listDetectedCMake(const QString &detectionSource, QString *logMessage)
{
    QTC_ASSERT(logMessage, return);
    QStringList logMessages{Tr::tr("CMake:")};
    for (const auto &tool : std::as_const(d->m_cmakeTools)) {
        if (tool->detectionSource() == detectionSource)
            logMessages.append(tool->displayName());
    }
    *logMessage = logMessages.join('\n');
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
        auto cmakeTool = Utils::findOrDefault(
                    cmakeTools(), [](CMakeTool *tool){ return tool->detectionSource().isEmpty(); });
        if (cmakeTool)
            d->m_defaultCMake = cmakeTool->id();
    }

    // signaling:
    if (oldId != d->m_defaultCMake)
        emit m_instance->defaultCMakeChanged();
}

} // CMakeProjectManager
