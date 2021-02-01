/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cmakeprojectimporter.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildsystem.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmaketoolmanager.h"

#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDir>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

static Q_LOGGING_CATEGORY(cmInputLog, "qtc.cmake.import", QtWarningMsg);

struct DirectoryData
{
    // Project Stuff:
    QByteArray cmakeBuildType;
    FilePath buildDirectory;
    FilePath cmakeHomeDirectory;

    // Kit Stuff
    FilePath cmakeBinary;
    QByteArray generator;
    QByteArray extraGenerator;
    QByteArray platform;
    QByteArray toolset;
    QByteArray sysroot;
    QtProjectImporter::QtVersionData qt;
    QVector<ToolChainDescription> toolChains;
};

static QStringList scanDirectory(const QString &path, const QString &prefix)
{
    QStringList result;
    qCDebug(cmInputLog) << "Scanning for directories matching" << prefix << "in" << path;

    const QDir base = QDir(path);
    foreach (const QString &dir, base.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString subPath = path + '/' + dir;
        qCDebug(cmInputLog) << "Checking" << subPath;
        if (dir.startsWith(prefix))
            result.append(subPath);
    }
    return result;
}

static QString baseCMakeToolDisplayName(CMakeTool &tool)
{
    CMakeTool::Version version = tool.version();
    return QString("CMake %1.%2.%3").arg(version.major).arg(version.minor).arg(version.patch);
}

static QString uniqueCMakeToolDisplayName(CMakeTool &tool)
{
    QString baseName = baseCMakeToolDisplayName(tool);

    QStringList existingNames;
    for (const CMakeTool *t : CMakeToolManager::cmakeTools())
        existingNames << t->displayName();
    return Utils::makeUniquelyNumbered(baseName, existingNames);
}

// CMakeProjectImporter

CMakeProjectImporter::CMakeProjectImporter(const FilePath &path) : QtProjectImporter(path)
{
    useTemporaryKitAspect(CMakeKitAspect::id(),
                               [this](Kit *k, const QVariantList &vl) { cleanupTemporaryCMake(k, vl); },
                               [this](Kit *k, const QVariantList &vl) { persistTemporaryCMake(k, vl); });

}

QStringList CMakeProjectImporter::importCandidates()
{
    QStringList candidates;

    QFileInfo pfi = projectFilePath().toFileInfo();
    candidates << scanDirectory(pfi.absolutePath(), "build");

    foreach (Kit *k, KitManager::kits()) {
        QFileInfo fi(CMakeBuildConfiguration::shadowBuildDirectory(projectFilePath(), k,
                                                                   QString(), BuildConfiguration::Unknown).toString());
        candidates << scanDirectory(fi.absolutePath(), QString());
    }
    const QStringList finalists = Utils::filteredUnique(candidates);
    qCInfo(cmInputLog) << "import candidates:" << finalists;
    return finalists;
}

