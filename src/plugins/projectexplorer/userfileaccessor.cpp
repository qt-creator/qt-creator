// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <coreplugin/icore.h>

#include <utils/appinfo.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/persistentsettings.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QGuiApplication>
#include <QRegularExpression>

using namespace Utils;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

using KeyVariantPair = std::pair<const Key, QVariant>;

static QString userFileExtension()
{
    const QString ext = Utils::appInfo().userFileExtension;
    return ext.isEmpty() ? QLatin1String(".user") : ext;
}

namespace {

const char OBSOLETE_VERSION_KEY[] = "ProjectExplorer.Project.Updater.FileVersion";
const char SHARED_SETTINGS[] = "SharedSettings";
const char USER_STICKY_KEYS_KEY[] = "UserStickyKeys";

// Version 14 Move builddir into BuildConfiguration
class UserFileVersion14Upgrader : public VersionUpgrader
{
public:
    UserFileVersion14Upgrader() : VersionUpgrader(14, "3.0-pre1") { }
    Store upgrade(const Store &map) final;
};

// Version 15 Use settingsaccessor based class for user file reading/writing
class UserFileVersion15Upgrader : public VersionUpgrader
{
public:
    UserFileVersion15Upgrader() : VersionUpgrader(15, "3.2-pre1") { }
    Store upgrade(const Store &map) final;
};

// Version 16 Changed android deployment
class UserFileVersion16Upgrader : public VersionUpgrader
{
public:
    UserFileVersion16Upgrader() : VersionUpgrader(16, "3.3-pre1") { }
    Store upgrade(const Store &data) final;
private:
    class OldStepMaps
    {
    public:
        QString defaultDisplayName;
        QString displayName;
        Store androidPackageInstall;
        Store androidDeployQt;
        bool isEmpty()
        {
            return androidPackageInstall.isEmpty() || androidDeployQt.isEmpty();
        }
    };


    Store removeAndroidPackageStep(Store deployMap);
    OldStepMaps extractStepMaps(const Store &deployMap);
    enum NamePolicy { KeepName, RenameBuildConfiguration };
    Store insertSteps(Store buildConfigurationMap,
                            const OldStepMaps &oldStepMap,
                            NamePolicy policy);
};

// Version 17 Apply user sticky keys per map
class UserFileVersion17Upgrader : public VersionUpgrader
{
public:
    UserFileVersion17Upgrader() : VersionUpgrader(17, "3.3-pre2") { }
    Store upgrade(const Store &map) final;

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
    Store upgrade(const Store &map) final;

    static QVariant process(const QVariant &entry);
};

// Version 19 makes arguments, working directory and run-in-terminal
// run configuration fields use the same key in the settings file.
class UserFileVersion19Upgrader : public VersionUpgrader
{
public:
    UserFileVersion19Upgrader() : VersionUpgrader(19, "4.8-pre2") { }
    Store upgrade(const Store &map) final;

    static QVariant process(const QVariant &entry, const KeyList &path);
};

// Version 20 renames "Qbs.Deploy" to "ProjectExplorer.DefaultDeployConfiguration"
// to account for the merging of the respective factories
// run configuration fields use the same key in the settings file.
class UserFileVersion20Upgrader : public VersionUpgrader
{
public:
    UserFileVersion20Upgrader() : VersionUpgrader(20, "4.9-pre1") { }
    Store upgrade(const Store &map) final;

