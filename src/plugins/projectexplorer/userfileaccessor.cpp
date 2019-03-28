/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "userfileaccessor.h"

#include "abi.h"
#include "devicesupport/devicemanager.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "toolchain.h"
#include "toolchainmanager.h"
#include "kit.h"
#include "kitmanager.h"

#include <app/app_version.h>
#include <coreplugin/icore.h>
#include <utils/persistentsettings.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace Utils;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {

const char OBSOLETE_VERSION_KEY[] = "ProjectExplorer.Project.Updater.FileVersion";
const char SHARED_SETTINGS[] = "SharedSettings";
const char USER_STICKY_KEYS_KEY[] = "UserStickyKeys";

// Version 14 Move builddir into BuildConfiguration
class UserFileVersion14Upgrader : public VersionUpgrader
{
public:
    UserFileVersion14Upgrader() : VersionUpgrader(14, "3.0-pre1") { }
    QVariantMap upgrade(const QVariantMap &map) final;
};

// Version 15 Use settingsaccessor based class for user file reading/writing
class UserFileVersion15Upgrader : public VersionUpgrader
{
public:
    UserFileVersion15Upgrader() : VersionUpgrader(15, "3.2-pre1") { }
    QVariantMap upgrade(const QVariantMap &map) final;
};

// Version 16 Changed android deployment
class UserFileVersion16Upgrader : public VersionUpgrader
{
public:
    UserFileVersion16Upgrader() : VersionUpgrader(16, "3.3-pre1") { }
    QVariantMap upgrade(const QVariantMap &data) final;
private:
    class OldStepMaps
    {
    public:
        QString defaultDisplayName;
        QString displayName;
        QVariantMap androidPackageInstall;
        QVariantMap androidDeployQt;
        bool isEmpty()
        {
            return androidPackageInstall.isEmpty() || androidDeployQt.isEmpty();
        }
    };


    QVariantMap removeAndroidPackageStep(QVariantMap deployMap);
    OldStepMaps extractStepMaps(const QVariantMap &deployMap);
    enum NamePolicy { KeepName, RenameBuildConfiguration };
    QVariantMap insertSteps(QVariantMap buildConfigurationMap,
                            const OldStepMaps &oldStepMap,
                            NamePolicy policy);
};

// Version 17 Apply user sticky keys per map
class UserFileVersion17Upgrader : public VersionUpgrader
{
public:
    UserFileVersion17Upgrader() : VersionUpgrader(17, "3.3-pre2") { }
    QVariantMap upgrade(const QVariantMap &map) final;

    QVariant process(const QVariant &entry);

private:
    QVariantList m_sticky;
};

// Version 18 renames "AutotoolsProjectManager.MakeStep.AdditionalArguments" to
// "AutotoolsProjectManager.MakeStep.MakeArguments" to account for
// sharing the MakeStep implementation
class UserFileVersion18Upgrader : public VersionUpgrader
{
public:
    UserFileVersion18Upgrader() : VersionUpgrader(18, "4.8-pre1") { }
    QVariantMap upgrade(const QVariantMap &map) final;

    static QVariant process(const QVariant &entry);
};

// Version 19 makes arguments, working directory and run-in-terminal
// run configuration fields use the same key in the settings file.
class UserFileVersion19Upgrader : public VersionUpgrader
{
public:
    UserFileVersion19Upgrader() : VersionUpgrader(19, "4.8-pre2") { }
    QVariantMap upgrade(const QVariantMap &map) final;

    static QVariant process(const QVariant &entry, const QStringList &path);
};

// Version 20 renames "Qbs.Deploy" to "ProjectExplorer.DefaultDeployConfiguration"
// to account for the merging of the respective factories
// run configuration fields use the same key in the settings file.
class UserFileVersion20Upgrader : public VersionUpgrader
{
public:
    UserFileVersion20Upgrader() : VersionUpgrader(20, "4.9-pre1") { }
    QVariantMap upgrade(const QVariantMap &map) final;

    static QVariant process(const QVariant &entry);
};

} // namespace

//
// Helper functions:
//

QT_BEGIN_NAMESPACE

class HandlerNode
{
public:
    QSet<QString> strings;
    QHash<QString, HandlerNode> children;
};