static FilePath qmakeFromCMakeCache(const CMakeConfig &config)
{
    // Qt4 way to define things (more convenient for us, so try this first;-)
    FilePath qmake
            = FilePath::fromUtf8(CMakeConfigItem::valueOf(QByteArray("QT_QMAKE_EXECUTABLE"), config));
    qCDebug(cmInputLog) << "QT_QMAKE_EXECUTABLE=" << qmake.toUserOutput();
    if (!qmake.isEmpty())
        return qmake;

    // Check Qt5 settings: oh, the horror!
    const FilePath qtCMakeDir = [config] {
        FilePath tmp = FilePath::fromUtf8(
            CMakeConfigItem::valueOf(QByteArray("Qt5Core_DIR"), config));
        if (tmp.isEmpty()) {
            tmp = FilePath::fromUtf8(
                CMakeConfigItem::valueOf(QByteArray("Qt6Core_DIR"), config));
        }
        return tmp;
    }();
    qCDebug(cmInputLog) << "QtXCore_DIR=" << qtCMakeDir.toUserOutput();
    const FilePath canQtCMakeDir = FilePath::fromString(qtCMakeDir.toFileInfo().canonicalFilePath());
    qCInfo(cmInputLog) << "QtXCore_DIR (canonical)=" << canQtCMakeDir.toUserOutput();
    if (qtCMakeDir.isEmpty())
        return FilePath();
    const FilePath baseQtDir = canQtCMakeDir.parentDir().parentDir().parentDir(); // Up 3 levels...
    qCDebug(cmInputLog) << "BaseQtDir:" << baseQtDir.toUserOutput();

    // Run a CMake project that would do qmake probing
    TemporaryDirectory qtcQMakeProbeDir("qtc-cmake-qmake-probe-XXXXXXXX");

    QFile cmakeListTxt(qtcQMakeProbeDir.path() + "/CMakeLists.txt");
    if (!cmakeListTxt.open(QIODevice::WriteOnly)) {
        return FilePath();
    }
    cmakeListTxt.write(QByteArray(R"(
        cmake_minimum_required(VERSION 3.15)

        project(qmake-probe LANGUAGES NONE)

        # Bypass Qt6's usage of find_dependency, which would require compiler
        # and source code probing, which slows things unnecessarily
        file(WRITE "${CMAKE_SOURCE_DIR}/CMakeFindDependencyMacro.cmake"
        [=[
            macro(find_dependency dep)
            endmacro()
        ]=])
        set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

        find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
        find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

        if (CMAKE_CROSSCOMPILING)
            find_program(qmake_binary
                NAMES qmake qmake.bat
                PATHS "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin"
                NO_DEFAULT_PATH)
            file(WRITE "${CMAKE_SOURCE_DIR}/qmake-location.txt" "${qmake_binary}")
        else()
            file(GENERATE
                OUTPUT "${CMAKE_SOURCE_DIR}/qmake-location.txt"
                CONTENT "$<TARGET_PROPERTY:Qt${QT_VERSION_MAJOR}::qmake,IMPORTED_LOCATION>")
        endif()
    )"));
    cmakeListTxt.close();

    SynchronousProcess cmake;
    cmake.setTimeoutS(5);
    cmake.setFlags(SynchronousProcess::UnixTerminalDisabled);
    Environment env = Environment::systemEnvironment();
    Environment::setupEnglishOutput(&env);
    cmake.setProcessEnvironment(env.toProcessEnvironment());
    cmake.setTimeOutMessageBoxEnabled(false);

    QString cmakeGenerator
            = QString::fromUtf8(CMakeConfigItem::valueOf(QByteArray("CMAKE_GENERATOR"), config));
    FilePath cmakeExecutable
            = FilePath::fromUtf8(CMakeConfigItem::valueOf(QByteArray("CMAKE_COMMAND"), config));
    FilePath cmakeMakeProgram
            = FilePath::fromUtf8(CMakeConfigItem::valueOf(QByteArray("CMAKE_MAKE_PROGRAM"), config));
    FilePath toolchainFile
            = FilePath::fromUtf8(CMakeConfigItem::valueOf(QByteArray("CMAKE_TOOLCHAIN_FILE"), config));
    FilePath hostPath
            = FilePath::fromUtf8(CMakeConfigItem::valueOf(QByteArray("QT_HOST_PATH"), config));

    QStringList args;
    args.push_back("-S");
    args.push_back(qtcQMakeProbeDir.path());
    args.push_back("-B");
    args.push_back(qtcQMakeProbeDir.path() + "/build");
    args.push_back("-G");
    args.push_back(cmakeGenerator);

    if (!cmakeMakeProgram.isEmpty()) {
        args.push_back(QStringLiteral("-DCMAKE_MAKE_PROGRAM=%1").arg(cmakeMakeProgram.toString()));
    }
    if (toolchainFile.isEmpty()) {
        args.push_back(QStringLiteral("-DCMAKE_PREFIX_PATH=%1").arg(baseQtDir.toString()));
    } else {
        args.push_back(QStringLiteral("-DCMAKE_FIND_ROOT_PATH=%1").arg(baseQtDir.toString()));
        args.push_back(QStringLiteral("-DCMAKE_TOOLCHAIN_FILE=%1").arg(toolchainFile.toString()));
    }
    if (!hostPath.isEmpty()) {
        args.push_back(QStringLiteral("-DQT_HOST_PATH=%1").arg(hostPath.toString()));
    }

    qCDebug(cmInputLog) << "CMake probing for qmake path: " << cmakeExecutable.toUserOutput() << args;
    cmake.runBlocking({cmakeExecutable, args});


    QFile qmakeLocationTxt(qtcQMakeProbeDir.path() + "/qmake-location.txt");
    if (!qmakeLocationTxt.open(QIODevice::ReadOnly)) {
        return FilePath();
    }
    FilePath qmakeLocation = FilePath::fromUtf8(qmakeLocationTxt.readLine().data());
    qCDebug(cmInputLog) << "qmake location: " << qmakeLocation.toUserOutput();

    return qmakeLocation;
}

static QVector<ToolChainDescription> extractToolChainsFromCache(const CMakeConfig &config)
{
    QVector<ToolChainDescription> result;
    bool haveCCxxCompiler = false;
    for (const CMakeConfigItem &i : config) {
        if (!i.key.startsWith("CMAKE_") || !i.key.endsWith("_COMPILER"))
            continue;
        const QByteArray language = i.key.mid(6, i.key.count() - 6 - 9); // skip "CMAKE_" and "_COMPILER"
        Id languageId;
        if (language == "CXX") {
            haveCCxxCompiler = true;
            languageId = ProjectExplorer::Constants::CXX_LANGUAGE_ID;
        }
        else  if (language == "C") {
            haveCCxxCompiler = true;
            languageId = ProjectExplorer::Constants::C_LANGUAGE_ID;
        }
        else
            languageId = Id::fromName(language);
        result.append({FilePath::fromUtf8(i.value), languageId});
    }

    if (!haveCCxxCompiler) {
        const QByteArray generator = CMakeConfigItem::valueOf(QByteArray("CMAKE_GENERATOR"), config);
        QString cCompilerName;
        QString cxxCompilerName;
        if (generator.contains("Visual Studio")) {
            cCompilerName = "cl.exe";
            cxxCompilerName = "cl.exe";
        } else if (generator.contains("Xcode")) {
            cCompilerName = "clang";
            cxxCompilerName = "clang++";
        }

        if (!cCompilerName.isEmpty() && !cxxCompilerName.isEmpty()) {
            const FilePath linker = FilePath::fromUtf8(
                CMakeConfigItem::valueOf(QByteArray("CMAKE_LINKER"), config));
            if (!linker.isEmpty()) {
                const FilePath compilerPath = linker.parentDir();
                result.append({compilerPath.pathAppended(cCompilerName),
                               ProjectExplorer::Constants::C_LANGUAGE_ID});
                result.append({compilerPath.pathAppended(cxxCompilerName),
                               ProjectExplorer::Constants::CXX_LANGUAGE_ID});
            }
        }
    }

    return result;
}

QList<void *> CMakeProjectImporter::examineDirectory(const FilePath &importPath,
                                                     QString *warningMessage) const
{
    qCInfo(cmInputLog) << "Examining directory:" << importPath.toUserOutput();
    const FilePath cacheFile = importPath.pathAppended("CMakeCache.txt");

    if (!cacheFile.exists()) {
        qCDebug(cmInputLog) << cacheFile.toUserOutput() << "does not exist, returning.";
        return { };
    }

    QString errorMessage;
    const CMakeConfig config = CMakeBuildSystem::parseCMakeCacheDotTxt(cacheFile, &errorMessage);
    if (config.isEmpty() || !errorMessage.isEmpty()) {
        qCDebug(cmInputLog) << "Failed to read configuration from" << cacheFile << errorMessage;
        return { };
    }

    QByteArrayList buildConfigurationTypes = {CMakeConfigItem::valueOf("CMAKE_BUILD_TYPE", config)};
    if (buildConfigurationTypes.front().isEmpty()) {
        QByteArray buildConfigurationTypesString =
                    CMakeConfigItem::valueOf("CMAKE_CONFIGURATION_TYPES", config);
        if (!buildConfigurationTypesString.isEmpty()) {
            buildConfigurationTypes = buildConfigurationTypesString.split(';');
        }
    }

    QList<void *> result;
    for (auto const &buildType: qAsConst(buildConfigurationTypes)) {
        auto data = std::make_unique<DirectoryData>();

        data->cmakeHomeDirectory = FilePath::fromUserInput(
                                    QString::fromUtf8(
                                      CMakeConfigItem::valueOf("CMAKE_HOME_DIRECTORY", config)))
                                    .canonicalPath();
        const FilePath canonicalProjectDirectory = projectDirectory().canonicalPath();
        if (data->cmakeHomeDirectory != canonicalProjectDirectory) {
            *warningMessage = tr("Unexpected source directory \"%1\", expected \"%2\". "
                                 "This can be correct in some situations, for example when "
                                 "importing a standalone Qt test, but usually this is an error. "
                                 "Import the build anyway?")
                                  .arg(data->cmakeHomeDirectory.toUserOutput(),
                                       canonicalProjectDirectory.toUserOutput());
        }

        data->buildDirectory = importPath;
        data->cmakeBuildType = buildType;

        data->cmakeBinary = FilePath::fromUtf8(CMakeConfigItem::valueOf("CMAKE_COMMAND", config));
        data->generator = CMakeConfigItem::valueOf("CMAKE_GENERATOR", config);
        data->extraGenerator = CMakeConfigItem::valueOf("CMAKE_EXTRA_GENERATOR", config);
        data->platform = CMakeConfigItem::valueOf("CMAKE_GENERATOR_PLATFORM", config);
        data->toolset = CMakeConfigItem::valueOf("CMAKE_GENERATOR_TOOLSET", config);

        data->sysroot = CMakeConfigItem::valueOf("CMAKE_SYSROOT", config);

        // Qt:
        const FilePath qmake = qmakeFromCMakeCache(config);
        if (!qmake.isEmpty())
            data->qt = findOrCreateQtVersion(qmake);

        // ToolChains:
        data->toolChains = extractToolChainsFromCache(config);

        qCInfo(cmInputLog) << "Offering to import" << importPath.toUserOutput();
        result.push_back(static_cast<void *>(data.release()));
    }
    return result;
}

bool CMakeProjectImporter::matchKit(void *directoryData, const Kit *k) const
{
    const DirectoryData *data = static_cast<DirectoryData *>(directoryData);

    CMakeTool *cm = CMakeKitAspect::cmakeTool(k);
    if (!cm || cm->cmakeExecutable() != data->cmakeBinary)
        return false;

    if (CMakeGeneratorKitAspect::generator(k) != QString::fromUtf8(data->generator)
            || CMakeGeneratorKitAspect::extraGenerator(k) != QString::fromUtf8(data->extraGenerator)
            || CMakeGeneratorKitAspect::platform(k) != QString::fromUtf8(data->platform)
            || CMakeGeneratorKitAspect::toolset(k) != QString::fromUtf8(data->toolset))
        return false;

    if (SysRootKitAspect::sysRoot(k) != FilePath::fromUtf8(data->sysroot))
        return false;

    if (data->qt.qt && QtSupport::QtKitAspect::qtVersionId(k) != data->qt.qt->uniqueId())
        return false;

    const QList<Id> allLanguages = ToolChainManager::allLanguages();
    for (const ToolChainDescription &tcd : data->toolChains) {
        if (!Utils::contains(allLanguages, [&tcd](const Id& language) {return language == tcd.language;}))
            continue;
        ToolChain *tc = ToolChainKitAspect::toolChain(k, tcd.language);
        if (!tc
            || !Utils::Environment::systemEnvironment()
                    .isSameExecutable(tc->compilerCommand().toString(),
                                      tcd.compilerPath.toString())) {
            return false;
        }
    }

    qCDebug(cmInputLog) << k->displayName()
                          << "matches directoryData for" << data->buildDirectory.toUserOutput();
    return true;
}

Kit *CMakeProjectImporter::createKit(void *directoryData) const
{
    const DirectoryData *data = static_cast<DirectoryData *>(directoryData);

    return QtProjectImporter::createTemporaryKit(data->qt, [&data, this](Kit *k) {
        const CMakeToolData cmtd = findOrCreateCMakeTool(data->cmakeBinary);
        QTC_ASSERT(cmtd.cmakeTool, return);
        if (cmtd.isTemporary)
            addTemporaryData(CMakeKitAspect::id(), cmtd.cmakeTool->id().toSetting(), k);
        CMakeKitAspect::setCMakeTool(k, cmtd.cmakeTool->id());

        CMakeGeneratorKitAspect::setGenerator(k, QString::fromUtf8(data->generator));
        CMakeGeneratorKitAspect::setExtraGenerator(k, QString::fromUtf8(data->extraGenerator));
        CMakeGeneratorKitAspect::setPlatform(k, QString::fromUtf8(data->platform));
        CMakeGeneratorKitAspect::setToolset(k, QString::fromUtf8(data->toolset));

        SysRootKitAspect::setSysRoot(k, FilePath::fromUtf8(data->sysroot));

        for (const ToolChainDescription &cmtcd : data->toolChains) {
            const ToolChainData tcd = findOrCreateToolChains(cmtcd);
            QTC_ASSERT(!tcd.tcs.isEmpty(), continue);

            if (tcd.areTemporary) {
                for (ToolChain *tc : tcd.tcs)
                    addTemporaryData(ToolChainKitAspect::id(), tc->id(), k);
            }

            ToolChainKitAspect::setToolChain(k, tcd.tcs.at(0));
        }

        qCInfo(cmInputLog) << "Temporary Kit created.";
    });
}

const QList<BuildInfo> CMakeProjectImporter::buildInfoList(void *directoryData) const
{
    auto data = static_cast<const DirectoryData *>(directoryData);

    // create info:
    BuildInfo info = CMakeBuildConfigurationFactory::createBuildInfo(
                CMakeBuildConfigurationFactory::buildTypeFromByteArray(data->cmakeBuildType));
    info.buildDirectory = data->buildDirectory;
    info.displayName = info.typeName;

    QVariantMap config;
    config.insert(Constants::CMAKE_HOME_DIR, data->cmakeHomeDirectory.toString());
    info.extraInfo = config;

    qCDebug(cmInputLog) << "BuildInfo configured.";
    return {info};
}

CMakeProjectImporter::CMakeToolData
CMakeProjectImporter::findOrCreateCMakeTool(const FilePath &cmakeToolPath) const
{
    CMakeToolData result;
    result.cmakeTool = CMakeToolManager::findByCommand(cmakeToolPath);
    if (!result.cmakeTool) {
        qCDebug(cmInputLog) << "Creating temporary CMakeTool for" << cmakeToolPath.toUserOutput();

        UpdateGuard guard(*this);

        auto newTool = std::make_unique<CMakeTool>(CMakeTool::ManualDetection, CMakeTool::createId());
        newTool->setFilePath(cmakeToolPath);
        newTool->setDisplayName(uniqueCMakeToolDisplayName(*newTool));

        result.cmakeTool = newTool.get();
        result.isTemporary = true;
        CMakeToolManager::registerCMakeTool(std::move(newTool));
    }
    return result;
}

void CMakeProjectImporter::deleteDirectoryData(void *directoryData) const
{
    delete static_cast<DirectoryData *>(directoryData);
}

void CMakeProjectImporter::cleanupTemporaryCMake(Kit *k, const QVariantList &vl)
{
    if (vl.isEmpty())
        return; // No temporary CMake
    QTC_ASSERT(vl.count() == 1, return);
    CMakeKitAspect::setCMakeTool(k, Id()); // Always mark Kit as not using this Qt
    CMakeToolManager::deregisterCMakeTool(Id::fromSetting(vl.at(0)));
    qCDebug(cmInputLog) << "Temporary CMake tool cleaned up.";
}

void CMakeProjectImporter::persistTemporaryCMake(Kit *k, const QVariantList &vl)
{
    if (vl.isEmpty())
        return; // No temporary CMake
    QTC_ASSERT(vl.count() == 1, return);
    const QVariant data = vl.at(0);
    CMakeTool *tmpCmake = CMakeToolManager::findById(Id::fromSetting(data));
    CMakeTool *actualCmake = CMakeKitAspect::cmakeTool(k);

    // User changed Kit away from temporary CMake that was set up:
    if (tmpCmake && actualCmake != tmpCmake)
        CMakeToolManager::deregisterCMakeTool(tmpCmake->id());

    qCDebug(cmInputLog) << "Temporary CMake tool made persistent.";
}

} // namespace Internal
} // namespace CMakeProjectManager

