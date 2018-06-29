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

#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/persistentsettings.h>
#include <utils/pointeralgorithm.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QDebug>
#include <QDir>

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;

namespace CMakeProjectManager {

// --------------------------------------------------------------------
// CMakeToolManagerPrivate:
// --------------------------------------------------------------------

class CMakeToolManagerPrivate
{
public:
    Id m_defaultCMake;
    std::vector<std::unique_ptr<CMakeTool>> m_cmakeTools;
    PersistentSettingsWriter *m_writer =  nullptr;
};
static CMakeToolManagerPrivate *d = nullptr;

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

const char CMAKETOOL_COUNT_KEY[] = "CMakeTools.Count";
const char CMAKETOOL_DEFAULT_KEY[] = "CMakeTools.Default";
const char CMAKETOOL_DATA_KEY[] = "CMakeTools.";
const char CMAKETOOL_FILE_VERSION_KEY[] = "Version";
const char CMAKETOOL_FILENAME[] = "/cmaketools.xml";

static FileName userSettingsFileName()
{
    return FileName::fromString(ICore::userResourcePath() + CMAKETOOL_FILENAME);
}

static std::vector<std::unique_ptr<CMakeTool>>
readCMakeTools(const FileName &fileName, Core::Id *defaultId, bool fromSDK)
{
    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return {};

    QVariantMap data = reader.restoreValues();

    // Check version
    int version = data.value(QLatin1String(CMAKETOOL_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return {};

    std::vector<std::unique_ptr<CMakeTool>> loaded;

    int count = data.value(QLatin1String(CMAKETOOL_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(CMAKETOOL_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap dbMap = data.value(key).toMap();
        auto item = std::make_unique<CMakeTool>(dbMap, fromSDK);
        if (item->isAutoDetected()) {
            if (!item->cmakeExecutable().toFileInfo().isExecutable()) {
                qWarning() << QString::fromLatin1("CMakeTool \"%1\" (%2) read from \"%3\" dropped since the command is not executable.")
                              .arg(item->cmakeExecutable().toUserOutput(), item->id().toString(), fileName.toUserOutput());
                continue;
            }
        }

        loaded.emplace_back(std::move(item));
    }

    *defaultId = Id::fromSetting(data.value(QLatin1String(CMAKETOOL_DEFAULT_KEY), defaultId->toSetting()));

    return loaded;
}

static std::vector<std::unique_ptr<CMakeTool>> autoDetectCMakeTools()
{
    Utils::Environment env = Environment::systemEnvironment();

    Utils::FileNameList path = env.path();
    path = Utils::filteredUnique(path);

    if (HostOsInfo::isWindowsHost()) {
        const QString progFiles = QLatin1String(qgetenv("ProgramFiles"));
        path.append(Utils::FileName::fromString(progFiles + "/CMake"));
        path.append(Utils::FileName::fromString(progFiles + "/CMake/bin"));
        const QString progFilesX86 = QLatin1String(qgetenv("ProgramFiles(x86)"));
        if (!progFilesX86.isEmpty()) {
            path.append(Utils::FileName::fromString(progFilesX86 + "/CMake"));
            path.append(Utils::FileName::fromString(progFilesX86 + "/CMake/bin"));
        }
    }

    if (HostOsInfo::isMacHost()) {
        path.append(Utils::FileName::fromString("/Applications/CMake.app/Contents/bin"));
        path.append(Utils::FileName::fromString("/usr/local/bin"));
        path.append(Utils::FileName::fromString("/opt/local/bin"));
    }

    const QStringList execs = env.appendExeExtensions(QLatin1String("cmake"));

    FileNameList suspects;
    foreach (const Utils::FileName &base, path) {
        if (base.isEmpty())
            continue;

        QFileInfo fi;
        for (const QString &exec : execs) {
            fi.setFile(QDir(base.toString()), exec);
            if (fi.exists() && fi.isFile() && fi.isExecutable())
                suspects << FileName::fromString(fi.absoluteFilePath());
        }
    }

    std::vector<std::unique_ptr<CMakeTool>> found;
    foreach (const FileName &command, suspects) {
        auto item = std::make_unique<CMakeTool>(CMakeTool::AutoDetection, CMakeTool::createId());
        item->setCMakeExecutable(command);
        item->setDisplayName(CMakeToolManager::tr("System CMake at %1").arg(command.toUserOutput()));

        found.emplace_back(std::move(item));
    }

    return found;
}

static std::vector<std::unique_ptr<CMakeTool>>
mergeTools(std::vector<std::unique_ptr<CMakeTool>> &sdkTools,
           std::vector<std::unique_ptr<CMakeTool>> &userTools,
           std::vector<std::unique_ptr<CMakeTool>> &autoDetectedTools)
{
    std::vector<std::unique_ptr<CMakeTool>> result;
    while (userTools.size() > 0) {
        std::unique_ptr<CMakeTool> userTool = std::move(userTools[0]);
        userTools.erase(std::begin(userTools));

        if (auto sdkTool = Utils::take(sdkTools, Utils::equal(&CMakeTool::id, userTool->id()))) {
            result.emplace_back(std::move(sdkTool.value()));
        } else {
            if (userTool->isAutoDetected()
                    && !Utils::contains(autoDetectedTools, Utils::equal(&CMakeTool::cmakeExecutable,
                                                                        userTool->cmakeExecutable()))) {

                qWarning() << QString::fromLatin1("Previously SDK provided CMakeTool \"%1\" (%2) dropped.")
                              .arg(userTool->cmakeExecutable().toUserOutput(), userTool->id().toString());
                continue;
            }
            result.emplace_back(std::move(userTool));
        }
    }

    // add all the autodetected tools that are not known yet
    while (autoDetectedTools.size() > 0) {
        std::unique_ptr<CMakeTool> autoDetectedTool = std::move(autoDetectedTools[0]);
        autoDetectedTools.erase(std::begin(autoDetectedTools));

        if (!Utils::contains(result,
                             Utils::equal(&CMakeTool::cmakeExecutable, autoDetectedTool->cmakeExecutable())))
            result.emplace_back(std::move(autoDetectedTool));
    }

    return result;
}

// --------------------------------------------------------------------
// CMakeToolManager:
// --------------------------------------------------------------------

CMakeToolManager *CMakeToolManager::m_instance = nullptr;

CMakeToolManager::CMakeToolManager(QObject *parent) : QObject(parent)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    d = new CMakeToolManagerPrivate;
    d->m_writer = new PersistentSettingsWriter(userSettingsFileName(), QStringLiteral("QtCreatorCMakeTools"));
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &CMakeToolManager::saveCMakeTools);

    connect(this, &CMakeToolManager::cmakeAdded, this, &CMakeToolManager::cmakeToolsChanged);
    connect(this, &CMakeToolManager::cmakeRemoved, this, &CMakeToolManager::cmakeToolsChanged);
    connect(this, &CMakeToolManager::cmakeUpdated, this, &CMakeToolManager::cmakeToolsChanged);
}

CMakeToolManager::~CMakeToolManager()
{
    delete d->m_writer;
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

Id CMakeToolManager::registerOrFindCMakeTool(const FileName &command)
{
    if (CMakeTool  *cmake = findByCommand(command))
        return cmake->id();

    auto cmake = std::make_unique<CMakeTool>(CMakeTool::ManualDetection, CMakeTool::createId());
    cmake->setCMakeExecutable(command);
    cmake->setDisplayName(tr("CMake at %1").arg(command.toUserOutput()));

    Core::Id id = cmake->id();
    QTC_ASSERT(registerCMakeTool(std::move(cmake)), return Core::Id());
    return id;
}

bool CMakeToolManager::registerCMakeTool(std::unique_ptr<CMakeTool> &&tool)
{
    if (!tool || Utils::contains(d->m_cmakeTools, tool.get()))
        return true;

    const Core::Id toolId = tool->id();
    QTC_ASSERT(toolId.isValid(),return false);

    //make sure the same id was not used before
    QTC_ASSERT(!Utils::contains(d->m_cmakeTools, [toolId](const std::unique_ptr<CMakeTool> &known) {
        return toolId == known->id();
    }), return false);

    d->m_cmakeTools.emplace_back(std::move(tool));

    emit CMakeToolManager::m_instance->cmakeAdded(toolId);

    ensureDefaultCMakeToolIsValid();

    return true;
}

void CMakeToolManager::deregisterCMakeTool(const Id &id)
{
    auto toRemove = Utils::take(d->m_cmakeTools, Utils::equal(&CMakeTool::id, id));
    if (toRemove.has_value()) {

        ensureDefaultCMakeToolIsValid();

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

CMakeTool *CMakeToolManager::findByCommand(const FileName &command)
{
    return Utils::findOrDefault(d->m_cmakeTools, Utils::equal(&CMakeTool::cmakeExecutable, command));
}

CMakeTool *CMakeToolManager::findById(const Id &id)
{
    return Utils::findOrDefault(d->m_cmakeTools, Utils::equal(&CMakeTool::id, id));
}

void CMakeToolManager::restoreCMakeTools()
{
    Core::Id defaultId;

    const FileName sdkSettingsFile = FileName::fromString(ICore::installerResourcePath()
                                                          + CMAKETOOL_FILENAME);

    std::vector<std::unique_ptr<CMakeTool>> sdkTools
            = readCMakeTools(sdkSettingsFile, &defaultId, true);

    //read the tools from the user settings file
    std::vector<std::unique_ptr<CMakeTool>> userTools
            = readCMakeTools(userSettingsFileName(), &defaultId, false);

    //autodetect tools
    std::vector<std::unique_ptr<CMakeTool>> autoDetectedTools = autoDetectCMakeTools();

    //filter out the tools that were stored in SDK
    std::vector<std::unique_ptr<CMakeTool>> toRegister = mergeTools(sdkTools, userTools, autoDetectedTools);

    // Store all tools
    for (auto it = std::begin(toRegister); it != std::end(toRegister); ++it) {
        const Utils::FileName executable = (*it)->cmakeExecutable();
        const Core::Id id = (*it)->id();

        if (!registerCMakeTool(std::move(*it))) {
            //this should never happen, but lets make sure we do not leak memory
            qWarning() << QString::fromLatin1("CMakeTool \"%1\" (%2) dropped.")
                          .arg(executable.toUserOutput(), id.toString());
        }
    }

    setDefaultCMakeTool(defaultId);

    emit m_instance->cmakeToolsLoaded();
}

void CMakeToolManager::notifyAboutUpdate(CMakeTool *tool)
{
    if (!tool || !Utils::contains(d->m_cmakeTools, tool))
        return;
    emit m_instance->cmakeUpdated(tool->id());
}

void CMakeToolManager::saveCMakeTools()
{
    QTC_ASSERT(d->m_writer, return);
    QVariantMap data;
    data.insert(QLatin1String(CMAKETOOL_FILE_VERSION_KEY), 1);
    data.insert(QLatin1String(CMAKETOOL_DEFAULT_KEY), d->m_defaultCMake.toSetting());

    int count = 0;
    for (const std::unique_ptr<CMakeTool> &item : d->m_cmakeTools) {
        QFileInfo fi = item->cmakeExecutable().toFileInfo();

        if (fi.isExecutable()) {
            QVariantMap tmp = item->toMap();
            if (tmp.isEmpty())
                continue;
            data.insert(QString::fromLatin1(CMAKETOOL_DATA_KEY) + QString::number(count), tmp);
            ++count;
        }
    }
    data.insert(QLatin1String(CMAKETOOL_COUNT_KEY), count);
    d->m_writer->save(data, ICore::mainWindow());
}

void CMakeToolManager::ensureDefaultCMakeToolIsValid()
{
    const Core::Id oldId = d->m_defaultCMake;
    if (d->m_cmakeTools.size() == 0) {
        d->m_defaultCMake = Core::Id();
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
