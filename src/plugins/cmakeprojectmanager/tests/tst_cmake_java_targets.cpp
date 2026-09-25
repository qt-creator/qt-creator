// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTemporaryDir>
#include <QtTest>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>

#include "../javatargets.h"

using namespace CMakeProjectManager::Internal;
using namespace Utils;

static FilePath javaTargetsFile(const FilePath &buildDirectory)
{
    return buildDirectory / ProjectExplorer::Constants::PROJECT_QTC_DIR / "java-targets.json";
}

static bool write(const FilePath &file, const QByteArray &contents)
{
    if (!file.parentDir().ensureWritableDir())
        return false;
    return bool(file.writeFileContents(contents));
}

// What cmake-helper/java-targets.cmake writes for a project with one add_jar()
// target: a path of either tree relative to the top of that tree, the way the
// file API reports its own.
static QByteArray helperOutput()
{
    return QByteArray(R"({
  "targets": [
    {
      "name": "hello",
      "jar": "hello.jar",
      "sourceDirectory": ".",
      "sources": ["Hello.java", "sub/Other.java"]
    }
  ]
}
)");
}

class tst_CMakeJavaTargets : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void readsWhatTheHelperWrote();
    void readsNothingWithoutAFile();
    void readsNothingFromAnEmptyFile();
    void readsNothingFromBrokenJson();
    void dropsEntriesWithoutANameOrAJar();
    void keepsThePathsOnTheBuildDevice();
    void readsAnAddJarProject();
    void readsASourceWithANonAsciiPath();

private:
    QTemporaryDir m_temporaryDir;
    FilePath m_directory;
};

// A path of the build directory is on the device the build runs on, which the
// test has to be able to read from for readJavaTargets() to get anywhere. The
// files are local ones under another scheme, which is enough to tell a path
// that kept the device from one that lost it.
void tst_CMakeJavaTargets::initTestCase()
{
    QVERIFY(m_temporaryDir.isValid());
    m_directory = FilePath::fromString(m_temporaryDir.path()).canonicalPath();

    DeviceFileHooks hooks;
    hooks.fileAccess = [](const FilePath &) -> Result<DeviceFileAccessPtr> {
        return DesktopDeviceFileAccess::instance();
    };
    hooks.osType = [](const FilePath &) { return HostOsInfo::hostOs(); };
    hooks.isSameDevice = [](const FilePath &left, const FilePath &right) {
        return left.scheme() == right.scheme() && left.host() == right.host();
    };
    DeviceFileHooks::setupDeviceFileHooks(hooks);
}

void tst_CMakeJavaTargets::init()
{
    const FilePath file = javaTargetsFile(m_directory);
    if (file.exists())
        QVERIFY(file.removeFile());
}

// What the helper wrote is read against the directories this side knows the
// trees under. Those need not be the ones CMake was handed: on Windows it
// configures the project through a junction, and a file under that name is
// not a file of the project.
void tst_CMakeJavaTargets::readsWhatTheHelperWrote()
{
    const FilePath source = m_directory / "source";
    const FilePath build = m_directory / "build";
    QVERIFY(write(javaTargetsFile(build), helperOutput()));

    const QList<CMakeJavaTarget> targets = readJavaTargets(source, build);
    QCOMPARE(targets.size(), 1);
    QCOMPARE(targets.first().name, QString("hello"));
    QCOMPARE(targets.first().jar, build / "hello.jar");
    QCOMPARE(targets.first().sourceDirectory, source);
    QCOMPARE(targets.first().sources,
             FilePaths({source / "Hello.java", source / "sub/Other.java"}));
}

void tst_CMakeJavaTargets::readsNothingWithoutAFile()
{
    QVERIFY(!javaTargetsFile(m_directory).exists());
    QVERIFY(readJavaTargets(m_directory, m_directory).isEmpty());
}

void tst_CMakeJavaTargets::readsNothingFromAnEmptyFile()
{
    QVERIFY(write(javaTargetsFile(m_directory), {}));
    QVERIFY(readJavaTargets(m_directory, m_directory).isEmpty());
}

void tst_CMakeJavaTargets::readsNothingFromBrokenJson()
{
    QVERIFY(write(javaTargetsFile(m_directory), R"({"targets": [{"name": )"));
    QVERIFY(readJavaTargets(m_directory, m_directory).isEmpty());
}

// Neither of the two is something a run configuration or the project tree
// could be built on, and the quoting of the helper is hand-written.
void tst_CMakeJavaTargets::dropsEntriesWithoutANameOrAJar()
{
    QVERIFY(write(javaTargetsFile(m_directory), R"({
  "targets": [
    { "name": "", "jar": "/b/nameless.jar" },
    { "name": "jarless", "jar": "" },
    { "name": "complete", "jar": "/b/complete.jar" }
  ]
}
)"));

    const QList<CMakeJavaTarget> targets = readJavaTargets(m_directory, m_directory);
    QCOMPARE(targets.size(), 1);
    QCOMPARE(targets.first().name, QString("complete"));
}

// CMake knows nothing of devices and writes plain paths. A build that runs on
// another machine than Qt Creator has none of them reachable as they stand.
void tst_CMakeJavaTargets::keepsThePathsOnTheBuildDevice()
{
    const FilePath buildDirectory
        = FilePath::fromParts(u"device", u"build-host", (m_directory / "build").path());
    const FilePath sourceDirectory
        = FilePath::fromParts(u"device", u"build-host", (m_directory / "source").path());
    QVERIFY(write(javaTargetsFile(m_directory / "build"), helperOutput()));

    const QList<CMakeJavaTarget> targets = readJavaTargets(sourceDirectory, buildDirectory);
    QCOMPARE(targets.size(), 1);
    for (const FilePath &path : QList<FilePath>{targets.first().jar,
                                                targets.first().sourceDirectory,
                                                targets.first().sources.first()}) {
        QCOMPARE(path.scheme(), QString("device"));
        QCOMPARE(path.host(), QString("build-host"));
    }
}