    static QVariant process(const QVariant &entry);
};

// Version 21 adds a "make install" step to an existing RemoteLinux deploy configuration
// if and only if such a step would be added when creating a new one.
// See QTCREATORBUG-22689.
class UserFileVersion21Upgrader : public VersionUpgrader
{
public:
    UserFileVersion21Upgrader() : VersionUpgrader(21, "4.10-pre1") { }
    Store upgrade(const Store &map) final;

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
    result.replace(QRegularExpression("[^a-zA-Z0-9_.-]"), QString('_')); // replace fishy character
    if (!result.startsWith('.'))
        result.prepend('.');
    return result;
}

// Return path to shared directory for .user files, create if necessary.
static inline std::optional<QString> defineExternalUserFileDir()
{
    const char userFilePathVariable[] = "QTC_USER_FILE_PATH";
    if (Q_LIKELY(!qtcEnvironmentVariableIsSet(userFilePathVariable)))
        return std::nullopt;
    const QFileInfo fi(qtcEnvironmentVariable(userFilePathVariable));
    const QString path = fi.absoluteFilePath();
    if (fi.isDir() || fi.isSymLink())
        return path;
    if (fi.exists()) {
        qWarning() << userFilePathVariable << '=' << QDir::toNativeSeparators(path)
            << " points to an existing file";
        return std::nullopt;
    }
    QDir dir;
    if (!dir.mkpath(path)) {
        qWarning() << "Cannot create: " << QDir::toNativeSeparators(path);
        return std::nullopt;
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
static FilePath externalUserFilePath(const Utils::FilePath &projectFilePath, const QString &suffix)
{
    static const std::optional<QString> externalUserFileDir = defineExternalUserFileDir();

    if (externalUserFileDir) {
        // Recreate the relative project file hierarchy under the shared directory.
        // PersistentSettingsWriter::write() takes care of creating the path.
        return FilePath::fromString(externalUserFileDir.value()
                                    + '/' + makeRelative(projectFilePath.toString())
                                    + suffix);
    }
    return {};
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

    FilePaths readFileCandidates(const Utils::FilePath &baseFileName) const final;
};

FilePaths UserFileBackUpStrategy::readFileCandidates(const FilePath &baseFileName) const
{
    const auto *const ac = static_cast<const UserFileAccessor *>(accessor());
    const FilePath externalUser = ac->externalUserFile();
    const FilePath projectUser = ac->projectUserFile();
    QTC_CHECK(!baseFileName.isEmpty());
    QTC_CHECK(baseFileName == externalUser || baseFileName == projectUser);

    FilePaths result = Utils::VersionedBackUpStrategy::readFileCandidates(projectUser);
    if (!externalUser.isEmpty())
        result.append(Utils::VersionedBackUpStrategy::readFileCandidates(externalUser));

    return result;
}

// --------------------------------------------------------------------
// UserFileAccessor:
// --------------------------------------------------------------------

UserFileAccessor::UserFileAccessor(Project *project)
    : m_project(project)
{
    setStrategy(std::make_unique<VersionedBackUpStrategy>(this));
    setDocType("QtCreatorProject");
    setApplicationDisplayName(QGuiApplication::applicationDisplayName());

    // Setup:
    const FilePath externalUser = externalUserFile();
    const FilePath projectUser = projectUserFile();
    setBaseFilePath(externalUser.isEmpty() ? projectUser : externalUser);

    auto secondary = std::make_unique<SettingsAccessor>();
    secondary->setDocType(m_docType);
    secondary->setApplicationDisplayName(m_applicationDisplayName);
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
    addVersionUpgrader(std::make_unique<UserFileVersion21Upgrader>());
}

Project *UserFileAccessor::project() const
{
    return m_project;
}


SettingsMergeResult
UserFileAccessor::merge(const MergingSettingsAccessor::SettingsMergeData &global,
                        const MergingSettingsAccessor::SettingsMergeData &local) const
{
    const KeyList stickyKeys = keysFromStrings(global.main.value(USER_STICKY_KEYS_KEY).toStringList());

    const Key key = local.key;
    const QVariant mainValue = local.main.value(key);
    const QVariant secondaryValue = local.secondary.value(key);

    if (mainValue.isNull() && secondaryValue.isNull())
        return std::nullopt;

    if (isHouseKeepingKey(key) || global.key == USER_STICKY_KEYS_KEY)
        return {{key, mainValue}};

    if (!stickyKeys.contains(global.key) && secondaryValue != mainValue && !secondaryValue.isNull())
        return {{key, secondaryValue}};
    if (!mainValue.isNull())
        return {{key, mainValue}};
    return {{key, secondaryValue}};
}

// When saving settings...
//   If a .shared file was considered in the previous restoring step, we check whether for
//   any of the current .shared settings there's a .user one which is different. If so, this
//   means the user explicitly changed it and we mark this setting as sticky.
//   Note that settings are considered sticky only when they differ from the .shared ones.
//   Although this approach is more flexible than permanent/forever sticky settings, it has
//   the side-effect that if a particular value unintentionally becomes the same in both
//   the .user and .shared files, this setting will "unstick".
SettingsMergeFunction UserFileAccessor::userStickyTrackerFunction(KeyList &stickyKeys) const
{
    return [&stickyKeys](const SettingsMergeData &global, const SettingsMergeData &local)
           -> SettingsMergeResult {
        const Key key = local.key;
        const QVariant main = local.main.value(key);
        const QVariant secondary = local.secondary.value(key);

        if (main.isNull()) // skip stuff not in main!
            return std::nullopt;

        if (isHouseKeepingKey(key))
            return {{key, main}};

        // Ignore house keeping keys:
        if (key == USER_STICKY_KEYS_KEY)
            return std::nullopt;

        // Track keys that changed in main from the value in secondary:
        if (main != secondary && !secondary.isNull() && !stickyKeys.contains(global.key))
            stickyKeys.append(global.key);
        return {{key, main}};
    };
}

QVariant UserFileAccessor::retrieveSharedSettings() const
{
    return project()->property(SHARED_SETTINGS);
}

FilePath UserFileAccessor::projectUserFile() const
{
    static const QString qtcExt = qtcEnvironmentVariable("QTC_EXTENSION");
    return m_project->projectFilePath().stringAppended(
        generateSuffix(qtcExt.isEmpty() ? userFileExtension() : qtcExt));
}

FilePath UserFileAccessor::externalUserFile() const
{
    static const QString qtcExt = qtcEnvironmentVariable("QTC_EXTENSION");
    return externalUserFilePath(m_project->projectFilePath(),
                                generateSuffix(qtcExt.isEmpty() ? userFileExtension() : qtcExt));
}

FilePath UserFileAccessor::sharedFile() const
{
    static const QString qtcExt = qtcEnvironmentVariable("QTC_SHARED_EXTENSION");
    return m_project->projectFilePath()
            .stringAppended(generateSuffix(qtcExt.isEmpty() ? ".shared" : qtcExt));
}

Store UserFileAccessor::postprocessMerge(const Store &main,
                                         const Store &secondary,
                                         const Store &result) const
{
    project()->setProperty(SHARED_SETTINGS, variantFromStore(secondary));
    return MergingSettingsAccessor::postprocessMerge(main, secondary, result);
}

Store UserFileAccessor::preprocessReadSettings(const Store &data) const
{
    Store tmp = MergingSettingsAccessor::preprocessReadSettings(data);

    // Move from old Version field to new one:
    // This cannot be done in a normal upgrader since the version information is needed
    // to decide which upgraders to run
    const Key obsoleteKey = OBSOLETE_VERSION_KEY;
    const int obsoleteVersion = tmp.value(obsoleteKey, -1).toInt();

    if (obsoleteVersion > versionFromMap(tmp))
        setVersionInMap(tmp, obsoleteVersion);

    tmp.remove(obsoleteKey);
    return tmp;
}

Store UserFileAccessor::prepareToWriteSettings(const Store &data) const
{
    const Store tmp = MergingSettingsAccessor::prepareToWriteSettings(data);
    const Store shared = storeFromVariant(retrieveSharedSettings());
    Store result;
    if (!shared.isEmpty()) {
        KeyList stickyKeys;
        SettingsMergeFunction merge = userStickyTrackerFunction(stickyKeys);
        result = storeFromVariant(mergeQVariantMaps(tmp, shared, merge));
        result.insert(USER_STICKY_KEYS_KEY, stringsFromKeys(stickyKeys));
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

Store UserFileVersion14Upgrader::upgrade(const Store &map)
{
    Store result;
    for (auto it = map.cbegin(), end = map.cend(); it != end; ++it) {
        if (it.value().typeId() == QVariant::Map)
            result.insert(it.key(), variantFromStore(upgrade(storeFromVariant(it.value()))));
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

Store UserFileVersion15Upgrader::upgrade(const Store &map)
{
    const QList<Change> changes{
        {"ProjectExplorer.Project.Updater.EnvironmentId", "EnvironmentId"},
        {"ProjectExplorer.Project.UserStickyKeys", "UserStickyKeys"}
    };
    return renameKeys(changes, map);
}

// --------------------------------------------------------------------
// UserFileVersion16Upgrader:
// --------------------------------------------------------------------

UserFileVersion16Upgrader::OldStepMaps UserFileVersion16Upgrader::extractStepMaps(const Store &deployMap)
{
    OldStepMaps result;
    result.defaultDisplayName = deployMap.value("ProjectExplorer.ProjectConfiguration.DefaultDisplayName").toString();
    result.displayName = deployMap.value("ProjectExplorer.ProjectConfiguration.DisplayName").toString();
    const Key stepListKey = "ProjectExplorer.BuildConfiguration.BuildStepList.0";
    Store stepListMap = storeFromVariant(deployMap.value(stepListKey));
    int stepCount = stepListMap.value("ProjectExplorer.BuildStepList.StepsCount", 0).toInt();
    Key stepKey = "ProjectExplorer.BuildStepList.Step.";
    for (int i = 0; i < stepCount; ++i) {
        Store stepMap = storeFromVariant(stepListMap.value(numberedKey(stepKey, i)));
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

Store UserFileVersion16Upgrader::removeAndroidPackageStep(Store deployMap)
{
    const Key stepListKey = "ProjectExplorer.BuildConfiguration.BuildStepList.0";
    Store stepListMap = storeFromVariant(deployMap.value(stepListKey));
    const Key stepCountKey = "ProjectExplorer.BuildStepList.StepsCount";
    int stepCount = stepListMap.value(stepCountKey, 0).toInt();
    Key stepKey = "ProjectExplorer.BuildStepList.Step.";
    int targetPosition = 0;
    for (int sourcePosition = 0; sourcePosition < stepCount; ++sourcePosition) {
        Store stepMap = storeFromVariant(stepListMap.value(numberedKey(stepKey, sourcePosition)));
        if (stepMap.value("ProjectExplorer.ProjectConfiguration.Id").toString()
                != "Qt4ProjectManager.AndroidPackageInstallationStep") {
            stepListMap.insert(numberedKey(stepKey, targetPosition), variantFromStore(stepMap));
            ++targetPosition;
        }
    }

    stepListMap.insert(stepCountKey, targetPosition);

    for (int i = targetPosition; i < stepCount; ++i)
        stepListMap.remove(numberedKey(stepKey, i));

    deployMap.insert(stepListKey, variantFromStore(stepListMap));
    return deployMap;
}

Store UserFileVersion16Upgrader::insertSteps(Store buildConfigurationMap,
                                                   const OldStepMaps &oldStepMap,
                                                   NamePolicy policy)
{
    const Key bslCountKey = "ProjectExplorer.BuildConfiguration.BuildStepListCount";
    int stepListCount = buildConfigurationMap.value(bslCountKey).toInt();

    const Key bslKey = "ProjectExplorer.BuildConfiguration.BuildStepList.";
    const Key bslTypeKey = "ProjectExplorer.ProjectConfiguration.Id";
    for (int bslNumber = 0; bslNumber < stepListCount; ++bslNumber) {
        Store buildStepListMap = buildConfigurationMap.value(numberedKey(bslKey, bslNumber)).value<Store>();
        if (buildStepListMap.value(bslTypeKey) != "ProjectExplorer.BuildSteps.Build")
            continue;

        const Key bslStepCountKey = "ProjectExplorer.BuildStepList.StepsCount";

        int stepCount = buildStepListMap.value(bslStepCountKey).toInt();
        buildStepListMap.insert(bslStepCountKey, stepCount + 2);

        Store androidPackageInstallStep;
        Store androidBuildApkStep;

        // common settings of all buildsteps
        const Key enabledKey = "ProjectExplorer.BuildStep.Enabled";
        const Key idKey = "ProjectExplorer.ProjectConfiguration.Id";
        const Key displayNameKey = "ProjectExplorer.ProjectConfiguration.DisplayName";
        const Key defaultDisplayNameKey = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName";

        QString displayName = oldStepMap.androidPackageInstall.value(displayNameKey).toString();
        QString defaultDisplayName = oldStepMap.androidPackageInstall.value(defaultDisplayNameKey).toString();
        bool enabled = oldStepMap.androidPackageInstall.value(enabledKey).toBool();

        androidPackageInstallStep.insert(idKey, Id("Qt4ProjectManager.AndroidPackageInstallationStep").toSetting());
        androidPackageInstallStep.insert(displayNameKey, displayName);
        androidPackageInstallStep.insert(defaultDisplayNameKey, defaultDisplayName);
        androidPackageInstallStep.insert(enabledKey, enabled);

        displayName = oldStepMap.androidDeployQt.value(keyFromString(displayName)).toString();
        defaultDisplayName = oldStepMap.androidDeployQt.value(defaultDisplayNameKey).toString();
        enabled = oldStepMap.androidDeployQt.value(enabledKey).toBool();

        androidBuildApkStep.insert(idKey, Id("QmakeProjectManager.AndroidBuildApkStep").toSetting());
        androidBuildApkStep.insert(displayNameKey, displayName);
        androidBuildApkStep.insert(defaultDisplayNameKey, defaultDisplayName);
        androidBuildApkStep.insert(enabledKey, enabled);

        // settings transferred from AndroidDeployQtStep to QmakeBuildApkStep
        const Key ProFilePathForInputFile = "ProFilePathForInputFile";
        const Key DeployActionKey = "Qt4ProjectManager.AndroidDeployQtStep.DeployQtAction";
        const Key KeystoreLocationKey = "KeystoreLocation";
        const Key BuildTargetSdkKey = "BuildTargetSdk";
        const Key VerboseOutputKey = "VerboseOutput";

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

        const Key buildStepKey = "ProjectExplorer.BuildStepList.Step.";
        buildStepListMap.insert(numberedKey(buildStepKey, stepCount), variantFromStore(androidPackageInstallStep));
        buildStepListMap.insert(numberedKey(buildStepKey, stepCount + 1), variantFromStore(androidBuildApkStep));

        buildConfigurationMap.insert(numberedKey(bslKey, bslNumber), variantFromStore(buildStepListMap));
    }

    if (policy == RenameBuildConfiguration) {
        const Key displayNameKey = "ProjectExplorer.ProjectConfiguration.DisplayName";
        const Key defaultDisplayNameKey = "ProjectExplorer.ProjectConfiguration.DefaultDisplayName";

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

Store UserFileVersion16Upgrader::upgrade(const Store &data)
{
    int targetCount = data.value("ProjectExplorer.Project.TargetCount", 0).toInt();
    if (!targetCount)
        return data;

    Store result = data;

    for (int i = 0; i < targetCount; ++i) {
        Key targetKey = numberedKey("ProjectExplorer.Project.Target.", i);
        Store targetMap = storeFromVariant(data.value(targetKey));

        const Key dcCountKey = "ProjectExplorer.Target.DeployConfigurationCount";
        int deployconfigurationCount = targetMap.value(dcCountKey).toInt();
        if (!deployconfigurationCount) // should never happen
            continue;

        QList<OldStepMaps> oldSteps;
        QList<Store> oldBuildConfigurations;

        Key deployKey = "ProjectExplorer.Target.DeployConfiguration.";
        for (int j = 0; j < deployconfigurationCount; ++j) {
            Store deployConfigurationMap
                    = storeFromVariant(targetMap.value(numberedKey(deployKey, j)));
            OldStepMaps oldStep = extractStepMaps(deployConfigurationMap);
            if (!oldStep.isEmpty()) {
                oldSteps.append(oldStep);
                deployConfigurationMap = removeAndroidPackageStep(deployConfigurationMap);
                targetMap.insert(numberedKey(deployKey, j), QVariant::fromValue(deployConfigurationMap));
            }
        }

        if (oldSteps.isEmpty()) // no android target?
            continue;

        const Key bcCountKey = "ProjectExplorer.Target.BuildConfigurationCount";
        int buildConfigurationCount
                = targetMap.value(bcCountKey).toInt();

        if (!buildConfigurationCount) // should never happen
            continue;

        Key bcKey = "ProjectExplorer.Target.BuildConfiguration.";
        for (int j = 0; j < buildConfigurationCount; ++j) {
            Store oldBuildConfigurationMap = storeFromVariant(targetMap.value(numberedKey(bcKey, j)));
            oldBuildConfigurations.append(oldBuildConfigurationMap);
        }

        QList<Store> newBuildConfigurations;

        NamePolicy policy = oldSteps.size() > 1 ? RenameBuildConfiguration : KeepName;

        for (const Store &oldBuildConfiguration : std::as_const(oldBuildConfigurations)) {
            for (const OldStepMaps &oldStep : std::as_const(oldSteps)) {
                Store newBuildConfiguration = insertSteps(oldBuildConfiguration, oldStep, policy);
                if (!newBuildConfiguration.isEmpty())
                    newBuildConfigurations.append(newBuildConfiguration);
            }
        }

        targetMap.insert(bcCountKey, newBuildConfigurations.size());

        for (int j = 0; j < newBuildConfigurations.size(); ++j)
            targetMap.insert(numberedKey(bcKey, j), variantFromStore(newBuildConfigurations.at(j)));
        result.insert(targetKey, variantFromStore(targetMap));
    }

    return result;
}

Store UserFileVersion17Upgrader::upgrade(const Store &map)
{
    m_sticky = map.value(USER_STICKY_KEYS_KEY).toList();
    if (m_sticky.isEmpty())
        return map;
    return storeFromVariant(process(variantFromStore(map)));
}

QVariant UserFileVersion17Upgrader::process(const QVariant &entry)
{
    switch (entry.typeId()) {
    case QVariant::List: {
        QVariantList result;
        for (const QVariant &item : entry.toList())
            result.append(process(item));
        return result;
    }
    case QVariant::Map: {
        Store result = storeFromVariant(entry);
        for (Store::iterator i = result.begin(), end = result.end(); i != end; ++i) {
            QVariant &v = i.value();
            v = process(v);
        }
        result.insert(USER_STICKY_KEYS_KEY, m_sticky);
        return variantFromStore(result);
    }
    default:
        return entry;
    }
}

Store UserFileVersion18Upgrader::upgrade(const Store &map)
{
    return storeFromVariant(process(variantFromStore(map)));
}

QVariant UserFileVersion18Upgrader::process(const QVariant &entry)
{
    switch (entry.typeId()) {
    case QVariant::List:
        return Utils::transform(entry.toList(), &UserFileVersion18Upgrader::process);
    case QVariant::Map: {
        Store map = storeFromVariant(entry);
        Store result;
        for (auto it = map.cbegin(), end = map.cend(); it != end; ++it) {
            Key key = it.key() == "AutotoolsProjectManager.MakeStep.AdditionalArguments"
                          ? Key("AutotoolsProjectManager.MakeStep.MakeArguments")
                          : it.key();
            result.insert(key, UserFileVersion18Upgrader::process(it.value()));
        }
        return variantFromStore(result);
    }
    default:
        return entry;
    }
}

Store UserFileVersion19Upgrader::upgrade(const Store &map)
{
    return storeFromVariant(process(variantFromStore(map), KeyList()));
}

QVariant UserFileVersion19Upgrader::process(const QVariant &entry, const KeyList &path)
{
    static const KeyList argsKeys = {"Qt4ProjectManager.MaemoRunConfiguration.Arguments",
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
    static const KeyList wdKeys = {"BareMetal.RunConfig.WorkingDirectory",
                                   "CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory",
                                   "Nim.NimRunConfiguration.WorkingDirectoryAspect",
                                   "ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory",
                                   "Qbs.RunConfiguration.WorkingDirectory",
                                   "Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory",
                                   "RemoteLinux.CustomRunConfig.WorkingDirectory",
                                   "RemoteLinux.RunConfig.WorkingDirectory",
                                   "WorkingDir"};
    static const KeyList termKeys = {"CMakeProjectManager.CMakeRunConfiguration.UseTerminal",
                                     "Nim.NimRunConfiguration.TerminalAspect",
                                     "ProjectExplorer.CustomExecutableRunConfiguration.UseTerminal",
                                     "PythonEditor.RunConfiguration.UseTerminal",
                                     "Qbs.RunConfiguration.UseTerminal",
                                     "Qt4ProjectManager.Qt4RunConfiguration.UseTerminal"};
    static const KeyList libsKeys = {"Qbs.RunConfiguration.UsingLibraryPaths",
                                     "QmakeProjectManager.QmakeRunConfiguration.UseLibrarySearchPath"};
    static const KeyList dyldKeys = {"Qbs.RunConfiguration.UseDyldImageSuffix",
                                     "QmakeProjectManager.QmakeRunConfiguration.UseDyldImageSuffix"};
    switch (entry.typeId()) {
    case QVariant::List:
        return Utils::transform(entry.toList(),
                                std::bind(&UserFileVersion19Upgrader::process, std::placeholders::_1, path));
    case QVariant::Map: {
        Store map = storeFromVariant(entry);
        Store result;
        for (auto it = map.cbegin(), end = map.cend(); it != end; ++it) {
            Key key = it.key();
            QVariant value = it.value();
            if (path.size() == 2
                    && path.at(1).view().startsWith("ProjectExplorer.Target.RunConfiguration.")) {
                if (argsKeys.contains(key))
                    key = "RunConfiguration.Arguments";
                else if (wdKeys.contains(key))
                    key = "RunConfiguration.WorkingDirectory";
                else if (termKeys.contains(key))
                    key = "RunConfiguration.UseTerminal";
                else if (libsKeys.contains(key))
                    key = "RunConfiguration.UseLibrarySearchPath";
                else if (dyldKeys.contains(key))
                    key = "RunConfiguration.UseDyldImageSuffix";
                else
                    value = UserFileVersion19Upgrader::process(value, path + KeyList{key});
            } else {
                value = UserFileVersion19Upgrader::process(value, path + KeyList{key});
            }
            result.insert(key, value);
        };
        return variantFromStore(result);
    }
    default:
        return entry;
    }
}

Store UserFileVersion20Upgrader::upgrade(const Store &map)
{
    return storeFromVariant(process(variantFromStore(map)));
}

QVariant UserFileVersion20Upgrader::process(const QVariant &entry)
{
    switch (entry.typeId()) {
    case QVariant::List:
        return Utils::transform(entry.toList(), &UserFileVersion20Upgrader::process);
    case QVariant::Map: {
        Store map = storeFromVariant(entry);
        Store result;
        for (auto it = map.cbegin(), end = map.cend(); it != end; ++it) {
            Key key = it.key();
            QVariant value = it.value();
            if (key == "ProjectExplorer.ProjectConfiguration.Id" && value == "Qbs.Deploy")
                value = "ProjectExplorer.DefaultDeployConfiguration";
            else
                value = UserFileVersion20Upgrader::process(value);
            result.insert(key, value);
        }
        return variantFromStore(result);
    }
    default:
        return entry;
    }
}

Store UserFileVersion21Upgrader::upgrade(const Store &map)
{
    return storeFromVariant(process(variantFromStore(map)));
}

QVariant UserFileVersion21Upgrader::process(const QVariant &entry)
{
    switch (entry.typeId()) {
    case QVariant::List:
        return Utils::transform(entry.toList(), &UserFileVersion21Upgrader::process);
    case QVariant::Map: {
        Store entryMap = storeFromVariant(entry);
        if (entryMap.value("ProjectExplorer.ProjectConfiguration.Id").toString()
                == "DeployToGenericLinux") {
            entryMap.insert("_checkMakeInstall", true);
            return variantFromStore(entryMap);
        }
        Store map = storeFromVariant(entry);
        Store result;
        for (auto it = map.cbegin(), end = map.cend(); it != end; ++it)
            result.insert(it.key(), UserFileVersion21Upgrader::process(it.value()));
        return variantFromStore(result);
    }
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

    void storeSharedSettings(const Store &data) const { m_storedSettings = data; }
    QVariant retrieveSharedSettings() const override { return variantFromStore(m_storedSettings); }

    using UserFileAccessor::preprocessReadSettings;
    using UserFileAccessor::prepareToWriteSettings;

    using UserFileAccessor::mergeSettings;

private:
    mutable Store m_storedSettings;
};


class TestProject : public Project
{
public:
    TestProject() : Project("x-test/testproject", "/test/project") { setDisplayName("Test Project"); }

    bool needsConfiguration() const final { return false; }
};

} // namespace

void ProjectExplorerPlugin::testUserFileAccessor_prepareToReadSettings()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    Store data;
    data.insert("Version", 4);
    data.insert("Foo", "bar");

    Store result = accessor.preprocessReadSettings(data);

    QCOMPARE(result, data);
}

void ProjectExplorerPlugin::testUserFileAccessor_prepareToReadSettingsObsoleteVersion()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    Store data;
    data.insert("ProjectExplorer.Project.Updater.FileVersion", 4);
    data.insert("Foo", "bar");

    Store result = accessor.preprocessReadSettings(data);

    QCOMPARE(result.count(), data.count());
    QCOMPARE(result.value("Foo"), data.value("Foo"));
    QCOMPARE(result.value("Version"), data.value("ProjectExplorer.Project.Updater.FileVersion"));
}

void ProjectExplorerPlugin::testUserFileAccessor_prepareToReadSettingsObsoleteVersionNewVersion()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    Store data;
    data.insert("ProjectExplorer.Project.Updater.FileVersion", 4);
    data.insert("Version", 5);
    data.insert("Foo", "bar");

    Store result = accessor.preprocessReadSettings(data);

    QCOMPARE(result.count(), data.count() - 1);
    QCOMPARE(result.value("Foo"), data.value("Foo"));
    QCOMPARE(result.value("Version"), data.value("Version"));
}

void ProjectExplorerPlugin::testUserFileAccessor_prepareToWriteSettings()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    Store sharedData;
    sharedData.insert("Version", 10);
    sharedData.insert("shared1", "bar");
    sharedData.insert("shared2", "baz");
    sharedData.insert("shared3", "foo");

    accessor.storeSharedSettings(sharedData);

    Store data;
    data.insert("Version", 10);
    data.insert("shared1", "bar1");
    data.insert("unique1", 1234);
    data.insert("shared3", "foo");
    Store result = accessor.prepareToWriteSettings(data);

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

    Store sharedData;
    sharedData.insert("Version", accessor.currentVersion());
    sharedData.insert("shared1", "bar");
    sharedData.insert("shared2", "baz");
    sharedData.insert("shared3", "foooo");
    TestUserFileAccessor::RestoreData shared("/shared/data", sharedData);

    Store data;
    data.insert("Version", accessor.currentVersion());
    data.insert("EnvironmentId", projectExplorerSettings().environmentId.toByteArray());
    data.insert("UserStickyKeys", QStringList({"shared1"}));
    data.insert("shared1", "bar1");
    data.insert("unique1", 1234);
    data.insert("shared3", "foo");
    TestUserFileAccessor::RestoreData user("/user/data", data);
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

    Store sharedData;
    sharedData.insert("Version", accessor.currentVersion());
    sharedData.insert("shared1", "bar");
    sharedData.insert("shared2", "baz");
    sharedData.insert("shared3", "foooo");
    TestUserFileAccessor::RestoreData shared("/shared/data", sharedData);

    Store data;
    TestUserFileAccessor::RestoreData user("/shared/data", data);

    TestUserFileAccessor::RestoreData result = accessor.mergeSettings(user, shared);

    QVERIFY(!result.hasIssue());
    QCOMPARE(result.data, sharedData);
}

void ProjectExplorerPlugin::testUserFileAccessor_mergeSettingsEmptyShared()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    Store sharedData;
    TestUserFileAccessor::RestoreData shared("/shared/data", sharedData);

    Store data;
    data.insert("Version", accessor.currentVersion());
    data.insert("OriginalVersion", accessor.currentVersion());
    data.insert("EnvironmentId", projectExplorerSettings().environmentId.toByteArray());
    data.insert("UserStickyKeys", QStringList({"shared1"}));
    data.insert("shared1", "bar1");
    data.insert("unique1", 1234);
    data.insert("shared3", "foo");
    TestUserFileAccessor::RestoreData user("/shared/data", data);

    TestUserFileAccessor::RestoreData result = accessor.mergeSettings(user, shared);

    QVERIFY(!result.hasIssue());
    QCOMPARE(result.data, data);
}

#endif // WITH_TESTS
