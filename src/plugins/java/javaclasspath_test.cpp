// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javaclasspath_test.h"

#include "javaclasspath.h"
#include "javalanguageserver.h"

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/temporarydirectory.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QRegularExpression>
#include <QTest>

using namespace Utils;

namespace Java::Internal {

class JavaClassPathTest : public QObject
{
    Q_OBJECT

private slots:
    void testPackageOf_data();
    void testPackageOf();

    void testSourceRoots();

    void testMergedClassPath();
    void testUserConfigurationIsKept();
    void testJavaIn();
    void testJavaRuns();
    void testJdtlsArguments();
    void testJdtlsCommandIn();
    void testRemoveLegacyEclipseProjectFiles();
};

// Writes a file where a JDK keeps its java, under the name a JDK gives it:
// "java.exe" on Windows, an executable "java" everywhere else. javaIn() only
// ever asks whether that file is there, so it need be no program.
static FilePath writeJava(const FilePath &binDirectory)
{
    if (!binDirectory.ensureWritableDir())
        return {};

    const FilePath java = binDirectory / HostOsInfo::withExecutableSuffix("java");
    if (!java.writeFileContents({}))
        return {};
    if (!HostOsInfo::isWindowsHost()
        && !java.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner)) {
        return {};
    }
    return java.isExecutableFile() ? java : FilePath();
}

// Writes a program that does nothing but leave the exit code given. A shell
// script, so only where a shell runs one: a Process is handed the program
// itself, and on Windows that leaves out the batch files, which are run by
// a cmd.exe rather than started.
static FilePath writeProgram(const FilePath &directory, const QString &name, int exitCode)
{
    if (!directory.ensureWritableDir())
        return {};

    const FilePath program = directory / name;
    if (!program.writeFileContents(QString("#!/bin/sh\nexit %1\n").arg(exitCode).toUtf8()))
        return {};
    if (!program.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner))
        return {};
    return program;
}

// What the build system says and what a plugin that knows the project adds on
// top are one class path, and what neither has to say is nothing at all.
void JavaClassPathTest::testMergedClassPath()
{
    const auto classPath = [](const QStringList &sources, const QStringList &libraries) {
        JavaClassPath path;
        path.sourceDirectories = FilePaths::fromStrings(sources);
        path.libraries = FilePaths::fromStrings(libraries);
        return path;
    };

    QVERIFY(!mergedClassPath({}, {}));

    const auto isSameAs = [](const std::optional<JavaClassPath> &merged,
                             const JavaClassPath &expected) {
        return merged && merged->sourceDirectories == expected.sourceDirectories
               && merged->libraries == expected.libraries;
    };

    // What one of them says alone is what they say together.
    const JavaClassPath fromBuildSystem = classPath({"/p/src"}, {"/p/a.jar"});
    QVERIFY(isSameAs(mergedClassPath(fromBuildSystem, {}), fromBuildSystem));
    QVERIFY(isSameAs(mergedClassPath({}, fromBuildSystem), fromBuildSystem));

    // The one on top adds to the one below rather than taking its place, and
    // what both of them name is named once.
    const std::optional<JavaClassPath> both
        = mergedClassPath(fromBuildSystem, classPath({"/p/src", "/q/src"}, {"/p/a.jar"}));
    QVERIFY(both);
    QCOMPARE(both->sourceDirectories, FilePaths::fromStrings({"/p/src", "/q/src"}));
    QCOMPARE(both->libraries, FilePaths::fromStrings({"/p/a.jar"}));
}

