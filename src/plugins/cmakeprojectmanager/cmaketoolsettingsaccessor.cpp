/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "cmaketoolsettingsaccessor.h"

#include "cmaketool.h"
#include "cmaketoolmanager.h"

#include <coreplugin/icore.h>

#include <app/app_version.h>
#include <utils/environment.h>

#include <utils/algorithm.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// CMakeToolSettingsUpgraders:
// --------------------------------------------------------------------

class CMakeToolSettingsUpgraderV0 : public VersionUpgrader
{
    // Necessary to make Version 1 supported.
public:
    CMakeToolSettingsUpgraderV0() : VersionUpgrader(0, "4.6") { }

    // NOOP
    QVariantMap upgrade(const QVariantMap &data) final { return data; }
};

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

const char CMAKE_TOOL_COUNT_KEY[] = "CMakeTools.Count";
const char CMAKE_TOOL_DATA_KEY[] = "CMakeTools.";
const char CMAKE_TOOL_DEFAULT_KEY[] = "CMakeTools.Default";
const char CMAKE_TOOL_FILENAME[] = "cmaketools.xml";

static std::vector<std::unique_ptr<CMakeTool>> autoDetectCMakeTools()
{
    Environment env = Environment::systemEnvironment();

    FilePaths path = env.path();
    path = Utils::filteredUnique(path);

    if (HostOsInfo::isWindowsHost()) {
        for (auto envVar : {"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"}) {
            if (qEnvironmentVariableIsSet(envVar)) {
                const QString progFiles = qEnvironmentVariable(envVar);
                path.append(FilePath::fromUserInput(progFiles + "/CMake"));
                path.append(FilePath::fromUserInput(progFiles + "/CMake/bin"));
            }
        }
    }

    if (HostOsInfo::isMacHost()) {
        path.append("/Applications/CMake.app/Contents/bin");
        path.append("/usr/local/bin");
        path.append("/opt/local/bin");
    }

    const QStringList execs = env.appendExeExtensions(QLatin1String("cmake"));

    FilePaths suspects;
    for (const FilePath &base : qAsConst(path)) {
        if (base.isEmpty())
            continue;

        QFileInfo fi;
        for (const QString &exec : execs) {
            fi.setFile(QDir(base.toString()), exec);
            if (fi.exists() && fi.isFile() && fi.isExecutable())
                suspects << FilePath::fromString(fi.absoluteFilePath());
        }
    }

    std::vector<std::unique_ptr<CMakeTool>> found;
    for (const FilePath &command : qAsConst(suspects)) {
        auto item = std::make_unique<CMakeTool>(CMakeTool::AutoDetection, CMakeTool::createId());
        item->setFilePath(command);
        item->setDisplayName(::CMakeProjectManager::CMakeToolManager::tr("System CMake at %1")
                                 .arg(command.toUserOutput()));

        found.emplace_back(std::move(item));
    }

    return found;
}


