// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "userfileaccessor.h"

#include "abi.h"
#include "buildsystem.h"
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
#include <utils/qtcprocess.h>
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

const char SHARED_SETTINGS[] = "SharedSettings";
const char USER_STICKY_KEYS_KEY[] = "UserStickyKeys";

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

// --------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------

namespace {

static QString generateSuffix(const QString &suffix)
{
    QString result = suffix;
    static const QRegularExpression regexp("[^a-zA-Z0-9_.-]");
    result.replace(regexp, QString('_')); // replace fishy character
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
        return FilePath::fromString(
            *externalUserFileDir + '/' + makeRelative(projectFilePath.toUrlishString()) + suffix);
    }
    return {};
}

} // namespace

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

    setSettingsId(projectExplorerSettings().environmentId.toByteArray());

    // Register Upgraders:
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

    return result;
}

Store UserFileVersion18Upgrader::upgrade(const Store &map)
{
    return storeFromVariant(process(variantFromStore(map)));
}

QVariant UserFileVersion18Upgrader::process(const QVariant &entry)
{
    switch (entry.typeId()) {
    case QMetaType::QVariantList:
        return Utils::transform(entry.toList(), &UserFileVersion18Upgrader::process);
    case QMetaType::QVariantMap: {
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
    case QMetaType::QVariantList:
        return Utils::transform(entry.toList(),
                                std::bind(&UserFileVersion19Upgrader::process, std::placeholders::_1, path));
    case QMetaType::QVariantMap: {
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
    case QMetaType::QVariantList:
        return Utils::transform(entry.toList(), &UserFileVersion20Upgrader::process);
    case QMetaType::QVariantMap: {
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
    case QMetaType::QVariantList:
        return Utils::transform(entry.toList(), &UserFileVersion21Upgrader::process);
    case QMetaType::QVariantMap: {
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

#ifdef WITH_TESTS

#include <QTest>

#include "projectexplorer_test.h"

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


class TestBuildSystem : public BuildSystem
{
public:
    using BuildSystem::BuildSystem;
private:
    void triggerParsing() override {}
};

class TestProject : public Project
{
public:
    TestProject() : Project("x-test/testproject", "/test/project")
    {
        setDisplayName("Test Project");
        setBuildSystemCreator<TestBuildSystem>("UserFileAccessorTest");
    }

    bool needsConfiguration() const final { return false; }
};

} // namespace

void ProjectExplorerTest::testUserFileAccessor_prepareToReadSettings()
{
    TestProject project;
    TestUserFileAccessor accessor(&project);

    Store data;
    data.insert("Version", 4);
    data.insert("Foo", "bar");

    Store result = accessor.preprocessReadSettings(data);

    QCOMPARE(result, data);
}

void ProjectExplorerTest::testUserFileAccessor_prepareToWriteSettings()
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

    QCOMPARE(result.count(), data.count() + 2);
    QCOMPARE(result.value("EnvironmentId").toByteArray(),
             projectExplorerSettings().environmentId.toByteArray());
    QCOMPARE(result.value("UserStickyKeys"), QVariant(QStringList({"shared1"})));
    QCOMPARE(result.value("Version").toInt(), accessor.currentVersion());
    QCOMPARE(result.value("shared1"), data.value("shared1"));
    QCOMPARE(result.value("shared3"), data.value("shared3"));
    QCOMPARE(result.value("unique1"), data.value("unique1"));
}

void ProjectExplorerTest::testUserFileAccessor_mergeSettings()
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

void ProjectExplorerTest::testUserFileAccessor_mergeSettingsEmptyUser()
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

void ProjectExplorerTest::testUserFileAccessor_mergeSettingsEmptyShared()
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