// What the build system says about the Java of a project goes in beside the
// configuration of the user rather than over it, and what the user set is
// left as it is, so that a class path that comes and goes does not take any
// of it along.
void JavaClassPathTest::testUserConfigurationIsKept()
{
    const QJsonObject project{{"sourcePaths", QJsonArray{"/p/src"}},
                              {"referencedLibraries", QJsonArray{"/p/android.jar"}}};
    const QJsonValue userConfiguration = QJsonObject{
        {"java",
         QJsonObject{{"project",
                      QJsonObject{{"encoding", "warning"},
                                  {"referencedLibraries", QJsonArray{"lib/**/*.jar"}}}},
                     {"format", QJsonObject{{"enabled", true}}}}}};

    // A project nothing knows the Java of leaves the user's own alone.
    QCOMPARE(javaConfigurationWithProject(userConfiguration, {}), userConfiguration);
    QCOMPARE(javaConfigurationWithProject({}, {}), QJsonValue(QJsonObject{}));

    const QJsonValue sent = javaConfigurationWithProject(userConfiguration, project);
    const QJsonObject java = sent.toObject().value("java").toObject();
    const QJsonObject sentProject = java.value("project").toObject();
    QCOMPARE(java.value("format").toObject().value("enabled").toBool(), true);
    QCOMPARE(sentProject.value("encoding").toString(), QString("warning"));
    QCOMPARE(sentProject.value("sourcePaths").toArray(), QJsonArray{"/p/src"});
    QCOMPARE(sentProject.value("referencedLibraries").toArray(),
             (QJsonArray{"lib/**/*.jar", "/p/android.jar"}));

    // A project whose Java has gone away takes what it said with it and
    // nothing else.
    QCOMPARE(javaConfigurationWithProject(userConfiguration, {}), userConfiguration);

    // A path the user named themselves is named once where the build system
    // names it too, and stays where the build system stops naming it: it is
    // theirs, not a copy of what the project happened to say.
    const QJsonValue userWithSharedLibrary = QJsonObject{
        {"java",
         QJsonObject{{"project",
                      QJsonObject{{"referencedLibraries", QJsonArray{"/p/android.jar"}}}}}}};
    const QJsonObject mergedProject = javaConfigurationWithProject(userWithSharedLibrary, project)
                                          .toObject().value("java").toObject()
                                          .value("project").toObject();
    QCOMPARE(mergedProject.value("referencedLibraries").toArray(), QJsonArray{"/p/android.jar"});
    QCOMPARE(javaConfigurationWithProject(userWithSharedLibrary, {}), userWithSharedLibrary);

    // Which holds because what comes back in is told from what the user set:
    // Client hands a client its own configuration again when the server comes
    // up, and reading that as the user's would make what the build system
    // named look like theirs - and let go of what is theirs with it.
    const QJsonValue sentWithShared = javaConfigurationWithProject(userWithSharedLibrary, project);
    QCOMPARE(userJavaConfiguration(sentWithShared, sentWithShared, userWithSharedLibrary),
             userWithSharedLibrary);
    QCOMPARE(userJavaConfiguration(userConfiguration, sentWithShared, userWithSharedLibrary),
             userConfiguration);

    // "referencedLibraries" is a list of its own or an object of an include
    // list beside an exclude one and the sources of the jars, both of which
    // the server reads. What the build system names goes into the list the
    // object includes, and the rest of it is left where it is.
    const QJsonValue asAnObject = QJsonObject{
        {"java",
         QJsonObject{
             {"project",
              QJsonObject{{"referencedLibraries",
                           QJsonObject{{"include", QJsonArray{"lib/**/*.jar"}},
                                       {"exclude", QJsonArray{"lib/sources/**"}},
                                       {"sources",
                                        QJsonObject{{"lib/foo.jar", "lib/foo-src.jar"}}}}}}}}}};

    const QJsonValue sentAsAnObject = javaConfigurationWithProject(asAnObject, project);
    const QJsonObject libraries = sentAsAnObject.toObject()
                                      .value("java").toObject()
                                      .value("project").toObject()
                                      .value("referencedLibraries").toObject();
    QCOMPARE(libraries.value("include").toArray(),
             (QJsonArray{"lib/**/*.jar", "/p/android.jar"}));
    QCOMPARE(libraries.value("exclude").toArray(), QJsonArray{"lib/sources/**"});
    QCOMPARE(libraries.value("sources").toObject().value("lib/foo.jar").toString(),
             QString("lib/foo-src.jar"));
    QCOMPARE(javaConfigurationWithProject(asAnObject, {}), asAnObject);

    // A class path that has gone away is told as empty lists: a configuration
    // that no longer names the keys is one the server goes on resolving
    // against what it had, and an empty one is not sent to it at all.
    QCOMPARE(javaProjectToSend({}, {}), QJsonObject{});
    QCOMPARE(javaProjectToSend(project, {}), project);
    const QJsonObject gone = javaProjectToSend({}, project);
    QCOMPARE(gone.value("sourcePaths").toArray(), QJsonArray{});
    QCOMPARE(gone.value("referencedLibraries").toArray(), QJsonArray{});
    QVERIFY2(!javaConfigurationWithProject({}, gone).toObject().isEmpty(),
             "nothing is sent, so the server goes on resolving against a class path that is gone");
}