#ifdef WITH_TESTS

#include "cmakeprojectplugin.h"

#include <QTest>

namespace CMakeProjectManager {
namespace Internal {

void CMakeProjectPlugin::testCMakeProjectImporterQt_data()
{
    QTest::addColumn<QStringList>("cache");
    QTest::addColumn<QString>("expectedQmake");

    QTest::newRow("Empty input")
            << QStringList() << QString();

    QTest::newRow("Qt4")
            << QStringList({QString::fromLatin1("QT_QMAKE_EXECUTABLE=/usr/bin/xxx/qmake")})
            << "/usr/bin/xxx/qmake";

    // Everything else will require Qt installations!
}

void CMakeProjectPlugin::testCMakeProjectImporterQt()
{
    QFETCH(QStringList, cache);
    QFETCH(QString, expectedQmake);

    CMakeConfig config;
    foreach (const QString &c, cache) {
        const int pos = c.indexOf('=');
        Q_ASSERT(pos > 0);
        const QString key = c.left(pos);
        const QString value = c.mid(pos + 1);
        config.append(CMakeConfigItem(key.toUtf8(), value.toUtf8()));
    }

    FilePath realQmake = qmakeFromCMakeCache(config);
    QCOMPARE(realQmake.toString(), expectedQmake);
}
void CMakeProjectPlugin::testCMakeProjectImporterToolChain_data()
{
    QTest::addColumn<QStringList>("cache");
    QTest::addColumn<QByteArrayList>("expectedLanguages");
    QTest::addColumn<QStringList>("expectedToolChains");

    QTest::newRow("Empty input")
            << QStringList() << QByteArrayList() << QStringList();

    QTest::newRow("Unrelated input")
            << QStringList("CMAKE_SOMETHING_ELSE=/tmp") << QByteArrayList() << QStringList();
    QTest::newRow("CXX compiler")
            << QStringList({"CMAKE_CXX_COMPILER=/usr/bin/g++"})
            << QByteArrayList({"Cxx"})
            << QStringList({"/usr/bin/g++"});
    QTest::newRow("CXX compiler, C compiler")
            << QStringList({"CMAKE_CXX_COMPILER=/usr/bin/g++", "CMAKE_C_COMPILER=/usr/bin/clang"})
            << QByteArrayList({"Cxx", "C"})
            << QStringList({"/usr/bin/g++", "/usr/bin/clang"});
    QTest::newRow("CXX compiler, C compiler, strange compiler")
            << QStringList({"CMAKE_CXX_COMPILER=/usr/bin/g++",
                             "CMAKE_C_COMPILER=/usr/bin/clang",
                             "CMAKE_STRANGE_LANGUAGE_COMPILER=/tmp/strange/compiler"})
            << QByteArrayList({"Cxx", "C", "STRANGE_LANGUAGE"})
            << QStringList({"/usr/bin/g++", "/usr/bin/clang", "/tmp/strange/compiler"});
    QTest::newRow("CXX compiler, C compiler, strange compiler (with junk)")
            << QStringList({"FOO=test",
                             "CMAKE_CXX_COMPILER=/usr/bin/g++",
                             "CMAKE_BUILD_TYPE=debug",
                             "CMAKE_C_COMPILER=/usr/bin/clang",
                             "SOMETHING_COMPILER=/usr/bin/something",
                             "CMAKE_STRANGE_LANGUAGE_COMPILER=/tmp/strange/compiler",
                             "BAR=more test"})
            << QByteArrayList({"Cxx", "C", "STRANGE_LANGUAGE"})
            << QStringList({"/usr/bin/g++", "/usr/bin/clang", "/tmp/strange/compiler"});
}

void CMakeProjectPlugin::testCMakeProjectImporterToolChain()
{
    QFETCH(QStringList, cache);
    QFETCH(QByteArrayList, expectedLanguages);
    QFETCH(QStringList, expectedToolChains);

    QCOMPARE(expectedLanguages.count(), expectedToolChains.count());

    CMakeConfig config;
    foreach (const QString &c, cache) {
        const int pos = c.indexOf('=');
        Q_ASSERT(pos > 0);
        const QString key = c.left(pos);
        const QString value = c.mid(pos + 1);
        config.append(CMakeConfigItem(key.toUtf8(), value.toUtf8()));
    }

    const QVector<ToolChainDescription> tcs = extractToolChainsFromCache(config);
    QCOMPARE(tcs.count(), expectedLanguages.count());
    for (int i = 0; i < tcs.count(); ++i) {
        QCOMPARE(tcs.at(i).language, expectedLanguages.at(i));
        QCOMPARE(tcs.at(i).compilerPath.toString(), expectedToolChains.at(i));
    }
}

} // namespace Internal
} // namespace CMakeProjectManager

#endif
