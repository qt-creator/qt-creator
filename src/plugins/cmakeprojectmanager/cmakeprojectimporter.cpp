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
#include "cmaketoolmanager.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace {

static Q_LOGGING_CATEGORY(cmInputLog, "qtc.cmake.import", QtWarningMsg);

struct DirectoryData
{
    // Project Stuff:
    QByteArray cmakeBuildType;
    Utils::FilePath buildDirectory;

    // Kit Stuff
    Utils::FilePath cmakeBinary;
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

} // namespace

namespace CMakeProjectManager {
namespace Internal {

CMakeProjectImporter::CMakeProjectImporter(const Utils::FilePath &path) : QtProjectImporter(path)
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

static Utils::FilePath qmakeFromCMakeCache(const CMakeConfig &config)
{
    // Qt4 way to define things (more convenient for us, so try this first;-)
    Utils::FilePath qmake
            = Utils::FilePath::fromUtf8(CMakeConfigItem::valueOf(QByteArray("QT_QMAKE_EXECUTABLE"), config));
    qCDebug(cmInputLog) << "QT_QMAKE_EXECUTABLE=" << qmake.toUserOutput();
    if (!qmake.isEmpty())
        return qmake;

    // Check Qt5 settings: oh, the horror!
    const Utils::FilePath qtCMakeDir = [config]() {
        Utils::FilePath tmp = Utils::FilePath::fromUtf8(
            CMakeConfigItem::valueOf(QByteArray("Qt5Core_DIR"), config));
        if (tmp.isEmpty()) {
            tmp = Utils::FilePath::fromUtf8(
                CMakeConfigItem::valueOf(QByteArray("Qt6Core_DIR"), config));
        }
        return tmp;
    }();
    qCDebug(cmInputLog) << "QtXCore_DIR=" << qtCMakeDir.toUserOutput();
    const Utils::FilePath canQtCMakeDir = Utils::FilePath::fromString(qtCMakeDir.toFileInfo().canonicalFilePath());
    qCInfo(cmInputLog) << "QtXCore_DIR (canonical)=" << canQtCMakeDir.toUserOutput();
    if (qtCMakeDir.isEmpty())
        return Utils::FilePath();
    const Utils::FilePath baseQtDir = canQtCMakeDir.parentDir().parentDir().parentDir(); // Up 3 levels...
    qCDebug(cmInputLog) << "BaseQtDir:" << baseQtDir.toUserOutput();

    // "Parse" Qt5Core/Qt5CoreConfigExtras.cmake:
    {
        // This assumes that roughly this kind of data is found in
        // inside Qt5Core/Qt5CoreConfigExtras.cmake:
        //
        //    if (NOT TARGET Qt5::qmake)
        //        add_executable(Qt5::qmake IMPORTED)
        //        set(imported_location "${_qt5Core_install_prefix}/bin/qmake")
        //        _qt5_Core_check_file_exists(${imported_location})
        //        set_target_properties(Qt5::qmake PROPERTIES
        //            IMPORTED_LOCATION ${imported_location}
        //        )
        //    endif()

        QFile extras(qtCMakeDir.toString() + "/Qt5CoreConfigExtras.cmake");
        if (!extras.open(QIODevice::ReadOnly)) {
            extras.setFileName(qtCMakeDir.toString() + "/Qt6CoreConfigExtras.cmake");
            if (!extras.open(QIODevice::ReadOnly))
                return Utils::FilePath();
        }

        QByteArray data;
        bool inQmakeSection = false;
        // Read in 4k chunks, lines longer than that are going to be ignored
        while (!extras.atEnd()) {
            data = extras.read(4 * 1024 - data.count());
            int startPos = 0;
            forever {
                const int pos = data.indexOf('\n', startPos);
                if (pos < 0) {
                    data = data.mid(startPos);
                    break;
                }

                QByteArray line = data.mid(startPos, pos - startPos);
                const QByteArray origLine = line;

                startPos = pos + 1;

                line.replace("(", " ( ");
                line.replace(")", " ) ");
                line = line.simplified();

                if (line == "if ( NOT TARGET Qt5::qmake )"
                        || line == "if ( NOT TARGET Qt6::qmake )")
                    inQmakeSection = true;

                if (!inQmakeSection)
                    continue;

                const QByteArray set = "set ( imported_location ";
                if (line.startsWith(set)) {
                    const int sp = origLine.indexOf('}');
                    const int ep = origLine.lastIndexOf('"');

                    QTC_ASSERT(sp > 0, return Utils::FilePath());
                    QTC_ASSERT(ep > sp + 2, return Utils::FilePath());
                    QTC_ASSERT(ep < origLine.count(), return Utils::FilePath());

                    // Eat the leading "}/" and trailing "
                    const QByteArray locationPart =  origLine.mid(sp + 2, ep - 2 - sp);
                    return baseQtDir.pathAppended(QString::fromUtf8(locationPart));
                }
            }
        }
    }

    // Now try to make sense of .../Qt5CoreConfig.cmake:
    return Utils::FilePath();
}

static QVector<ToolChainDescription> extractToolChainsFromCache(const CMakeConfig &config)
{
    QVector<ToolChainDescription> result;
    for (const CMakeConfigItem &i : config) {
        if (!i.key.startsWith("CMAKE_") || !i.key.endsWith("_COMPILER"))
            continue;
        const QByteArray language = i.key.mid(6, i.key.count() - 6 - 9); // skip "CMAKE_" and "_COMPILER"
        Utils::Id languageId;
        if (language == "CXX")
            languageId = ProjectExplorer::Constants::CXX_LANGUAGE_ID;
        else  if (language == "C")
            languageId = ProjectExplorer::Constants::C_LANGUAGE_ID;
        else
            languageId = Utils::Id::fromName(language);
        result.append({Utils::FilePath::fromUtf8(i.value), languageId});
    }
    return result;
}

QList<void *> CMakeProjectImporter::examineDirectory(const Utils::FilePath &importPath) const
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
    const auto homeDir
            = Utils::FilePath::fromUserInput(QString::fromUtf8(CMakeConfigItem::valueOf("CMAKE_HOME_DIRECTORY", config)));
    const Utils::FilePath canonicalProjectDirectory = projectDirectory().canonicalPath();
    if (homeDir != canonicalProjectDirectory) {
        qCDebug(cmInputLog) << "Wrong source directory:" << homeDir.toUserOutput()
                              << "expected:" << canonicalProjectDirectory.toUserOutput();
        return { };
    }

