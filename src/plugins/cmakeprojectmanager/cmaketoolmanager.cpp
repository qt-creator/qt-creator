/****************************************************************************
**
** Copyright (C) 2015 Canonical Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmaketoolmanager.h"

#include <coreplugin/icore.h>
#include <projectexplorer/toolchainmanager.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/environment.h>
#include <utils/algorithm.h>

#include <QFileInfo>
#include <QDebug>
#include <QDir>

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;

namespace CMakeProjectManager {

const char CMAKETOOL_COUNT_KEY[] = "CMakeTools.Count";
const char CMAKETOOL_DEFAULT_KEY[] = "CMakeTools.Default";
const char CMAKETOOL_PREFER_NINJA_KEY[] = "CMakeTools.PreferNinja";
const char CMAKETOOL_DATA_KEY[] = "CMakeTools.";
const char CMAKETOOL_FILE_VERSION_KEY[] = "Version";
const char CMAKETOOL_FILENAME[] = "/qtcreator/cmaketools.xml";

class CMakeToolManagerPrivate
{

public:
    CMakeToolManagerPrivate() :
        m_preferNinja(false),
        m_writer(0)
    {}

    bool m_preferNinja;
    Id m_defaultCMake;
    QList<CMakeTool *> m_cmakeTools;
    PersistentSettingsWriter *m_writer;
    QList<CMakeToolManager::AutodetectionHelper> m_autoDetectionHelpers;
};
static CMakeToolManagerPrivate *d = 0;

static void addCMakeTool(CMakeTool *item)
{
    QTC_ASSERT(item->id().isValid(), return);

    d->m_cmakeTools.append(item);

    //set the first registered cmake tool as default if there is not already one
    if (!d->m_defaultCMake.isValid())
        CMakeToolManager::setDefaultCMakeTool(item->id());
}

static FileName userSettingsFileName()
{
    QFileInfo settingsLocation(ICore::settings()->fileName());
    return FileName::fromString(settingsLocation.absolutePath() + QLatin1String(CMAKETOOL_FILENAME));
}

static QList<CMakeTool *> readCMakeTools(const FileName &fileName, Core::Id *defaultId, bool fromSDK)
{
    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return QList<CMakeTool *>();

    QVariantMap data = reader.restoreValues();

    // Check version
    int version = data.value(QLatin1String(CMAKETOOL_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return QList<CMakeTool *>();

    QList<CMakeTool *> loaded;

    int count = data.value(QLatin1String(CMAKETOOL_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(CMAKETOOL_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap dbMap = data.value(key).toMap();
        CMakeTool *item = new CMakeTool(dbMap,fromSDK);
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
    d->m_preferNinja= data.value(QLatin1String(CMAKETOOL_PREFER_NINJA_KEY), d->m_preferNinja).toBool();

    return loaded;
}

static void readAndDeleteLegacyCMakeSettings ()
{
    // restore the legacy cmake
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("CMakeSettings"));

    FileName exec = FileName::fromUserInput(settings->value(QLatin1String("cmakeExecutable")).toString());
    if (exec.toFileInfo().isExecutable()) {
        CMakeTool *item = CMakeToolManager::findByCommand(exec);
        if (!item) {
            item = new CMakeTool(CMakeTool::ManualDetection);
            item->setCMakeExecutable(exec);
            item->setDisplayName(CMakeToolManager::tr("CMake at %1").arg(item->cmakeExecutable().toUserOutput()));

            if (!CMakeToolManager::registerCMakeTool(item)) {
                delete item;
                item = 0;
            }
        }

        //this setting used to be the default cmake, make sure it is again
        if (item)
            d->m_defaultCMake = item->id();
    }

    //read the legacy ninja setting, if its not available use the current value
    d->m_preferNinja = settings->value(QLatin1String("preferNinja"), d->m_preferNinja).toBool();

    settings->remove(QString());
    settings->endGroup();
}

static QList<CMakeTool *> autoDetectCMakeTools()
{
    QList<FileName> suspects;

    Utils::Environment env = Environment::systemEnvironment();

    QStringList path = env.path();
    path.removeDuplicates();

    QStringList execs = env.appendExeExtensions(QLatin1String("cmake"));

    foreach (QString base, path) {
        const QChar slash = QLatin1Char('/');
        if (base.isEmpty())
            continue;
        // Avoid turning '/' into '//' on Windows which triggers Windows to check
        // for network drives!
        if (!base.endsWith(slash))
            base += slash;

        foreach (const QString &exec, execs) {
            QFileInfo fi(base + exec);
            if (fi.exists() && fi.isFile() && fi.isExecutable())
                suspects << FileName::fromString(fi.absoluteFilePath());
        }
    }

    QList<CMakeTool *> found;
    foreach (const FileName &command, suspects) {
        CMakeTool *item = new CMakeTool(CMakeTool::AutoDetection);
        item->setCMakeExecutable(command);
        item->setDisplayName(CMakeToolManager::tr("System CMake at %1").arg(command.toUserOutput()));

        found.append(item);
    }

    //execute custom helpers if available
    foreach (CMakeToolManager::AutodetectionHelper source, d->m_autoDetectionHelpers)
        found.append(source());

    return found;
}

CMakeToolManager *CMakeToolManager::m_instance = 0;

CMakeToolManager::CMakeToolManager(QObject *parent) :
    QObject(parent)
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
    d = 0;
}

CMakeToolManager *CMakeToolManager::instance()
{
    return m_instance;
}

QList<CMakeTool *> CMakeToolManager::cmakeTools()
{
    return d->m_cmakeTools;
}

void CMakeToolManager::setPreferNinja(bool set)
{
    d->m_preferNinja = set;
}

bool CMakeToolManager::preferNinja()
{
    return d->m_preferNinja;
}

Id CMakeToolManager::registerOrFindCMakeTool(const FileName &command)
{
    CMakeTool  *cmake = findByCommand(command);
    if (cmake)
        return cmake->id();

    cmake = new CMakeTool(CMakeTool::ManualDetection);
    cmake->setCMakeExecutable(command);
    cmake->setDisplayName(tr("CMake at %1").arg(command.toUserOutput()));

    addCMakeTool(cmake);
    emit m_instance->cmakeAdded(cmake->id());
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

    addCMakeTool(tool);
    emit m_instance->cmakeAdded(tool->id());
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

    QFileInfo systemSettingsFile(ICore::settings(QSettings::SystemScope)->fileName());
    FileName sdkSettingsFile = FileName::fromString(systemSettingsFile.absolutePath()
                                                    + QLatin1String(CMAKETOOL_FILENAME));

    QList<CMakeTool *> toolsToRegister = readCMakeTools(sdkSettingsFile, &defaultId, true);

    //read the tools from the user settings file
    QList<CMakeTool *> readTools = readCMakeTools(userSettingsFileName(), &defaultId, false);

    //autodetect tools
    QList<CMakeTool *> autoDetected = autoDetectCMakeTools();

    //filter out the tools that were stored in SDK
    for (int i = readTools.size() - 1; i >= 0; i--) {
        CMakeTool *currTool = readTools.takeAt(i);
        if (Utils::anyOf(toolsToRegister, Utils::equal(&CMakeTool::id, currTool->id()))) {
            delete currTool;
        } else {
            //if the current tool is marked as autodetected and NOT in the autodetected list,
            //it is a leftover SDK provided tool. The user will not be able to edit it,
            //so we automatically drop it
            if (currTool->isAutoDetected()) {
                if (!Utils::anyOf(autoDetected,
                                  Utils::equal(&CMakeTool::cmakeExecutable, currTool->cmakeExecutable()))) {

                    qWarning() << QString::fromLatin1("Previously SDK provided CMakeTool \"%1\" (%2) dropped.")
                                  .arg(currTool->cmakeExecutable().toUserOutput(), currTool->id().toString());

                    delete currTool;
                    continue;
                }
            }
            toolsToRegister.append(currTool);
        }
    }

    //filter out the tools that are already known
    while (autoDetected.size()) {
        CMakeTool *currTool = autoDetected.takeFirst();
        if (Utils::anyOf(toolsToRegister,
                         Utils::equal(&CMakeTool::cmakeExecutable, currTool->cmakeExecutable())))
            delete currTool;
        else
            toolsToRegister.append(currTool);
    }

    // Store all tools
    foreach (CMakeTool *current, toolsToRegister) {
        if (!registerCMakeTool(current)) {
            //this should never happen, but lets make sure we do not leak memory
            qWarning() << QString::fromLatin1("CMakeTool \"%1\" (%2) dropped.")
                          .arg(current->cmakeExecutable().toUserOutput(), current->id().toString());

            delete current;
        }
    }

    if (CMakeToolManager::findById(defaultId))
        d->m_defaultCMake = defaultId;

    // restore the legacy cmake settings only once and keep them around
    readAndDeleteLegacyCMakeSettings();
    emit m_instance->cmakeToolsLoaded();
}

void CMakeToolManager::registerAutodetectionHelper(CMakeToolManager::AutodetectionHelper helper)
{
    d->m_autoDetectionHelpers.append(helper);
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
    data.insert(QLatin1String(CMAKETOOL_PREFER_NINJA_KEY), d->m_preferNinja);

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