// A JAVA_HOME is set by whoever installed a JDK and names it outright, so it
// is taken before the PATH, where the first "java" need be no JDK at all.
void JavaClassPathTest::testJavaIn()
{
    TemporaryDirectory temporaryDirectory("qtc-java-home-XXXXXX");
    const FilePath root = temporaryDirectory.path();

    const FilePath jdk = root / "jdk";
    const FilePath fromJavaHome = writeJava(jdk / "bin");
    QVERIFY(!fromJavaHome.isEmpty());

    const FilePath elsewhere = root / "elsewhere";
    const FilePath onPath = writeJava(elsewhere);
    QVERIFY(!onPath.isEmpty());

    Environment environment;
    environment.prependOrSetPath(elsewhere);
    QCOMPARE(javaIn(environment), onPath);

    environment.set("JAVA_HOME", jdk.toUserOutput());
    QCOMPARE(javaIn(environment), fromJavaHome);

    // A JAVA_HOME that names no java is no answer, and the PATH stands.
    environment.set("JAVA_HOME", (root / "nowhere").toUserOutput());
    QCOMPARE(javaIn(environment), onPath);
}

// An executable file called "java" is no proof of a JDK: macOS ships
// /usr/bin/java as a stub that reports that there is no runtime, and a java
// too old for the server fails just as late. Either would be handed the
// launcher jar and leave the user with a server that never comes up.
void JavaClassPathTest::testJavaRuns()
{
    TemporaryDirectory temporaryDirectory("qtc-java-runs-XXXXXX");
    const FilePath root = temporaryDirectory.path();

    // What the stub of macOS is: a java that starts and reports that it has
    // no runtime. On Windows it is an empty "java.exe" instead, which the
    // suffix makes executable and nothing stands behind - as close as a test
    // gets there, since a program that does run is a PE binary and not
    // something to write from here.
    const FilePath stub = HostOsInfo::isWindowsHost() ? writeJava(root / "stub")
                                                      : writeProgram(root, "stub", 1);
    QVERIFY(!stub.isEmpty());
    QVERIFY2(stub.isExecutableFile(), "the stub must look as good as a JDK to be worth testing");
    QVERIFY2(!javaRuns(stub),
             "a java that cannot run the server was taken for one that can");

    QVERIFY(!javaRuns(root / "nothing"));
    QVERIFY(!javaRuns({}));

    if (HostOsInfo::isWindowsHost())
        return;

    const FilePath works = writeProgram(root, "java", 0);
    QVERIFY(!works.isEmpty());
    QVERIFY(javaRuns(works));
}