Q_DECLARE_TYPEINFO(HandlerNode, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

namespace {

static QString generateSuffix(const QString &suffix)
{
    QString result = suffix;
    result.replace(QRegExp("[^a-zA-Z0-9_.-]"), QString('_')); // replace fishy character
    if (!result.startsWith('.'))
        result.prepend('.');
    return result;
}

// Return path to shared directory for .user files, create if necessary.
static inline Utils::optional<QString> defineExternalUserFileDir()
{
    static const char userFilePathVariable[] = "QTC_USER_FILE_PATH";
    if (Q_LIKELY(!qEnvironmentVariableIsSet(userFilePathVariable)))
        return nullopt;
    const QFileInfo fi(QFile::decodeName(qgetenv(userFilePathVariable)));
    const QString path = fi.absoluteFilePath();
    if (fi.isDir() || fi.isSymLink())
        return path;
    if (fi.exists()) {
        qWarning() << userFilePathVariable << '=' << QDir::toNativeSeparators(path)
            << " points to an existing file";
        return nullopt;
    }
    QDir dir;
    if (!dir.mkpath(path)) {
        qWarning() << "Cannot create: " << QDir::toNativeSeparators(path);
        return nullopt;
    }
    return path;
}

// Return a suitable relative path to be created under the shared .user directory.
static QString makeRelative(QString path)
{
    const QChar slash('/');
    // Windows network shares: "//server.domain-a.com/foo' -> 'serverdomainacom/foo'
    if (path.startsWith("//")) {
        path.remove(0, 2);
        const int nextSlash = path.indexOf(slash);
        if (nextSlash > 0) {
            for (int p = nextSlash; p >= 0; --p) {
                if (!path.at(p).isLetterOrNumber())
                    path.remove(p, 1);
            }
        }
        return path;
    }
    // Windows drives: "C:/foo' -> 'c/foo'
    if (path.size() > 3 && path.at(1) == ':') {
        path.remove(1, 1);
        path[0] = path.at(0).toLower();
        return path;
    }
    if (path.startsWith(slash)) // Standard UNIX paths: '/foo' -> 'foo'
        path.remove(0, 1);
    return path;
}

// Return complete file path of the .user file.
static FileName externalUserFilePath(const Utils::FileName &projectFilePath, const QString &suffix)
{
    FileName result;
    static const optional<QString> externalUserFileDir = defineExternalUserFileDir();

    if (externalUserFileDir) {
        // Recreate the relative project file hierarchy under the shared directory.
        // PersistentSettingsWriter::write() takes care of creating the path.
        result = FileName::fromString(externalUserFileDir.value());
        result.appendString('/' + makeRelative(projectFilePath.toString()));
        result.appendString(suffix);
    }
    return result;
}

} // namespace

// --------------------------------------------------------------------
// UserFileBackupStrategy:
// --------------------------------------------------------------------

class UserFileBackUpStrategy : public Utils::VersionedBackUpStrategy
{
public:
    UserFileBackUpStrategy(UserFileAccessor *accessor) : Utils::VersionedBackUpStrategy(accessor)
    { }

    FileNameList readFileCandidates(const Utils::FileName &baseFileName) const final;
};

FileNameList UserFileBackUpStrategy::readFileCandidates(const FileName &baseFileName) const
{
    const auto *const ac = static_cast<const UserFileAccessor *>(accessor());
    const FileName externalUser = ac->externalUserFile();
    const FileName projectUser = ac->projectUserFile();
    QTC_CHECK(!baseFileName.isEmpty());
    QTC_CHECK(baseFileName == externalUser || baseFileName == projectUser);

    FileNameList result = Utils::VersionedBackUpStrategy::readFileCandidates(projectUser);
    if (!externalUser.isEmpty())
        result.append(Utils::VersionedBackUpStrategy::readFileCandidates(externalUser));

    return result;
}

// --------------------------------------------------------------------
// UserFileAccessor:
// --------------------------------------------------------------------

UserFileAccessor::UserFileAccessor(Project *project) :
    MergingSettingsAccessor(std::make_unique<VersionedBackUpStrategy>(this),
                            "QtCreatorProject", project->displayName(),
                            Core::Constants::IDE_DISPLAY_NAME),
    m_project(project)
{
    // Setup:
    const FileName externalUser = externalUserFile();
    const FileName projectUser = projectUserFile();
    setBaseFilePath(externalUser.isEmpty() ? projectUser : externalUser);

    auto secondary
            = std::make_unique<SettingsAccessor>(docType, displayName, applicationDisplayName);
    secondary->setBaseFilePath(sharedFile());
    secondary->setReadOnly();
    setSecondaryAccessor(std::move(secondary));

    setSettingsId(ProjectExplorerPlugin::projectExplorerSettings().environmentId.toByteArray());

    // Register Upgraders:
    addVersionUpgrader(std::make_unique<UserFileVersion14Upgrader>());
    addVersionUpgrader(std::make_unique<UserFileVersion15Upgrader>());
    addVersionUpgrader(std::make_unique<UserFileVersion16Upgrader>());
    addVersionUpgrader(std::make_unique<UserFileVersion17Upgrader>());
    addVersionUpgrader(std::make_unique<UserFileVersion18Upgrader>());
    addVersionUpgrader(std::make_unique<UserFileVersion19Upgrader>());
    addVersionUpgrader(std::make_unique<UserFileVersion20Upgrader>());
}

Project *UserFileAccessor::project() const
{
    return m_project;
}


SettingsMergeResult
UserFileAccessor::merge(const MergingSettingsAccessor::SettingsMergeData &global,
                        const MergingSettingsAccessor::SettingsMergeData &local) const
{
    const QStringList stickyKeys = global.main.value(USER_STICKY_KEYS_KEY).toStringList();

    const QString key = local.key;
    const QVariant mainValue = local.main.value(key);
    const QVariant secondaryValue = local.secondary.value(key);

    if (mainValue.isNull() && secondaryValue.isNull())
        return nullopt;

    if (isHouseKeepingKey(key) || global.key == USER_STICKY_KEYS_KEY)
        return qMakePair(key, mainValue);

    if (!stickyKeys.contains(global.key) && secondaryValue != mainValue && !secondaryValue.isNull())
        return qMakePair(key, secondaryValue);
    if (!mainValue.isNull())
        return qMakePair(key, mainValue);
    return qMakePair(key, secondaryValue);
}

// When saving settings...
//   If a .shared file was considered in the previous restoring step, we check whether for
//   any of the current .shared settings there's a .user one which is different. If so, this
//   means the user explicitly changed it and we mark this setting as sticky.
//   Note that settings are considered sticky only when they differ from the .shared ones.
//   Although this approach is more flexible than permanent/forever sticky settings, it has
//   the side-effect that if a particular value unintentionally becomes the same in both
//   the .user and .shared files, this setting will "unstick".
SettingsMergeFunction UserFileAccessor::userStickyTrackerFunction(QStringList &stickyKeys) const
{
    return [this, &stickyKeys](const SettingsMergeData &global, const SettingsMergeData &local)
           -> SettingsMergeResult {
        const QString key = local.key;
        const QVariant main = local.main.value(key);
        const QVariant secondary = local.secondary.value(key);

        if (main.isNull()) // skip stuff not in main!
            return nullopt;

        if (isHouseKeepingKey(key))
            return qMakePair(key, main);

        // Ignore house keeping keys:
        if (key == USER_STICKY_KEYS_KEY)
            return nullopt;

        // Track keys that changed in main from the value in secondary:
        if (main != secondary && !secondary.isNull() && !stickyKeys.contains(global.key))
            stickyKeys.append(global.key);
        return qMakePair(key, main);
    };
}

QVariant UserFileAccessor::retrieveSharedSettings() const
{
    return project()->property(SHARED_SETTINGS);
}

FileName UserFileAccessor::projectUserFile() const
{
    static const QString qtcExt = QLatin1String(qgetenv("QTC_EXTENSION"));
    FileName projectUserFile = m_project->projectFilePath();
    projectUserFile.appendString(generateSuffix(qtcExt.isEmpty() ? ".user" : qtcExt));
    return projectUserFile;
}

FileName UserFileAccessor::externalUserFile() const
{
    static const QString qtcExt = QFile::decodeName(qgetenv("QTC_EXTENSION"));
    return externalUserFilePath(m_project->projectFilePath(),
                                generateSuffix(qtcExt.isEmpty() ? ".user" : qtcExt));
}

FileName UserFileAccessor::sharedFile() const
{
    static const QString qtcExt = QLatin1String(qgetenv("QTC_SHARED_EXTENSION"));
    FileName sharedFile = m_project->projectFilePath();
    sharedFile.appendString(generateSuffix(qtcExt.isEmpty() ? ".shared" : qtcExt));
    return sharedFile;
}

QVariantMap UserFileAccessor::postprocessMerge(const QVariantMap &main,
                                               const QVariantMap &secondary,
                                               const QVariantMap &result) const
{
    project()->setProperty(SHARED_SETTINGS, secondary);
    return MergingSettingsAccessor::postprocessMerge(main, secondary, result);
}

QVariantMap UserFileAccessor::preprocessReadSettings(const QVariantMap &data) const
{
    QVariantMap tmp = MergingSettingsAccessor::preprocessReadSettings(data);

    // Move from old Version field to new one:
    // This cannot be done in a normal upgrader since the version information is needed
    // to decide which upgraders to run
    const QString obsoleteKey = OBSOLETE_VERSION_KEY;
    const int obsoleteVersion = tmp.value(obsoleteKey, -1).toInt();

    if (obsoleteVersion > versionFromMap(tmp))
        setVersionInMap(tmp, obsoleteVersion);

    tmp.remove(obsoleteKey);
    return tmp;
}

QVariantMap UserFileAccessor::prepareToWriteSettings(const QVariantMap &data) const
{
    const QVariantMap tmp = MergingSettingsAccessor::prepareToWriteSettings(data);
    const QVariantMap shared = retrieveSharedSettings().toMap();
    QVariantMap result;
    if (!shared.isEmpty()) {
        QStringList stickyKeys;
        SettingsMergeFunction merge = userStickyTrackerFunction(stickyKeys);
        result = mergeQVariantMaps(tmp, shared, merge).toMap();
        result.insert(USER_STICKY_KEYS_KEY, stickyKeys);
    } else {
        result = tmp;
    }

    // for compatibility with QtC 3.1 and older:
    result.insert(OBSOLETE_VERSION_KEY, currentVersion());
    return result;
}

// --------------------------------------------------------------------
// UserFileVersion14Upgrader:
// --------------------------------------------------------------------

QVariantMap UserFileVersion14Upgrader::upgrade(const QVariantMap &map)
{
    QVariantMap result;
    QMapIterator<QString, QVariant> it(map);
    while (it.hasNext()) {
        it.next();
        if (it.value().type() == QVariant::Map)
            result.insert(it.key(), upgrade(it.value().toMap()));
        else if (it.key() == "AutotoolsProjectManager.AutotoolsBuildConfiguration.BuildDirectory"
                 || it.key() == "CMakeProjectManager.CMakeBuildConfiguration.BuildDirectory"
                 || it.key() == "GenericProjectManager.GenericBuildConfiguration.BuildDirectory"
                 || it.key() == "Qbs.BuildDirectory"
                 || it.key() == "Qt4ProjectManager.Qt4BuildConfiguration.BuildDirectory")
            result.insert("ProjectExplorer.BuildConfiguration.BuildDirectory", it.value());
        else
            result.insert(it.key(), it.value());
    }
    return result;
}

// --------------------------------------------------------------------
// UserFileVersion15Upgrader:
// --------------------------------------------------------------------

QVariantMap UserFileVersion15Upgrader::upgrade(const QVariantMap &map)
{
    QList<Change> changes;
    changes.append(qMakePair(QLatin1String("ProjectExplorer.Project.Updater.EnvironmentId"),
                             QLatin1String("EnvironmentId")));
    // This is actually handled in the SettingsAccessor itself:
    // changes.append(qMakePair(QLatin1String("ProjectExplorer.Project.Updater.FileVersion"),
    //                          QLatin1String("Version")));
    changes.append(qMakePair(QLatin1String("ProjectExplorer.Project.UserStickyKeys"),
                             QLatin1String("UserStickyKeys")));

    return renameKeys(changes, QVariantMap(map));
}

// --------------------------------------------------------------------
// UserFileVersion16Upgrader:
// --------------------------------------------------------------------

UserFileVersion16Upgrader::OldStepMaps UserFileVersion16Upgrader::extractStepMaps(const QVariantMap &deployMap)
{
    OldStepMaps result;
    result.defaultDisplayName = deployMap.value("ProjectExplorer.ProjectConfiguration.DefaultDisplayName").toString();
    result.displayName = deployMap.value("ProjectExplorer.ProjectConfiguration.DisplayName").toString();
    const QString stepListKey = "ProjectExplorer.BuildConfiguration.BuildStepList.0";
    QVariantMap stepListMap = deployMap.value(stepListKey).toMap();
    int stepCount = stepListMap.value("ProjectExplorer.BuildStepList.StepsCount", 0).toInt();
    QString stepKey = "ProjectExplorer.BuildStepList.Step.";
    for (int i = 0; i < stepCount; ++i) {
        QVariantMap stepMap = stepListMap.value(stepKey + QString::number(i)).toMap();
        const QString id = stepMap.value("ProjectExplorer.ProjectConfiguration.Id").toString();
        if (id == "Qt4ProjectManager.AndroidDeployQtStep")
            result.androidDeployQt = stepMap;
        else if (id == "Qt4ProjectManager.AndroidPackageInstallationStep")
            result.androidPackageInstall = stepMap;
        if (!result.isEmpty())
            return result;

    }
    return result;
}

QVariantMap UserFileVersion16Upgrader::removeAndroidPackageStep(QVariantMap deployMap)
{
    const QString stepListKey = "ProjectExplorer.BuildConfiguration.BuildStepList.0";
    QVariantMap stepListMap = deployMap.value(stepListKey).toMap();
    const QString stepCountKey = "ProjectExplorer.BuildStepList.StepsCount";
    int stepCount = stepListMap.value(stepCountKey, 0).toInt();
    QString stepKey = "ProjectExplorer.BuildStepList.Step.";
    int targetPosition = 0;
    for (int sourcePosition = 0; sourcePosition < stepCount; ++sourcePosition) {
        QVariantMap stepMap = stepListMap.value(stepKey + QString::number(sourcePosition)).toMap();
        if (stepMap.value("ProjectExplorer.ProjectConfiguration.Id").toString()
                != "Qt4ProjectManager.AndroidPackageInstallationStep") {
            stepListMap.insert(stepKey + QString::number(targetPosition), stepMap);
            ++targetPosition;
        }
    }

    stepListMap.insert(stepCountKey, targetPosition);

    for (int i = targetPosition; i < stepCount; ++i)
        stepListMap.remove(stepKey + QString::number(i));

    deployMap.insert(stepListKey, stepListMap);
    return deployMap;
}

QVariantMap UserFileVersion16Upgrader::insertSteps(QVariantMap buildConfigurationMap,
                                                   const OldStepMaps &oldStepMap,
                                                   NamePolicy policy)
{
    const QString bslCountKey = "ProjectExplorer.BuildConfiguration.BuildStepListCount";
    int stepListCount = buildConfigurationMap.value(bslCountKey).toInt();

    const QString bslKey = "ProjectExplorer.BuildConfiguration.BuildStepList.";
    const QString bslTypeKey = "ProjectExplorer.ProjectConfiguration.Id";
    for (int bslNumber = 0; bslNumber < stepListCount; ++bslNumber) {
        QVariantMap buildStepListMap = buildConfigurationMap.value(bslKey + QString::number(bslNumber)).toMap();
        if (buildStepListMap.value(bslTypeKey) != "ProjectExplorer.BuildSteps.Build")
            continue;

        const QString bslStepCountKey = "ProjectExplorer.BuildStepList.StepsCount";

        int stepCount = buildStepListMap.value(bslStepCountKey).toInt();
        buildStepListMap.insert(bslStepCountKey, stepCount + 2);

        QVariantMap androidPackageInstallStep;
        QVariantMap androidBuildApkStep;

        // common settings of all buildsteps
        const QString enabledKey = "ProjectExplorer.BuildStep.Enabled";
        const QString idKey = "ProjectExplorer.ProjectConfiguration.Id";
        const QString displayNameKey = "ProjectExplorer.ProjectConfiguration.DisplayName";
        const QString defaultDisplayNameKey = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName";

        QString displayName = oldStepMap.androidPackageInstall.value(displayNameKey).toString();
        QString defaultDisplayName = oldStepMap.androidPackageInstall.value(defaultDisplayNameKey).toString();
        bool enabled = oldStepMap.androidPackageInstall.value(enabledKey).toBool();

        androidPackageInstallStep.insert(idKey, Core::Id("Qt4ProjectManager.AndroidPackageInstallationStep").toSetting());
        androidPackageInstallStep.insert(displayNameKey, displayName);
        androidPackageInstallStep.insert(defaultDisplayNameKey, defaultDisplayName);
        androidPackageInstallStep.insert(enabledKey, enabled);

        displayName = oldStepMap.androidDeployQt.value(displayName).toString();
        defaultDisplayName = oldStepMap.androidDeployQt.value(defaultDisplayNameKey).toString();
        enabled = oldStepMap.androidDeployQt.value(enabledKey).toBool();

        androidBuildApkStep.insert(idKey, Core::Id("QmakeProjectManager.AndroidBuildApkStep").toSetting());
        androidBuildApkStep.insert(displayNameKey, displayName);
        androidBuildApkStep.insert(defaultDisplayNameKey, defaultDisplayName);
        androidBuildApkStep.insert(enabledKey, enabled);

        // settings transferred from AndroidDeployQtStep to QmakeBuildApkStep
        const QString ProFilePathForInputFile = "ProFilePathForInputFile";
        const QString DeployActionKey = "Qt4ProjectManager.AndroidDeployQtStep.DeployQtAction";
        const QString KeystoreLocationKey = "KeystoreLocation";
        const QString BuildTargetSdkKey = "BuildTargetSdk";
        const QString VerboseOutputKey = "VerboseOutput";

        QString inputFile = oldStepMap.androidDeployQt.value(ProFilePathForInputFile).toString();
        int oldDeployAction = oldStepMap.androidDeployQt.value(DeployActionKey).toInt();
        QString keyStorePath = oldStepMap.androidDeployQt.value(KeystoreLocationKey).toString();
        QString buildTargetSdk = oldStepMap.androidDeployQt.value(BuildTargetSdkKey).toString();
        bool verbose = oldStepMap.androidDeployQt.value(VerboseOutputKey).toBool();
        androidBuildApkStep.insert(ProFilePathForInputFile, inputFile);
        androidBuildApkStep.insert(DeployActionKey, oldDeployAction);
        androidBuildApkStep.insert(KeystoreLocationKey, keyStorePath);
        androidBuildApkStep.insert(BuildTargetSdkKey, buildTargetSdk);
        androidBuildApkStep.insert(VerboseOutputKey, verbose);

        const QString buildStepKey = "ProjectExplorer.BuildStepList.Step.";
        buildStepListMap.insert(buildStepKey + QString::number(stepCount), androidPackageInstallStep);
        buildStepListMap.insert(buildStepKey + QString::number(stepCount + 1), androidBuildApkStep);

        buildConfigurationMap.insert(bslKey + QString::number(bslNumber), buildStepListMap);
    }

    if (policy == RenameBuildConfiguration) {
        const QString displayNameKey = "ProjectExplorer.ProjectConfiguration.DisplayName";
        const QString defaultDisplayNameKey = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName";

        QString defaultDisplayName = buildConfigurationMap.value(defaultDisplayNameKey).toString();
        QString displayName = buildConfigurationMap.value(displayNameKey).toString();
        if (displayName.isEmpty())
            displayName = defaultDisplayName;
        QString oldDisplayname = oldStepMap.displayName;
        if (oldDisplayname.isEmpty())
            oldDisplayname = oldStepMap.defaultDisplayName;

        displayName.append(" - ");
        displayName.append(oldDisplayname);
        buildConfigurationMap.insert(displayNameKey, displayName);

        defaultDisplayName.append(" - ");
        defaultDisplayName.append(oldStepMap.defaultDisplayName);
        buildConfigurationMap.insert(defaultDisplayNameKey, defaultDisplayName);
    }

    return buildConfigurationMap;
}

QVariantMap UserFileVersion16Upgrader::upgrade(const QVariantMap &data)
{
    int targetCount = data.value("ProjectExplorer.Project.TargetCount", 0).toInt();
    if (!targetCount)
        return data;

    QVariantMap result = data;

    for (int i = 0; i < targetCount; ++i) {
        QString targetKey = QLatin1String("ProjectExplorer.Project.Target.") + QString::number(i);
        QVariantMap targetMap = data.value(targetKey).toMap();

        const QString dcCountKey = "ProjectExplorer.Target.DeployConfigurationCount";
        int deployconfigurationCount = targetMap.value(dcCountKey).toInt();
        if (!deployconfigurationCount) // should never happen
            continue;

        QList<OldStepMaps> oldSteps;
        QList<QVariantMap> oldBuildConfigurations;

        QString deployKey = "ProjectExplorer.Target.DeployConfiguration.";
        for (int j = 0; j < deployconfigurationCount; ++j) {
            QVariantMap deployConfigurationMap
                    = targetMap.value(deployKey + QString::number(j)).toMap();
            OldStepMaps oldStep = extractStepMaps(deployConfigurationMap);
            if (!oldStep.isEmpty()) {
                oldSteps.append(oldStep);
                deployConfigurationMap = removeAndroidPackageStep(deployConfigurationMap);
                targetMap.insert(deployKey + QString::number(j), deployConfigurationMap);
            }
        }

        if (oldSteps.isEmpty()) // no android target?
            continue;

        const QString bcCountKey = "ProjectExplorer.Target.BuildConfigurationCount";
        int buildConfigurationCount
                = targetMap.value(bcCountKey).toInt();

        if (!buildConfigurationCount) // should never happen
            continue;

        QString bcKey = "ProjectExplorer.Target.BuildConfiguration.";
        for (int j = 0; j < buildConfigurationCount; ++j) {
            QVariantMap oldBuildConfigurationMap = targetMap.value(bcKey + QString::number(j)).toMap();
            oldBuildConfigurations.append(oldBuildConfigurationMap);
        }

        QList<QVariantMap> newBuildConfigurations;

        NamePolicy policy = oldSteps.size() > 1 ? RenameBuildConfiguration : KeepName;

        foreach (const QVariantMap &oldBuildConfiguration, oldBuildConfigurations) {
            foreach (const OldStepMaps &oldStep, oldSteps) {
                QVariantMap newBuildConfiguration = insertSteps(oldBuildConfiguration, oldStep, policy);
                if (!newBuildConfiguration.isEmpty())
                    newBuildConfigurations.append(newBuildConfiguration);
            }
        }

        targetMap.insert(bcCountKey, newBuildConfigurations.size());

        for (int j = 0; j < newBuildConfigurations.size(); ++j)
            targetMap.insert(bcKey + QString::number(j), newBuildConfigurations.at(j));
        result.insert(targetKey, targetMap);
    }

    return result;
}

QVariantMap UserFileVersion17Upgrader::upgrade(const QVariantMap &map)
{
    m_sticky = map.value(USER_STICKY_KEYS_KEY).toList();
    if (m_sticky.isEmpty())
        return map;
    return process(map).toMap();
}

QVariant UserFileVersion17Upgrader::process(const QVariant &entry)
{
    switch (entry.type()) {
    case QVariant::List: {
        QVariantList result;
        foreach (const QVariant &item, entry.toList())
            result.append(process(item));
        return result;
    }
    case QVariant::Map: {
        QVariantMap result = entry.toMap();
        for (QVariantMap::iterator i = result.begin(), end = result.end(); i != end; ++i) {
            QVariant &v = i.value();
            v = process(v);
        }
        result.insert(USER_STICKY_KEYS_KEY, m_sticky);
        return result;
    }
    default:
        return entry;
    }
}

QVariantMap UserFileVersion18Upgrader::upgrade(const QVariantMap &map)
{
    return process(map).toMap();
}

QVariant UserFileVersion18Upgrader::process(const QVariant &entry)
{
    switch (entry.type()) {
    case QVariant::List:
        return Utils::transform(entry.toList(), &UserFileVersion18Upgrader::process);
    case QVariant::Map:
        return Utils::transform<QMap<QString, QVariant>>(
            entry.toMap().toStdMap(), [](const std::pair<const QString, QVariant> &item) {
                const QString key = (item.first
                                             == "AutotoolsProjectManager.MakeStep.AdditionalArguments"
                                         ? QString("AutotoolsProjectManager.MakeStep.MakeArguments")
                                         : item.first);
                return qMakePair(key, UserFileVersion18Upgrader::process(item.second));
            });
    default:
        return entry;
    }
}

QVariantMap UserFileVersion19Upgrader::upgrade(const QVariantMap &map)
{
    return process(map, QStringList()).toMap();
}

QVariant UserFileVersion19Upgrader::process(const QVariant &entry, const QStringList &path)
{
    static const QStringList argsKeys = {"Qt4ProjectManager.MaemoRunConfiguration.Arguments",
                                         "CMakeProjectManager.CMakeRunConfiguration.Arguments",
                                         "Ios.run_arguments",
                                         "Nim.NimRunConfiguration.ArgumentAspect",
                                         "ProjectExplorer.CustomExecutableRunConfiguration.Arguments",
                                         "PythonEditor.RunConfiguration.Arguments",
                                         "Qbs.RunConfiguration.CommandLineArguments",
                                         "Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments",
                                         "RemoteLinux.CustomRunConfig.Arguments",
                                         "WinRtRunConfigurationArgumentsId",
                                         "CommandLineArgs"};
    static const QStringList wdKeys = {"BareMetal.RunConfig.WorkingDirectory",
                                       "CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory",
                                       "Nim.NimRunConfiguration.WorkingDirectoryAspect",
                                       "ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory",
                                       "Qbs.RunConfiguration.WorkingDirectory",
                                       "Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory",
                                       "RemoteLinux.CustomRunConfig.WorkingDirectory"
                                       "RemoteLinux.RunConfig.WorkingDirectory",
                                       "WorkingDir"};
    static const QStringList termKeys = {"CMakeProjectManager.CMakeRunConfiguration.UseTerminal",
                                         "Nim.NimRunConfiguration.TerminalAspect",
                                         "ProjectExplorer.CustomExecutableRunConfiguration.UseTerminal",
                                         "PythonEditor.RunConfiguration.UseTerminal",
                                         "Qbs.RunConfiguration.UseTerminal",
                                         "Qt4ProjectManager.Qt4RunConfiguration.UseTerminal"};
    static const QStringList libsKeys = {"Qbs.RunConfiguration.UsingLibraryPaths",
                                         "QmakeProjectManager.QmakeRunConfiguration.UseLibrarySearchPath"};
    static const QStringList dyldKeys = {"Qbs.RunConfiguration.UseDyldImageSuffix",
                                         "QmakeProjectManager.QmakeRunConfiguration.UseDyldImageSuffix"};
    switch (entry.type()) {
    case QVariant::List:
        return Utils::transform(entry.toList(),
                                std::bind(&UserFileVersion19Upgrader::process, std::placeholders::_1, path));
    case QVariant::Map:
        return Utils::transform<QVariantMap>(
            entry.toMap().toStdMap(), [&](const std::pair<const QString, QVariant> &item) {
                if (path.size() == 2 && path.at(1).startsWith("ProjectExplorer.Target.RunConfiguration.")) {
                    if (argsKeys.contains(item.first))
                        return qMakePair(QString("RunConfiguration.Arguments"), item.second);
                    if (wdKeys.contains(item.first))
                        return qMakePair(QString("RunConfiguration.WorkingDirectory"), item.second);
                    if (termKeys.contains(item.first))
                        return qMakePair(QString("RunConfiguration.UseTerminal"), item.second);
                    if (libsKeys.contains(item.first))
                        return qMakePair(QString("RunConfiguration.UseLibrarySearchPath"), item.second);
                    if (dyldKeys.contains(item.first))
                        return qMakePair(QString("RunConfiguration.UseDyldImageSuffix"), item.second);
                }
                QStringList newPath = path;
                newPath.append(item.first);
                return qMakePair(item.first, UserFileVersion19Upgrader::process(item.second, newPath));
            });
    default:
        return entry;
    }
}

QVariantMap UserFileVersion20Upgrader::upgrade(const QVariantMap &map)
{
    return process(map).toMap();
}

QVariant UserFileVersion20Upgrader::process(const QVariant &entry)
{
    switch (entry.type()) {
    case QVariant::List:
        return Utils::transform(entry.toList(), &UserFileVersion20Upgrader::process);
    case QVariant::Map:
        return Utils::transform<QMap<QString, QVariant>>(
            entry.toMap().toStdMap(), [](const std::pair<const QString, QVariant> &item) {
                auto res = qMakePair(item.first, item.second);
                if (item.first == "ProjectExplorer.ProjectConfiguration.Id"
                        && item.second == "Qbs.Deploy")
                    res.second = QVariant("ProjectExplorer.DefaultDeployConfiguration");
                else
                    res.second = UserFileVersion20Upgrader::process(item.second);
                return res;
            });
    default:
        return entry;
    }
}

#if defined(WITH_TESTS)

#include <QTest>

#include "projectexplorer.h"

namespace {

class TestUserFileAccessor : public UserFileAccessor
{
public:
    TestUserFileAccessor(Project *project) : UserFileAccessor(project) { }

    void storeSharedSettings(const QVariantMap &data) const { m_storedSettings = data; }
    QVariant retrieveSharedSettings() const override { return m_storedSettings; }

    using UserFileAccessor::preprocessReadSettings;
    using UserFileAccessor::prepareToWriteSettings;

    using UserFileAccessor::mergeSettings;

private:
    mutable QVariantMap m_storedSettings;
};


class TestProject : public Project
{
public:
    TestProject() : Project("x-test/testproject", Utils::FileName::fromString("/test/project")) {
        setDisplayName("Test Project");
    }

    bool needsConfiguration() const final { return false; }
};

} // namespace

void ProjectExplorerPlugin::testUserFileAccessor_prepareToReadSettings()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    QVariantMap data;
    data.insert("Version", 4);
    data.insert("Foo", "bar");

    QVariantMap result = accessor.preprocessReadSettings(data);

    QCOMPARE(result, data);
}