static std::vector<std::unique_ptr<CMakeTool>>
mergeTools(std::vector<std::unique_ptr<CMakeTool>> &sdkTools,
           std::vector<std::unique_ptr<CMakeTool>> &userTools,
           std::vector<std::unique_ptr<CMakeTool>> &autoDetectedTools)
{
    std::vector<std::unique_ptr<CMakeTool>> result = std::move(sdkTools);
    while (userTools.size() > 0) {
        std::unique_ptr<CMakeTool> userTool = std::move(userTools[0]);
        userTools.erase(std::begin(userTools));

        int userToolIndex = Utils::indexOf(result, [&userTool](const std::unique_ptr<CMakeTool> &tool) {
            // Id should be sufficient, but we have older "mis-registered" docker based items.
            // Make sure that these don't override better new values from the sdk by
            // also checking the actual executable.
            return userTool->id() == tool->id() && userTool->cmakeExecutable() == tool->cmakeExecutable();
        });
        if (userToolIndex >= 0) {
            // Replace the sdk tool with the user tool, so any user changes do not get lost
            result[userToolIndex] = std::move(userTool);
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
// CMakeToolSettingsAccessor:
// --------------------------------------------------------------------

CMakeToolSettingsAccessor::CMakeToolSettingsAccessor() :
    UpgradingSettingsAccessor("QtCreatorCMakeTools",
                              QCoreApplication::translate("CMakeProjectManager::CMakeToolManager", "CMake"),
                              Core::Constants::IDE_DISPLAY_NAME)
{
    setBaseFilePath(Core::ICore::userResourcePath(CMAKE_TOOL_FILENAME));

    addVersionUpgrader(std::make_unique<CMakeToolSettingsUpgraderV0>());
}

CMakeToolSettingsAccessor::CMakeTools CMakeToolSettingsAccessor::restoreCMakeTools(QWidget *parent) const
{
    CMakeTools result;

    const FilePath sdkSettingsFile = Core::ICore::installerResourcePath(CMAKE_TOOL_FILENAME);

    CMakeTools sdkTools = cmakeTools(restoreSettings(sdkSettingsFile, parent), true);

    //read the tools from the user settings file
    CMakeTools userTools = cmakeTools(restoreSettings(parent), false);

    //autodetect tools
    std::vector<std::unique_ptr<CMakeTool>> autoDetectedTools = autoDetectCMakeTools();

    //filter out the tools that were stored in SDK
    std::vector<std::unique_ptr<CMakeTool>> toRegister = mergeTools(sdkTools.cmakeTools,
                                                                    userTools.cmakeTools,
                                                                    autoDetectedTools);

    // Store all tools
    for (auto it = std::begin(toRegister); it != std::end(toRegister); ++it)
        result.cmakeTools.emplace_back(std::move(*it));

    result.defaultToolId = userTools.defaultToolId.isValid() ? userTools.defaultToolId : sdkTools.defaultToolId;

    // Set default TC...
    return result;
}

void CMakeToolSettingsAccessor::saveCMakeTools(const QList<CMakeTool *> &cmakeTools,
                                               const Id &defaultId,
                                               QWidget *parent)
{
    QVariantMap data;
    data.insert(QLatin1String(CMAKE_TOOL_DEFAULT_KEY), defaultId.toSetting());

    int count = 0;
    for (const CMakeTool *item : cmakeTools) {
        Utils::FilePath fi = item->cmakeExecutable();

        if (fi.needsDevice() || fi.isExecutableFile()) { // be graceful for device related stuff
            QVariantMap tmp = item->toMap();
            if (tmp.isEmpty())
                continue;
            data.insert(QString::fromLatin1(CMAKE_TOOL_DATA_KEY) + QString::number(count), tmp);
            ++count;
        }
    }
    data.insert(QLatin1String(CMAKE_TOOL_COUNT_KEY), count);

    saveSettings(data, parent);
}

CMakeToolSettingsAccessor::CMakeTools
CMakeToolSettingsAccessor::cmakeTools(const QVariantMap &data, bool fromSdk) const
{
    CMakeTools result;

    int count = data.value(QLatin1String(CMAKE_TOOL_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(CMAKE_TOOL_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            continue;

        const QVariantMap dbMap = data.value(key).toMap();
        auto item = std::make_unique<CMakeTool>(dbMap, fromSdk);
        const FilePath cmakeExecutable = item->cmakeExecutable();
        if (item->isAutoDetected() && !cmakeExecutable.needsDevice() && !cmakeExecutable.isExecutableFile()) {
            qWarning() << QString("CMakeTool \"%1\" (%2) dropped since the command is not executable.")
                          .arg(cmakeExecutable.toUserOutput(), item->id().toString());
            continue;
        }

        result.cmakeTools.emplace_back(std::move(item));
    }

    result.defaultToolId = Id::fromSetting(data.value(CMAKE_TOOL_DEFAULT_KEY, Id().toSetting()));

    return result;
}

} // namespace Internal
} // namespace CMakeProjectManager
