// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QPromise>
#include <QTemporaryDir>
#include <QtTest>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include "../cmakeinstallrules.h"
#include "../fileapiparser.h"

using namespace CMakeProjectManager::Internal;
using namespace CMakeProjectManager::Internal::FileApiDetails;
using namespace ProjectExplorer;
using namespace Utils;

// The installers below are what CMake 4.4 wrote for a project exercising one install()
// form each; only the paths of the directory rules point somewhere else, so that a test
// can put files there.
static Installer installer(const QString &type,
                           const QString &destination,
                           const std::vector<PathPair> &paths)
{
    Installer result;
    result.type = type;
    result.destination = destination;
    result.paths = paths;
    return result;
}

static Installer targetInstaller(const QString &destination,
                                 const QString &path,
                                 const QString &targetId)
{
    Installer result = installer("target", destination, {{path, {}}});
    result.targetId = targetId;
    return result;
}

static std::vector<TargetDetails> probeTargets()
{
    const auto target = [](const QString &id, const QString &type, const FilePaths &artifacts) {
        TargetDetails result;
        result.id = id;
        result.type = type;
        result.artifacts = artifacts;
        return result;
    };
    // The artifact of a bundle sits inside the directory the install rule of the target
    // names, which is how such a rule tells itself apart from one naming a file.
    return {target("app::@0", "EXECUTABLE", {"app.exe"}),
            target("shared::@0", "SHARED_LIBRARY", {"shared.dll", "shared.lib"}),
            target("stat::@0", "STATIC_LIBRARY", {"stat.lib"}),
            target("sublib::@1", "STATIC_LIBRARY", {"sub/sublib.lib"}),
            target("bundle::@0", "EXECUTABLE", {"bundle.app/Contents/MacOS/bundle"})};
}

static QStringList described(const DeploymentData &data)
{
    QStringList result = Utils::transform(data.allFiles(), [](const DeployableFile &f) -> QString {
        const QString suffix = f.isExecutable() ? QString(" (executable)") : QString();
        return f.localFilePath().path() + " -> " + f.remoteFilePath() + suffix;
    });
    result.sort();
    return result;
}

// QCOMPARE has nothing to print for a plain enum, so give it a name to print.
static QString described(DeploymentKnowledge knowledge)
{
    switch (knowledge) {
    case DeploymentKnowledge::Perfect:
        return "perfect";
    case DeploymentKnowledge::Approximative:
        return "approximative";
    case DeploymentKnowledge::Bad:
        return "bad";
    }
    return "unknown";
}

class InstallRulesTest : public QObject
{
    Q_OBJECT

public:
    // A default-constructed QFuture is a canceled one, which the code under test gives up
    // on; a promise of our own hands it one that is still running.
    InstallRulesTest() { m_promise.start(); }

private:
    QFuture<void> cancelFuture() const { return m_promise.future(); }