void ProjectExplorerPlugin::testUserFileAccessor_prepareToReadSettingsObsoleteVersion()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    QVariantMap data;
    data.insert("ProjectExplorer.Project.Updater.FileVersion", 4);
    data.insert("Foo", "bar");

    QVariantMap result = accessor.preprocessReadSettings(data);

    QCOMPARE(result.count(), data.count());
    QCOMPARE(result.value("Foo"), data.value("Foo"));
    QCOMPARE(result.value("Version"), data.value("ProjectExplorer.Project.Updater.FileVersion"));
}

void ProjectExplorerPlugin::testUserFileAccessor_prepareToReadSettingsObsoleteVersionNewVersion()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    QVariantMap data;
    data.insert("ProjectExplorer.Project.Updater.FileVersion", 4);
    data.insert("Version", 5);
    data.insert("Foo", "bar");

    QVariantMap result = accessor.preprocessReadSettings(data);

    QCOMPARE(result.count(), data.count() - 1);
    QCOMPARE(result.value("Foo"), data.value("Foo"));
    QCOMPARE(result.value("Version"), data.value("Version"));
}

void ProjectExplorerPlugin::testUserFileAccessor_prepareToWriteSettings()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    QVariantMap sharedData;
    sharedData.insert("Version", 10);
    sharedData.insert("shared1", "bar");
    sharedData.insert("shared2", "baz");
    sharedData.insert("shared3", "foo");

    accessor.storeSharedSettings(sharedData);

    QVariantMap data;
    data.insert("Version", 10);
    data.insert("shared1", "bar1");
    data.insert("unique1", 1234);
    data.insert("shared3", "foo");
    QVariantMap result = accessor.prepareToWriteSettings(data);

    QCOMPARE(result.count(), data.count() + 3);
    QCOMPARE(result.value("EnvironmentId").toByteArray(),
             projectExplorerSettings().environmentId.toByteArray());
    QCOMPARE(result.value("UserStickyKeys"), QVariant(QStringList({"shared1"})));
    QCOMPARE(result.value("Version").toInt(), accessor.currentVersion());
    QCOMPARE(result.value("ProjectExplorer.Project.Updater.FileVersion").toInt(), accessor.currentVersion());
    QCOMPARE(result.value("shared1"), data.value("shared1"));
    QCOMPARE(result.value("shared3"), data.value("shared3"));
    QCOMPARE(result.value("unique1"), data.value("unique1"));
}