    auto data = std::make_unique<DirectoryData>();
    data->buildDirectory = importPath;
    data->cmakeBuildType = CMakeConfigItem::valueOf("CMAKE_BUILD_TYPE", config);

    data->cmakeBinary
            = Utils::FilePath::fromUtf8(CMakeConfigItem::valueOf("CMAKE_COMMAND", config));
    data->generator = CMakeConfigItem::valueOf("CMAKE_GENERATOR", config);
    data->extraGenerator = CMakeConfigItem::valueOf("CMAKE_EXTRA_GENERATOR", config);
    data->platform = CMakeConfigItem::valueOf("CMAKE_GENERATOR_PLATFORM", config);
    data->toolset = CMakeConfigItem::valueOf("CMAKE_GENERATOR_TOOLSET", config);

    data->sysroot = CMakeConfigItem::valueOf("CMAKE_SYSROOT", config);

    // Qt:
    const Utils::FilePath qmake = qmakeFromCMakeCache(config);
    if (!qmake.isEmpty())
        data->qt = findOrCreateQtVersion(qmake);

    // ToolChains:
    data->toolChains = extractToolChainsFromCache(config);

    qCInfo(cmInputLog) << "Offering to import" << importPath.toUserOutput();
    return {static_cast<void *>(data.release())};
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

    if (SysRootKitAspect::sysRoot(k) != Utils::FilePath::fromUtf8(data->sysroot))
        return false;

    if (data->qt.qt && QtSupport::QtKitAspect::qtVersionId(k) != data->qt.qt->uniqueId())
        return false;

    for (const ToolChainDescription &tcd : data->toolChains) {
        ToolChain *tc = ToolChainKitAspect::toolChain(k, tcd.language);
        if (!tc || tc->compilerCommand() != tcd.compilerPath)
            return false;
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

        CMakeGeneratorKitAspect::setGenerator(k, QString::fromUtf8(data->generator));
        CMakeGeneratorKitAspect::setExtraGenerator(k, QString::fromUtf8(data->extraGenerator));
        CMakeGeneratorKitAspect::setPlatform(k, QString::fromUtf8(data->platform));
        CMakeGeneratorKitAspect::setToolset(k, QString::fromUtf8(data->toolset));

        SysRootKitAspect::setSysRoot(k, Utils::FilePath::fromUtf8(data->sysroot));

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

    qCDebug(cmInputLog) << "BuildInfo configured.";
    return {info};
}

CMakeProjectImporter::CMakeToolData
CMakeProjectImporter::findOrCreateCMakeTool(const Utils::FilePath &cmakeToolPath) const
{
    CMakeToolData result;
    result.cmakeTool = CMakeToolManager::findByCommand(cmakeToolPath);
    if (!result.cmakeTool) {
        qCDebug(cmInputLog) << "Creating temporary CMakeTool for" << cmakeToolPath.toUserOutput();
        result.cmakeTool = new CMakeTool(CMakeTool::ManualDetection, CMakeTool::createId());
        result.isTemporary = true;
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
    CMakeKitAspect::setCMakeTool(k, Utils::Id()); // Always mark Kit as not using this Qt
    CMakeToolManager::deregisterCMakeTool(Utils::Id::fromSetting(vl.at(0)));
    qCDebug(cmInputLog) << "Temporary CMake tool cleaned up.";
}

void CMakeProjectImporter::persistTemporaryCMake(Kit *k, const QVariantList &vl)
{
    if (vl.isEmpty())
        return; // No temporary CMake
    QTC_ASSERT(vl.count() == 1, return);
    const QVariant data = vl.at(0);
    CMakeTool *tmpCmake = CMakeToolManager::findById(Utils::Id::fromSetting(data));
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

    Utils::FilePath realQmake = qmakeFromCMakeCache(config);
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