    QPromise<void> m_promise;

private slots:
    void installRules()
    {
        Installer importLibrary = targetInstaller("lib/static", "shared.lib", "shared::@0");
        importLibrary.targetIsImportLibrary = true;

        Installer excluded = installer("file", "extra", {{"data/app.conf", {}}});
        excluded.component = "extras";
        excluded.isExcludeFromAll = true;

        // What a library installs for the projects using it, not for a target to run.
        Installer headers = installer("fileSet", "include", {{"include/lib.h", "lib.h"}});
        headers.fileSetName = "probeHeaders";
        headers.fileSetType = "HEADERS";

        DirectoryDetails top;
        top.installers = {
            targetInstaller("bin", "app.exe", "app::@0"),
            importLibrary,
            targetInstaller("bin", "shared.dll", "shared::@0"),
            targetInstaller("lib/static", "stat.lib", "stat::@0"),
            installer("file", "etc", {{"data/app.conf", {}}}),
            installer("file", "bin", {{"run.sh", {}}}),
            // A generator expression in a destination is resolved already.
            installer("file", "etc/Debug", {{"data/app.conf", {}}}),
            excluded,
            headers,
            installer("export", "lib/cmake/probe", {{"probe-config.cmake", {}}}),
            installer("file", "/opt/absolute", {{"data/app.conf", {}}}),
        };

        DirectoryDetails sub;
        sub.installers = {
            targetInstaller("lib/sub", "sub/sublib.lib", "sublib::@1"),
            installer("cxxModuleBmi", "lib/sub", {}),
            installer("file", "etc/sub", {{"sub/../data/app.conf", {}}}),
        };

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top, sub}, probeTargets(), "/src", "/build", "/opt/probe");

        QCOMPARE(described(deployment.data),
                 QStringList({
                     // Only the executable is marked as one; the import library of the
                     // shared library is not even a candidate.
                     "/build/app.exe -> /opt/probe/bin/app.exe (executable)",
                     "/build/shared.dll -> /opt/probe/bin/shared.dll",
                     "/build/shared.lib -> /opt/probe/lib/static/shared.lib",
                     "/build/stat.lib -> /opt/probe/lib/static/stat.lib",
                     "/build/sub/sublib.lib -> /opt/probe/lib/sub/sublib.lib",
                     "/src/data/app.conf -> /opt/absolute/app.conf",
                     "/src/data/app.conf -> /opt/probe/etc/Debug/app.conf",
                     "/src/data/app.conf -> /opt/probe/etc/app.conf",
                     "/src/data/app.conf -> /opt/probe/etc/sub/app.conf",
                     "/src/run.sh -> /opt/probe/bin/run.sh",
                 }));
        QCOMPARE(described(deployment.knowledge), described(DeploymentKnowledge::Perfect));
    }

    void noInstallPrefix()
    {
        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0"),
                          installer("file", "/opt/absolute", {{"data/app.conf", {}}})};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, probeTargets(), "/src", "/build", {});

        QCOMPARE(described(deployment.data),
                 QStringList({"/build/app.exe -> bin/app.exe (executable)",
                              "/src/data/app.conf -> /opt/absolute/app.conf"}));
    }

    void installPrefixLosesItsDrive()
    {
        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0"),
                          installer("file", "/opt/absolute", {{"data/app.conf", {}}})};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(),
            {top},
            probeTargets(),
            "/src",
            "/build",
            "C:/Program Files (x86)/probe");

        // The drive of the host says nothing on the target, and an install with DESTDIR
        // set leaves it out as well.
        QCOMPARE(described(deployment.data),
                 QStringList({"/build/app.exe -> /Program Files (x86)/probe/bin/app.exe "
                              "(executable)",
                              "/src/data/app.conf -> /opt/absolute/app.conf"}));
    }

    void duplicatesCollapse()
    {
        DirectoryDetails top;
        top.installers = {installer("file", "etc", {{"data/app.conf", {}}}),
                          installer("file", "etc/", {{"data/app.conf", {}}}),
                          installer("file", "etc", {{"data/app.conf", {}}})};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, {}, "/src", "/build", "/opt/probe");

        QCOMPARE(described(deployment.data),
                 QStringList({"/src/data/app.conf -> /opt/probe/etc/app.conf"}));
    }

    void executableSurvivesADuplicate_data()
    {
        QTest::addColumn<bool>("targetFirst");
        QTest::newRow("install(TARGETS) first") << true;
        QTest::newRow("install(FILES) first") << false;
    }

    void executableSurvivesADuplicate()
    {
        QFETCH(bool, targetFirst);

        // A DeployableFile compares by local path and remote directory alone, so the two
        // rules below name one file. Its type is not part of that comparison, and losing
        // it would leave the binary on the target without its executable bit.
        const Installer target = targetInstaller("bin", "app.exe", "app::@0");
        const Installer file = installer("file", "bin", {{"../build/app.exe", {}}});

        DirectoryDetails top;
        top.installers = targetFirst ? std::vector<Installer>{target, file}
                                     : std::vector<Installer>{file, target};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, probeTargets(), "/src", "/build", "/opt/probe");

        QCOMPARE(described(deployment.data),
                 QStringList({"/build/app.exe -> /opt/probe/bin/app.exe (executable)"}));
    }

    void renamedFileLeavesNoDeployment()
    {
        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0"),
                          installer("file", "etc", {{"data/app.conf", "other.conf"}})};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, probeTargets(), "/src", "/build", "/opt/probe");

        // A DeployableFile has nowhere to hold the name an install(FILES ... RENAME) gives
        // a file, so the deployment cannot be described at all rather than describing the
        // file under the wrong name.
        QCOMPARE(described(deployment.data), QStringList());
        QCOMPARE(described(deployment.knowledge), described(DeploymentKnowledge::Bad));
    }

    void unknownFilesLeaveNoDeployment_data()
    {
        QTest::addColumn<QString>("type");
        QTest::newRow("install(SCRIPT)") << "script";
        QTest::newRow("install(CODE)") << "code";
        QTest::newRow("install(IMPORTED_RUNTIME_ARTIFACTS)") << "importedRuntimeArtifacts";
        QTest::newRow("RUNTIME_DEPENDENCY_SET") << "runtimeDependencySet";
        QTest::newRow("a type of a later CMake") << "somethingAddedAfterThisWasWritten";
    }

    void unknownFilesLeaveNoDeployment()
    {
        QFETCH(QString, type);

        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0"),
                          installer(type, "bin", {})};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, probeTargets(), "/src", "/build", "/opt/probe");

        // These rules install the libraries of the application, which is what a
        // deployment is for, and a type nobody here knows may well do the same: the
        // deploy step that installs the project has to keep running, and it does so only
        // as long as knowledge stays Bad.
        QCOMPARE(described(deployment.data), QStringList());
        QCOMPARE(described(deployment.knowledge), described(DeploymentKnowledge::Bad));
    }

    void optionalInstallerLeavesNoDeployment()
    {
        // An OPTIONAL rule installs its file only where the build produced one - the
        // debug symbols of a release build are the usual case - and a deploy step takes a
        // local file that is missing for an error. Deploying the rest and calling that
        // the deployment would leave the file out of it for good.
        Installer optional = installer("file", "optional", {{"data/app.conf", {}}});
        optional.isOptional = true;

        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0"), optional};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, probeTargets(), "/src", "/build", "/opt/probe");

        QCOMPARE(described(deployment.data), QStringList());
        QCOMPARE(described(deployment.knowledge), described(DeploymentKnowledge::Bad));
    }

    void anApplicationBundleLeavesNoDeployment()
    {
        // install(TARGETS bundle BUNDLE DESTINATION .) names the directory of the bundle,
        // and what is in it is nowhere in the rules. A DeployableFile holds a file, so
        // taking the name would have a deploy step upload a directory as one.
        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0"),
                          targetInstaller(".", "bundle.app", "bundle::@0")};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, probeTargets(), "/src", "/build", "/opt/probe");

        QCOMPARE(described(deployment.data), QStringList());
        QCOMPARE(described(deployment.knowledge), described(DeploymentKnowledge::Bad));
    }

    void anEmptyDestinationLeavesNoDeployment_data()
    {
        QTest::addColumn<QString>("destination");
        QTest::addColumn<QString>("installPrefix");
        // Where the file goes is not known, which is not the same as knowing it goes
        // nowhere - with an install prefix to fall back on just as much as without one.
        QTest::newRow("no destination") << QString() << QString();
        QTest::newRow("no destination, but a prefix") << QString() << QString("/opt/probe");
        // A DeployableFile holds a directory and a file name with a slash in between, so
        // the root of the target is not a destination it can express. The install prefix
        // is not where those files go either: CMake ignores it for an absolute one.
        QTest::newRow("the root of the target") << QString("/") << QString("/opt/probe");
        QTest::newRow("the root of a drive") << QString("C:/") << QString("/opt/probe");
    }

    void anEmptyDestinationLeavesNoDeployment()
    {
        QFETCH(QString, destination);
        QFETCH(QString, installPrefix);

        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0"),
                          installer("file", destination, {{"data/app.conf", {}}})};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, probeTargets(), "/src", "/build", installPrefix);

        QCOMPARE(described(deployment.data), QStringList());
        QCOMPARE(described(deployment.knowledge), described(DeploymentKnowledge::Bad));
    }

    void unknownFilesOfAnotherComponentDoNotCount()
    {
        Installer script = installer("script", "bin", {});
        script.component = "extras";
        script.isExcludeFromAll = true;

        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0"), script};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, probeTargets(), "/src", "/build", "/opt/probe");

        QCOMPARE(described(deployment.data),
                 QStringList({"/build/app.exe -> /opt/probe/bin/app.exe (executable)"}));
        QCOMPARE(described(deployment.knowledge), described(DeploymentKnowledge::Perfect));
    }

    void aDirectoryRuleLeavesNoDeployment_data()
    {
        QTest::addColumn<QStringList>("contents");
        // The files of the directory are right there and still not the ones to deploy:
        // which of them a PATTERN or EXCLUDE option of the rule keeps is not reported,
        // and a file added afterwards reconfigures nothing that would pick it up, so the
        // list would go stale without saying so.
        QTest::newRow("the directory holds files") << QStringList{"assets/a.png",
                                                                  "assets/keep/b.png"};
        // The QML modules of an application are written by the build, so the rule
        // installing them has nothing to show while the install rules are read.
        QTest::newRow("the directory is empty") << QStringList();
    }

    // A deployment silently missing files is worse than none at all, so a directory rule
    // hands over no data and leaves knowledge Bad - which is what keeps the deploy step
    // that installs the project in the deploy configuration to list the files for real.
    void aDirectoryRuleLeavesNoDeployment()
    {
        QFETCH(QStringList, contents);

        QTemporaryDir temporaryDir;
        QVERIFY(temporaryDir.isValid());
        const FilePath sourceDirectory = FilePath::fromString(temporaryDir.path());
        QVERIFY(sourceDirectory.pathAppended("assets").ensureWritableDir());
        for (const QString &relative : contents) {
            const FilePath file = sourceDirectory.pathAppended(relative);
            QVERIFY(file.parentDir().ensureWritableDir());
            QVERIFY(file.writeFileContents("x"));
        }

        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0"),
                          installer("directory", "share/probe", {{"assets", {}}})};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            cancelFuture(), {top}, probeTargets(), sourceDirectory, "/build", "/opt/probe");

        QCOMPARE(described(deployment.data), QStringList());
        QCOMPARE(described(deployment.knowledge), described(DeploymentKnowledge::Bad));
    }

    void aCanceledParseIsGivenUpOn()
    {
        DirectoryDetails top;
        top.installers = {targetInstaller("bin", "app.exe", "app::@0")};

        const InstallRuleDeployment deployment = deploymentFromInstallRules(
            {}, {top}, probeTargets(), "/src", "/build", "/opt/probe");

        QCOMPARE(described(deployment.data), QStringList());
        QCOMPARE(described(deployment.knowledge), described(DeploymentKnowledge::Bad));
    }
};

QTEST_GUILESS_MAIN(InstallRulesTest)
#include "tst_cmake_install_rules.moc"