void ProjectExplorerPlugin::testUserFileAccessor_mergeSettings()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    QVariantMap sharedData;
    sharedData.insert("Version", accessor.currentVersion());
    sharedData.insert("shared1", "bar");
    sharedData.insert("shared2", "baz");
    sharedData.insert("shared3", "foooo");
    TestUserFileAccessor::RestoreData shared(FileName::fromString("/shared/data"), sharedData);

    QVariantMap data;
    data.insert("Version", accessor.currentVersion());
    data.insert("EnvironmentId", projectExplorerSettings().environmentId.toByteArray());
    data.insert("UserStickyKeys", QStringList({"shared1"}));
    data.insert("shared1", "bar1");
    data.insert("unique1", 1234);
    data.insert("shared3", "foo");
    TestUserFileAccessor::RestoreData user(FileName::fromString("/user/data"), data);
    TestUserFileAccessor::RestoreData result = accessor.mergeSettings(user, shared);

    QVERIFY(!result.hasIssue());
    QCOMPARE(result.data.count(), data.count() + 1);
    // mergeSettings does not run updateSettings, so no OriginalVersion will be set
    QCOMPARE(result.data.value("EnvironmentId").toByteArray(),
             projectExplorerSettings().environmentId.toByteArray()); // unchanged
    QCOMPARE(result.data.value("UserStickyKeys"), QVariant(QStringList({"shared1"}))); // unchanged
    QCOMPARE(result.data.value("Version").toInt(), accessor.currentVersion()); // forced
    QCOMPARE(result.data.value("shared1"), data.value("shared1")); // from data
    QCOMPARE(result.data.value("shared2"), sharedData.value("shared2")); // from shared, missing!
    QCOMPARE(result.data.value("shared3"), sharedData.value("shared3")); // from shared
    QCOMPARE(result.data.value("unique1"), data.value("unique1"));
}

