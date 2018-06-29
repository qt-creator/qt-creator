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

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/persistentsettings.h>
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
    QList<CMakeTool *> m_cmakeTools;
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

static QList<CMakeTool *> readCMakeTools(const FileName &fileName, Core::Id *defaultId, bool fromSDK)
{
    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return {};

    QVariantMap data = reader.restoreValues();

    // Check version
    int version = data.value(QLatin1String(CMAKETOOL_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return {};

    QList<CMakeTool *> loaded;

    int count = data.value(QLatin1String(CMAKETOOL_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(CMAKETOOL_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap dbMap = data.value(key).toMap();
        auto item = new CMakeTool(dbMap,fromSDK);
        if (item->isAutoDetected()) {
            if (!item->cmakeExecutable().toFileInfo().isExecutable()) {
                qWarning() << QString::fromLatin1("CMakeTool \"%1\" (%2) read from \"%3\" dropped since the command is not executable.")
                              .arg(item->cmakeExecutable().toUserOutput(), item->id().toString(), fileName.toUserOutput());
                delete item;
                continue;
            }
        }

        loaded.append(item);
    }

    *defaultId = Id::fromSetting(data.value(QLatin1String(CMAKETOOL_DEFAULT_KEY), defaultId->toSetting()));

    return loaded;
}

static QList<CMakeTool *> autoDetectCMakeTools()
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

    QList<CMakeTool *> found;
    foreach (const FileName &command, suspects) {
        auto item = new CMakeTool(CMakeTool::AutoDetection, CMakeTool::createId());
        item->setCMakeExecutable(command);
        item->setDisplayName(CMakeToolManager::tr("System CMake at %1").arg(command.toUserOutput()));

        found.append(item);
    }

    return found;
}

static QList<CMakeTool *> mergeTools(QList<CMakeTool *> &sdkTools,
                                     QList<CMakeTool *> &userTools,
                                     QList<CMakeTool *> &autoDetectedTools)
{
    QList<CMakeTool *> result;
    for (int i = userTools.size() - 1; i >= 0; i--) {
        CMakeTool *currTool = userTools.takeAt(i);
        if (CMakeTool *sdk = Utils::findOrDefault(sdkTools, Utils::equal(&CMakeTool::id, currTool->id()))) {
            delete currTool;
            result.append(sdk);
        } else {
            //if the current tool is marked as autodetected and NOT in the autodetected list,
            //it is a leftover SDK provided tool. The user will not be able to edit it,
            //so we automatically drop it
            if (currTool->isAutoDetected()) {
                if (!Utils::anyOf(autoDetectedTools,
                                  Utils::equal(&CMakeTool::cmakeExecutable, currTool->cmakeExecutable()))) {

                    qWarning() << QString::fromLatin1("Previously SDK provided CMakeTool \"%1\" (%2) dropped.")
                                  .arg(currTool->cmakeExecutable().toUserOutput(), currTool->id().toString());

                    delete currTool;
                    continue;
                }
            }
            result.append(currTool);
        }
    }

    //filter out the tools that are already known
    while (autoDetectedTools.size()) {
        CMakeTool *currTool = autoDetectedTools.takeFirst();
        if (Utils::anyOf(result,
                         Utils::equal(&CMakeTool::cmakeExecutable, currTool->cmakeExecutable())))
            delete currTool;
        else
            result.append(currTool);
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
    qDeleteAll(d->m_cmakeTools);
    delete d;
}

CMakeToolManager *CMakeToolManager::instance()
{
    return m_instance;
}

QList<CMakeTool *> CMakeToolManager::cmakeTools()
{
    return d->m_cmakeTools;
}

Id CMakeToolManager::registerOrFindCMakeTool(const FileName &command)
{
    CMakeTool  *cmake = findByCommand(command);
    if (cmake)
        return cmake->id();

    cmake = new CMakeTool(CMakeTool::ManualDetection, CMakeTool::createId());
    cmake->setCMakeExecutable(command);
    cmake->setDisplayName(tr("CMake at %1").arg(command.toUserOutput()));

    QTC_ASSERT(registerCMakeTool(cmake), return Core::Id());
    return cmake->id();
}

bool CMakeToolManager::registerCMakeTool(CMakeTool *tool)
{
    if (!tool || d->m_cmakeTools.contains(tool))
        return true;

    QTC_ASSERT(tool->id().isValid(),return false);

    //make sure the same id was not used before
    foreach (CMakeTool *current, d->m_cmakeTools) {
        if (tool->id() == current->id())
            return false;
    }

    d->m_cmakeTools.append(tool);

    emit CMakeToolManager::m_instance->cmakeAdded(tool->id());

    //set the first registered cmake tool as default if there is not already one
    if (!d->m_defaultCMake.isValid())
        CMakeToolManager::setDefaultCMakeTool(tool->id());

    return true;
}

void CMakeToolManager::deregisterCMakeTool(const Id &id)
{
    int idx = Utils::indexOf(d->m_cmakeTools, Utils::equal(&CMakeTool::id, id));
    if (idx >= 0) {
        CMakeTool *toRemove = d->m_cmakeTools.takeAt(idx);
        if (toRemove->id() == d->m_defaultCMake) {
            if (d->m_cmakeTools.isEmpty())
                d->m_defaultCMake = Id();
            else
                d->m_defaultCMake = d->m_cmakeTools.first()->id();

            emit m_instance->defaultCMakeChanged();
        }

        emit m_instance->cmakeRemoved(id);
        delete toRemove;
    }
}

CMakeTool *CMakeToolManager::defaultCMakeTool()
{
    CMakeTool *tool = findById(d->m_defaultCMake);
    if (!tool) {
        //if the id is not valid, we set the firstly registered one as default
        if (!d->m_cmakeTools.isEmpty()) {
            d->m_defaultCMake = d->m_cmakeTools.first()->id();
            emit m_instance->defaultCMakeChanged();

            return d->m_cmakeTools.first();
        }
    }
    return tool;
}

void CMakeToolManager::setDefaultCMakeTool(const Id &id)
{
    if (d->m_defaultCMake == id)
        return;

    if (findById(id)) {
        d->m_defaultCMake = id;
        emit m_instance->defaultCMakeChanged();
    }
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

    QList<CMakeTool *> sdkTools = readCMakeTools(sdkSettingsFile, &defaultId, true);

    //read the tools from the user settings file
    QList<CMakeTool *> userTools = readCMakeTools(userSettingsFileName(), &defaultId, false);

    //autodetect tools
    QList<CMakeTool *> autoDetectedTools = autoDetectCMakeTools();

    //filter out the tools that were stored in SDK
    const QList<CMakeTool *> toRegister = mergeTools(sdkTools, userTools, autoDetectedTools);

    // Store all tools
    foreach (CMakeTool *current, toRegister) {
        if (!registerCMakeTool(current)) {
            //this should never happen, but lets make sure we do not leak memory
            qWarning() << QString::fromLatin1("CMakeTool \"%1\" (%2) dropped.")
                          .arg(current->cmakeExecutable().toUserOutput(), current->id().toString());

            delete current;
        }
    }

    if (CMakeToolManager::findById(defaultId))
        d->m_defaultCMake = defaultId;

    emit m_instance->cmakeToolsLoaded();
}

void CMakeToolManager::notifyAboutUpdate(CMakeTool *tool)
{
    if (!tool || !d->m_cmakeTools.contains(tool))
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
    foreach (CMakeTool *item, d->m_cmakeTools) {
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

} // namespace CMakeProjectManager