// A java is told which jar to run the server from and where the configuration
// of it is, a launcher that starts the server by itself is told none of that,
// and an installation that has no configuration for this machine is told
// nothing, which leaves a setting that isValid() turns down.
void JavaClassPathTest::testJdtlsArguments()
{
    TemporaryDirectory temporaryDirectory("qtc-java-jls-XXXXXX");
    const FilePath installation = temporaryDirectory.path();
    const FilePath launcherJar = installation / "plugins/org.eclipse.equinox.launcher_1.6.jar";
    QVERIFY(launcherJar.parentDir().ensureWritableDir());
    QVERIFY(launcherJar.writeFileContents({}));

    const FilePath java = installation / "java";
    const FilePath launcher = installation / "jdtls";

    const auto configurationIn = [](const QString &arguments) {
        static const QRegularExpression pattern(R"re(-configuration "([^"]*)")re");
        return pattern.match(arguments).captured(1);
    };

    // An installation that ships no configuration for this system is one no
    // command line is known for, and so is a setting with no jar at all. The
    // installation directory itself is not a configuration either.
    QVERIFY(jdtlsArguments(java, launcherJar).isEmpty());
    QVERIFY(jdtlsArguments(java, {}).isEmpty());

    for (const QString &name : {QString("config_win"), QString("config_linux"),
                                QString("config_mac")}) {
        QVERIFY((installation / name).ensureWritableDir());
        QVERIFY((installation / (name + "_arm")).ensureWritableDir());
    }

    // The configuration is per architecture as well as per system.
    const QString perArchitecture = jdtlsArguments(java, launcherJar);
    QVERIFY(perArchitecture.contains(QString("-jar \"%1\"").arg(launcherJar.path())));
    const QString chosen = configurationIn(perArchitecture);
    QVERIFY(FilePath::fromString(chosen).isReadableDir());
    QCOMPARE(chosen.endsWith("_arm"), HostOsInfo::hostArchitecture() == OsArchArm64);

    // An installation that ships the plain directory alone is read as well,
    // and it is the one of the same system.
    for (const QString &name : {QString("config_win_arm"), QString("config_linux_arm"),
                                QString("config_mac_arm")}) {
        QVERIFY((installation / name).removeRecursively());
    }
    QCOMPARE(configurationIn(jdtlsArguments(java, launcherJar)),
             QString(chosen).remove("_arm"));

    // A launcher of the server works the jar out by itself, so the ones of a
    // jar the setting still names must not reach it.
    QVERIFY2(jdtlsArguments(launcher, launcherJar).isEmpty(),
             "the arguments of a launcher jar were given to a launcher that knows nothing of it");
    QVERIFY(jdtlsArguments(launcher, {}).isEmpty());
}

// The archive of the Eclipse project ships a launcher beside the launcher
// jar, and that launcher is a Python program the machine of the user need not
// have. A java that can be told to run the jar is taken over it.
void JavaClassPathTest::testJdtlsCommandIn()
{
    TemporaryDirectory temporaryDirectory("qtc-java-jdtls-XXXXXX");
    const FilePath installation = temporaryDirectory.path();
    const FilePath java = installation / "java";

    const QString launcherName = HostOsInfo::isWindowsHost() ? QString("jdtls.bat")
                                                             : QString("jdtls");
    const FilePath launcher = installation / "bin" / launcherName;
    QVERIFY(launcher.parentDir().ensureWritableDir());
    QVERIFY(launcher.writeFileContents({}));
    QVERIFY(launcher.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

    // An installation that is the launcher alone is the launcher.
    QCOMPARE(jdtlsCommandIn(installation, java).executable, launcher);

    const FilePath launcherJar = installation / "plugins/org.eclipse.equinox.launcher_1.6.jar";
    QVERIFY(launcherJar.parentDir().ensureWritableDir());
    QVERIFY(launcherJar.writeFileContents({}));

    // A jar of an installation that ships no configuration for this machine
    // is one there is no command line for, so the launcher stands.
    QCOMPARE(jdtlsCommandIn(installation, java).executable, launcher);

    for (const QString &name : {QString("config_win"), QString("config_linux"),
                                QString("config_mac")}) {
        QVERIFY((installation / name).ensureWritableDir());
    }

    // With both there, the jar wins: it needs nothing but the JDK.
    const JdtlsCommand jarCommand = jdtlsCommandIn(installation, java);
    QCOMPARE(jarCommand.executable, java);
    QCOMPARE(jarCommand.launcherJar, launcherJar);
    QVERIFY(!jarCommand.arguments.isEmpty());

    // Without a java there is nothing to run the jar with.
    QCOMPARE(jdtlsCommandIn(installation, {}).executable, launcher);

    // And an installation that is neither is no way in at all.
    QVERIFY(jdtlsCommandIn(temporaryDirectory.path() / "nowhere", java).isEmpty());
}

// The Android support of Qt Creator used to write an Eclipse project into the
// directory of the project, which the server reads in place of what this
// plugin tells it.
void JavaClassPathTest::testRemoveLegacyEclipseProjectFiles()
{
    TemporaryDirectory temporaryDirectory("qtc-java-files-XXXXXX");
    const FilePath root = temporaryDirectory.path();
    const FilePath autogenerated = root / ".project";
    const FilePath handWritten = root / ".classpath";
    QVERIFY(autogenerated.writeFileContents(
        "<!--Autogenerated by Qt Creator. Changes to this file will not be taken "
        "into account.-->\n<projectDescription/>\n"));
    QVERIFY(handWritten.writeFileContents("<classpath/>\n"));

    removeLegacyEclipseProjectFiles(root);

    QVERIFY2(!autogenerated.exists(),
             "the Eclipse project Qt Creator wrote is still there, and the server reads it "
             "rather than what this plugin tells it");
    QVERIFY2(handWritten.exists(), "a file the user wrote themselves was taken away");
}

void JavaClassPathTest::testPackageOf_data()
{
    QTest::addColumn<QString>("contents");
    QTest::addColumn<QString>("package");

    QTest::newRow("plain") << "package com.example;\n\nclass A {}\n" << "com.example";
    QTest::newRow("after a comment")
        << "// Copyright\n\npackage com.example.deep;\n" << "com.example.deep";
    QTest::newRow("indented") << "  package com.example ;\n" << "com.example";
    QTest::newRow("none") << "class A {}\n" << QString();
    QTest::newRow("single") << "package example;\n" << "example";
    // The declaration is a statement of its own, so one that a line holds
    // something else in front of is not one.
    QTest::newRow("not at the start of a line") << "class A { /* package a.b; */ }\n" << QString();
}

void JavaClassPathTest::testPackageOf()
{
    QFETCH(QString, contents);
    QFETCH(QString, package);

    QCOMPARE(javaPackageOf(contents), package);
}

void JavaClassPathTest::testSourceRoots()
{
    TemporaryDirectory temporaryDirectory("qtc-java-test-XXXXXX");
    const FilePath root = temporaryDirectory.path();

    const auto write = [](const FilePath &file, const QString &contents) {
        return file.parentDir().ensureWritableDir()
               && file.writeFileContents(contents.toUtf8());
    };

    // The package hierarchy starts at src/main/java, which is what a language
    // server is to be told about rather than the directory of the file.
    const FilePath main = root / "src/main/java/com/example/Main.java";
    QVERIFY(write(main, "package com.example;\n\nclass Main {}\n"));

    // A second file of the same package is the same root, asked for or not.
    QVERIFY(write(root / "src/main/java/com/example/Helper.java", "package com.example;\n"));

    // A file that declares no package is at the root it lies in.
    const FilePath loose = root / "scripts/Loose.java";
    QVERIFY(write(loose, "class Loose {}\n"));

    // One that lies somewhere else than its package says is its own root:
    // nothing else about it can be relied on.
    const FilePath misplaced = root / "elsewhere/Misplaced.java";
    QVERIFY(write(misplaced, "package org.somewhere;\n"));

    const FilePaths roots = javaSourceRoots(
        {main, root / "src/main/java/com/example/Helper.java", loose, misplaced});

    QCOMPARE(roots.size(), 3);
    QVERIFY(roots.contains(root / "src/main/java"));
    QVERIFY(roots.contains(root / "scripts"));
    QVERIFY(roots.contains(root / "elsewhere"));
}

QObject *createJavaClassPathTest()
{
    return new JavaClassPathTest;
}

} // namespace Java::Internal

#include "javaclasspath_test.moc"