void ProjectExplorerPlugin::testUserFileAccessor_mergeSettingsEmptyUser()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    QVariantMap sharedData;
    sharedData.insert("Version", accessor.currentVersion());
    sharedData.insert("shared1", "bar");
    sharedData.insert("shared2", "baz");
    sharedData.insert("shared3", "foooo");
    TestUserFileAccessor::RestoreData shared(FileName::fromString("/shared/data"), sharedData);

    QVariantMap data;
    TestUserFileAccessor::RestoreData user(FileName::fromString("/shared/data"), data);

    TestUserFileAccessor::RestoreData result = accessor.mergeSettings(user, shared);

    QVERIFY(!result.hasIssue());
    QCOMPARE(result.data, sharedData);
}

void ProjectExplorerPlugin::testUserFileAccessor_mergeSettingsEmptyShared()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    QVariantMap sharedData;
    TestUserFileAccessor::RestoreData shared(FileName::fromString("/shared/data"), sharedData);

    QVariantMap data;
    data.insert("Version", accessor.currentVersion());
    data.insert("OriginalVersion", accessor.currentVersion());
    data.insert("EnvironmentId", projectExplorerSettings().environmentId.toByteArray());
    data.insert("UserStickyKeys", QStringList({"shared1"}));
    data.insert("shared1", "bar1");
    data.insert("unique1", 1234);
    data.insert("shared3", "foo");
    TestUserFileAccessor::RestoreData user(FileName::fromString("/shared/data"), data);

    TestUserFileAccessor::RestoreData result = accessor.mergeSettings(user, shared);

    QVERIFY(!result.hasIssue());
    QCOMPARE(result.data, data);
}

#endif // WITH_TESTS