// A configure of a real project is the only thing that can say that the
// properties and the java_sources format UseJava leaves behind are still the
// ones being read, so a machine without the tools for one has nothing to say.
static bool canConfigureAnAddJarProject()
{
    const Environment env = Environment::systemEnvironment();
    if (env.searchInPath("cmake").isEmpty())
        return false;

    const FilePath javac = env.searchInPath("javac");
    if (javac.isEmpty())
        return false;
    // The javac of macOS is a stub that fails where no JDK is installed.
    Process version;
    version.setCommand({javac, {"-version"}});
    version.runBlocking();
    return version.result() == ProcessResult::FinishedWithSuccess;
}

// Configures a project with one add_jar() target that compiles \a sourceFile,
// spelled the way the project spells it, and returns what the helper recorded.
static Result<QList<CMakeJavaTarget>> configureAddJarProject(const FilePath &source,
                                                             const FilePath &build,
                                                             const QString &sourceFile)
{
    const FilePath java = source / sourceFile;
    if (!java.parentDir().ensureWritableDir())
        return ResultError(QString("Cannot create \"%1\"").arg(java.parentDir().toUserOutput()));
    if (const Result<qint64> written = (source / "CMakeLists.txt")
            .writeFileContents(QString("cmake_minimum_required(VERSION 3.19)\n"
                                       "project(jars LANGUAGES NONE)\n"
                                       "find_package(Java REQUIRED COMPONENTS Development)\n"
                                       "include(UseJava)\n"
                                       "add_jar(hello SOURCES %1)\n")
                                   .arg(sourceFile)
                                   .toUtf8());
        !written) {
        return ResultError(written.error());
    }
    if (const Result<qint64> written = java.writeFileContents(
            "public class Hello {\n"
            "    public static void main(String[] arguments) {}\n"
            "}\n");
        !written) {
        return ResultError(written.error());
    }

    Process configure;
    configure.setCommand({Environment::systemEnvironment().searchInPath("cmake"),
                          {"-S",
                           source.nativePath(),
                           "-B",
                           build.nativePath(),
                           "-DCMAKE_PROJECT_INCLUDE_BEFORE=" + QString(JAVA_TARGETS_CMAKE)}});
    // A configure starts java and javac to read their versions off them, and
    // looks for a toolchain besides; the ten seconds a blocking run waits by
    // default are not something a loaded machine owes it. A run that was cut
    // short says nothing through its exit code, which is what the result is
    // read for.
    configure.runBlocking(std::chrono::minutes(2));
    if (configure.result() != ProcessResult::FinishedWithSuccess)
        return ResultError(configure.allOutput());

    return readJavaTargets(source, build);
}

void tst_CMakeJavaTargets::readsAnAddJarProject()
{
    if (!canConfigureAnAddJarProject())
        QSKIP("No cmake or JDK on this machine");

    const FilePath source = m_directory / "source";
    const FilePath build = m_directory / "build";
    const Result<QList<CMakeJavaTarget>> targets
        = configureAddJarProject(source, build, "Hello.java");
    if (!targets)
        QFAIL(qPrintable(targets.error()));

    QCOMPARE(targets->size(), 1);
    QCOMPARE(targets->first().name, QString("hello"));
    QCOMPARE(targets->first().jar.fileName(), QString("hello.jar"));
    QCOMPARE(targets->first().sourceDirectory, source);
    QCOMPARE(targets->first().sources, FilePaths({source / "Hello.java"}));
    // The helper writes a path of either tree relative to the top of it, so
    // that the IDE resolves it against the directory it knows that tree
    // under, and not under the name CMake was handed - on Windows the two
    // differ, as the project is configured through a junction.
    const Result<QByteArray> written = javaTargetsFile(build).fileContents();
    QVERIFY(written);
    QVERIFY(!written->contains(source.path().toUtf8()));
    QVERIFY(!written->contains(build.path().toUtf8()));
}

// add_jar() writes java_sources with configure_file(), which makes it UTF-8.
// Read as anything else, a source whose path holds a character outside ASCII
// comes apart at that character, and the file of the project is replaced in
// the tree by two that do not exist.
void tst_CMakeJavaTargets::readsASourceWithANonAsciiPath()
{
    if (!canConfigureAnAddJarProject())
        QSKIP("No cmake or JDK on this machine");

    // "Juergen" with an u-umlaut, in the UTF-8 that configure_file() writes.
    const QString subdirectory = QString::fromUtf8("J\xc3\xbc" "rgen");
    const FilePath source = m_directory / "non-ascii-source";
    const Result<QList<CMakeJavaTarget>> targets
        = configureAddJarProject(source,
                                 m_directory / "non-ascii-build",
                                 subdirectory + "/Hello.java");
    if (!targets)
        QFAIL(qPrintable(targets.error()));

    QCOMPARE(targets->size(), 1);
    QCOMPARE(targets->first().sources, FilePaths({source / subdirectory / "Hello.java"}));
}

QTEST_GUILESS_MAIN(tst_CMakeJavaTargets)

#include "tst_cmake_java_targets.moc"
