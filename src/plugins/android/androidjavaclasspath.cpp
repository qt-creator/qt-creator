// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidjavaclasspath.h"

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidutils.h"

#include <java/javaclasspath.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>

#ifdef WITH_TESTS
#include <utils/temporarydirectory.h>

#include <QDir>
#include <QScopeGuard>
#include <QTest>
#endif

using namespace ProjectExplorer;
using namespace Utils;

namespace Android::Internal {

// What the package source directory of a product holds: the directory its
// packages start at, which is "src/main/java" or the "src" of an application
// that predates it, and the archives that lie beside it. A product that names
// no directory of its own holds nothing - a library target of a multi-target
// project usually names none, and a relative path would name wherever Creator
// happens to have been started from.
static Java::JavaClassPath packageSourceClassPath(const FilePath &packageSourceDir)
{
    if (packageSourceDir.isEmpty())
        return {};

    FilePath sourceDir = packageSourceDir.pathAppended("src/main/java");
    if (!sourceDir.exists()) {
        sourceDir = packageSourceDir.pathAppended("src");
        if (!sourceDir.exists())
            return {};
    }

    Java::JavaClassPath classPath;
    classPath.sourceDirectories << sourceDir;
    classPath.libraries << packageSourceDir.pathAppended("libs").dirEntries(
        {{"*.jar"}, DirFilterFlag::Files});
    return classPath;
}

// An Android application compiles its Java against the platform of the SDK it
// targets and against the Java that Qt ships for Android, next to whatever the
// package source directories of its products hold.
static std::optional<Java::JavaClassPath> androidClassPath(BuildConfiguration *bc)
{
    Kit *kit = bc->kit();
    if (RunDeviceTypeKitAspect::deviceTypeId(kit) != Constants::ANDROID_DEVICE_TYPE)
        return {};

    BuildSystem *bs = bc->buildSystem();
    ProjectNode *projectNode = bc->project()->findNodeForBuildKey(bc->activeBuildKey());
    if (!bs || !projectNode)
        return {};

    QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(kit);
    if (!version)
        return {};

    const FilePath androidJar = AndroidConfig::sdkLocation()
                                / QString("platforms/%1/android.jar").arg(buildTargetSDK(bc));

    Java::JavaClassPath classPath;
    classPath.libraries << androidJar;

    projectNode->managingProject()->forEachProjectNode([bs, &classPath](const ProjectNode *node) {
        if (!node->isProduct())
            return;

        const FilePath packageSourceDir = FilePath::fromVariant(
            bs->extraData(node->buildKey(), Constants::AndroidPackageSourceDir));
        const Java::JavaClassPath productClassPath = packageSourceClassPath(packageSourceDir);
        if (productClassPath.isEmpty())
            return;

        classPath.sourceDirectories << productClassPath.sourceDirectories;
        classPath.libraries << productClassPath.libraries;

        const QStringList classPaths
            = bs->extraData(node->buildKey(), Constants::AndroidClassPaths).toStringList();
        for (const QString &path : classPaths)
            classPath.libraries << FilePath::fromString(path);
    });

    classPath.sourceDirectories << version->prefix().pathAppended("src/android/java/src");

    // Two products may well share a package source directory, and one entry
    // twice is one entry.
    classPath.sourceDirectories = Utils::filteredUnique(classPath.sourceDirectories);
    classPath.libraries = Utils::filteredUnique(classPath.libraries);

    return classPath;
}

void setupAndroidJavaClassPath()
{
    Java::javaClassPathProviders().append(&androidClassPath);
}

} // namespace Android::Internal

#ifdef WITH_TESTS

namespace Android::Internal {

class AndroidJavaClassPathTest final : public QObject
{
    Q_OBJECT

private slots:
    void testPackageSourceClassPath()
    {
        TemporaryDirectory temporaryDirectory("qtc-android-classpath-XXXXXX");
        const FilePath root = temporaryDirectory.path();

        const FilePath packageSourceDir = root / "android";
        QVERIFY((packageSourceDir / "src/main/java").ensureWritableDir());
        QVERIFY((packageSourceDir / "libs").ensureWritableDir());
        QVERIFY((packageSourceDir / "libs/support.jar").writeFileContents({}));
        QVERIFY((packageSourceDir / "libs/notes.txt").writeFileContents({}));

        const Java::JavaClassPath classPath = packageSourceClassPath(packageSourceDir);
        QCOMPARE(classPath.sourceDirectories, FilePaths{packageSourceDir / "src/main/java"});
        QCOMPARE(classPath.libraries, FilePaths{packageSourceDir / "libs/support.jar"});

        // The layout of an application that predates src/main/java.
        const FilePath plain = root / "plain";
        QVERIFY((plain / "src").ensureWritableDir());
        QCOMPARE(packageSourceClassPath(plain).sourceDirectories, FilePaths{plain / "src"});

        // A directory that holds neither holds no Java of the product.
        QVERIFY(packageSourceClassPath(root / "empty").isEmpty());

        // A product that names no package source directory is left out rather
        // than resolved against whatever Qt Creator was started from, which
        // is what a bare "src/main/java" would name.
        const QString previous = QDir::currentPath();
        QVERIFY(QDir::setCurrent(packageSourceDir.path()));
        const QScopeGuard restore([previous] { QDir::setCurrent(previous); });
        QVERIFY2(packageSourceClassPath({}).isEmpty(),
                 "the working directory of Qt Creator was taken for the Java of a product");
    }
};

QObject *createAndroidJavaClassPathTest()
{
    return new AndroidJavaClassPathTest;
}

} // namespace Android::Internal

#include "androidjavaclasspath.moc"

#endif // WITH_TESTS
