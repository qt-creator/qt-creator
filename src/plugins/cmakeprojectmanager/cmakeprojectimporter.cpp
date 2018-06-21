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

#include "builddirmanager.h"
#include "cmakebuildconfiguration.h"
#include "cmakebuildinfo.h"
#include "cmakekitinformation.h"
#include "cmaketoolmanager.h"

#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace QtSupport;

namespace {

Q_LOGGING_CATEGORY(cmInputLog, "qtc.cmake.import");

struct CMakeToolChainData
{
    QByteArray languageId;
    Utils::FileName compilerPath;
    Core::Id mapLanguageIdToQtC() const
    {
        const QByteArray li = languageId.toLower();
        if (li == "cxx")
            return ProjectExplorer::Constants::CXX_LANGUAGE_ID;
        else if (li == "c")
            return ProjectExplorer::Constants::C_LANGUAGE_ID;
        else
            return Core::Id::fromName(languageId);
    }
};

struct DirectoryData
{
    // Project Stuff:
    QByteArray cmakeBuildType;
    Utils::FileName buildDirectory;

    // Kit Stuff
    Utils::FileName cmakeBinary;
    QByteArray generator;
    QByteArray extraGenerator;
    QByteArray platform;
    QByteArray toolset;
    QByteArray sysroot;
    QtProjectImporter::QtVersionData qt;
    QVector<CMakeToolChainData> toolChains;
};

static QStringList scanDirectory(const QString &path, const QString &prefix)
{
    QStringList result;
    qCDebug(cmInputLog()) << "Scanning for directories matching" << prefix << "in" << path;

    const QDir base = QDir(path);
    foreach (const QString &dir, base.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString subPath = path + '/' + dir;
        qCDebug(cmInputLog()) << "Checking" << subPath;
        if (dir.startsWith(prefix))
            result.append(subPath);
    }
    return result;
}

} // namespace

namespace CMakeProjectManager {
namespace Internal {

CMakeProjectImporter::CMakeProjectImporter(const Utils::FileName &path) : QtProjectImporter(path)
{
    useTemporaryKitInformation(CMakeKitInformation::id(),
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
    qCInfo(cmInputLog()) << "import candidates:" << finalists;
    return finalists;
}

static Utils::FileName qmakeFromCMakeCache(const CMakeConfig &config)
{
    // Qt4 way to define things (more convenient for us, so try this first;-)
    Utils::FileName qmake
            = Utils::FileName::fromUtf8(CMakeConfigItem::valueOf(QByteArray("QT_QMAKE_EXECUTABLE"), config));
    qCDebug(cmInputLog()) << "QT_QMAKE_EXECUTABLE=" << qmake.toUserOutput();
    if (!qmake.isEmpty())
        return qmake;

    // Check Qt5 settings: oh, the horror!
    const Utils::FileName qtCMakeDir
            = Utils::FileName::fromUtf8(CMakeConfigItem::valueOf(QByteArray("Qt5Core_DIR"), config));
    qCDebug(cmInputLog()) << "Qt5Core_DIR=" << qtCMakeDir.toUserOutput();
    const Utils::FileName canQtCMakeDir = Utils::FileName::fromString(qtCMakeDir.toFileInfo().canonicalFilePath());
    qCInfo(cmInputLog()) << "Qt5Core_DIR (canonical)=" << canQtCMakeDir.toUserOutput();
    if (qtCMakeDir.isEmpty())
        return Utils::FileName();
    const Utils::FileName baseQtDir = canQtCMakeDir.parentDir().parentDir().parentDir(); // Up 3 levels...
    qCDebug(cmInputLog()) << "BaseQtDir:" << baseQtDir.toUserOutput();

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
        if (!extras.open(QIODevice::ReadOnly))
            return Utils::FileName();

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

                if (line == "if ( NOT TARGET Qt5::qmake )")
                    inQmakeSection = true;

                if (!inQmakeSection)
                    continue;

                const QByteArray set = "set ( imported_location ";
                if (line.startsWith(set)) {
                    const int sp = origLine.indexOf('}');
                    const int ep = origLine.lastIndexOf('"');

                    QTC_ASSERT(sp > 0, return Utils::FileName());
                    QTC_ASSERT(ep > sp + 2, return Utils::FileName());
                    QTC_ASSERT(ep < origLine.count(), return Utils::FileName());

                    // Eat the leading "}/" and trailing "
                    const QByteArray locationPart =  origLine.mid(sp + 2, ep - 2 - sp);
                    Utils::FileName result = baseQtDir;
                    result.appendPath(QString::fromUtf8(locationPart));
                    return result;
                }
            }
        }
    }

    // Now try to make sense of .../Qt5CoreConfig.cmake:
    return Utils::FileName();
}

QVector<CMakeToolChainData> extractToolChainsFromCache(const CMakeConfig &config)
{
    QVector<CMakeToolChainData> result;
    for (const CMakeConfigItem &i : config) {
        if (!i.key.startsWith("CMAKE_") || !i.key.endsWith("_COMPILER"))
            continue;
        const QByteArray language = i.key.mid(6, i.key.count() - 6 - 9); // skip "CMAKE_" and "_COMPILER"
        result.append({language, Utils::FileName::fromUtf8(i.value)});
    }
    return result;
}

QList<void *> CMakeProjectImporter::examineDirectory(const Utils::FileName &importPath) const
{
    qCInfo(cmInputLog()) << "Examining directory:" << importPath.toUserOutput();
    Utils::FileName cacheFile = importPath;
    cacheFile.appendPath("CMakeCache.txt");

    if (!cacheFile.exists()) {
        qCDebug(cmInputLog()) << cacheFile.toUserOutput() << "does not exist, returning.";
        return { };
    }

    QString errorMessage;
    const CMakeConfig config = BuildDirManager::parseCMakeConfiguration(cacheFile, &errorMessage);
    if (config.isEmpty() || !errorMessage.isEmpty()) {
        qCDebug(cmInputLog()) << "Failed to read configuration from" << cacheFile << errorMessage;
        return { };
    }
    const auto homeDir
            = Utils::FileName::fromUserInput(QString::fromUtf8(CMakeConfigItem::valueOf("CMAKE_HOME_DIRECTORY", config)));
    const Utils::FileName canonicalProjectDirectory = Utils::FileUtils::canonicalPath(projectDirectory());
    if (homeDir != canonicalProjectDirectory) {
        qCDebug(cmInputLog()) << "Wrong source directory:" << homeDir.toUserOutput()
                              << "expected:" << canonicalProjectDirectory.toUserOutput();
        return { };
    }

    auto data = std::make_unique<DirectoryData>();
    data->buildDirectory = importPath;
    data->cmakeBuildType = CMakeConfigItem::valueOf("CMAKE_BUILD_TYPE", config);

    data->cmakeBinary
            = Utils::FileName::fromUtf8(CMakeConfigItem::valueOf("CMAKE_COMMAND", config));
    data->generator = CMakeConfigItem::valueOf("CMAKE_GENERATOR", config);
    data->extraGenerator = CMakeConfigItem::valueOf("CMAKE_EXTRA_GENERATOR", config);
    data->platform = CMakeConfigItem::valueOf("CMAKE_GENERATOR_PLATFORM", config);
    data->toolset = CMakeConfigItem::valueOf("CMAKE_GENERATOR_TOOLSET", config);

    data->sysroot = CMakeConfigItem::valueOf("CMAKE_SYSROOT", config);

    // Qt:
    const Utils::FileName qmake = qmakeFromCMakeCache(config);
    if (!qmake.isEmpty())
        data->qt = findOrCreateQtVersion(qmake);

    // ToolChains:
    data->toolChains = extractToolChainsFromCache(config);

    qCInfo(cmInputLog()) << "Offering to import" << importPath.toUserOutput();
    return {static_cast<void *>(data.release())};
}

bool CMakeProjectImporter::matchKit(void *directoryData, const Kit *k) const
{
    const DirectoryData *data = static_cast<DirectoryData *>(directoryData);

    CMakeTool *cm = CMakeKitInformation::cmakeTool(k);
    if (!cm || cm->cmakeExecutable() != data->cmakeBinary)
        return false;

    if (CMakeGeneratorKitInformation::generator(k) != QString::fromUtf8(data->generator)
            || CMakeGeneratorKitInformation::extraGenerator(k) != QString::fromUtf8(data->extraGenerator)
            || CMakeGeneratorKitInformation::platform(k) != QString::fromUtf8(data->platform)
            || CMakeGeneratorKitInformation::toolset(k) != QString::fromUtf8(data->toolset))
        return false;

    if (SysRootKitInformation::sysRoot(k) != Utils::FileName::fromUtf8(data->sysroot))
        return false;

    if (data->qt.qt && QtSupport::QtKitInformation::qtVersionId(k) != data->qt.qt->uniqueId())
        return false;

    for (const CMakeToolChainData &tcd : data->toolChains) {
        ToolChain *tc = ToolChainKitInformation::toolChain(k, tcd.mapLanguageIdToQtC());
        if (!tc || tc->compilerCommand() != tcd.compilerPath)
            return false;
    }

    qCDebug(cmInputLog()) << k->displayName()
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
            addTemporaryData(CMakeKitInformation::id(), cmtd.cmakeTool->id().toSetting(), k);

        CMakeGeneratorKitInformation::setGenerator(k, QString::fromUtf8(data->generator));
        CMakeGeneratorKitInformation::setExtraGenerator(k, QString::fromUtf8(data->extraGenerator));
        CMakeGeneratorKitInformation::setPlatform(k, QString::fromUtf8(data->platform));
        CMakeGeneratorKitInformation::setToolset(k, QString::fromUtf8(data->toolset));

        SysRootKitInformation::setSysRoot(k, Utils::FileName::fromUtf8(data->sysroot));

        for (const CMakeToolChainData &cmtcd : data->toolChains) {
            const ToolChainData tcd
                    = findOrCreateToolChains(cmtcd.compilerPath, cmtcd.mapLanguageIdToQtC());
            QTC_ASSERT(!tcd.tcs.isEmpty(), continue);

            if (tcd.areTemporary) {
                for (ToolChain *tc : tcd.tcs)
                    addTemporaryData(ToolChainKitInformation::id(), tc->id(), k);
            }

            ToolChainKitInformation::setToolChain(k, tcd.tcs.at(0));
        }

        qCInfo(cmInputLog()) << "Temporary Kit created.";
    });
}

QList<BuildInfo *> CMakeProjectImporter::buildInfoListForKit(const Kit *k, void *directoryData) const
{
    QList<BuildInfo *> result;
    DirectoryData *data = static_cast<DirectoryData *>(directoryData);
    auto factory = qobject_cast<CMakeBuildConfigurationFactory *>(
                IBuildConfigurationFactory::find(k, projectFilePath().toString()));
    if (!factory)
        return result;

    // create info:
    std::unique_ptr<CMakeBuildInfo>
            info(factory->createBuildInfo(k, projectDirectory().toString(),
                                          CMakeBuildConfigurationFactory::buildTypeFromByteArray(data->cmakeBuildType)));
    info->buildDirectory = data->buildDirectory;
    info->displayName = info->typeName;

    bool found = false;
    foreach (BuildInfo *bInfo, result) {
        if (*static_cast<CMakeBuildInfo *>(bInfo) == *info) {
            found = true;
            break;
        }
    }
    if (!found)
        result << info.release();

    qCDebug(cmInputLog()) << "BuildInfo configured.";

    return result;
}

CMakeProjectImporter::CMakeToolData
CMakeProjectImporter::findOrCreateCMakeTool(const Utils::FileName &cmakeToolPath) const
{
    CMakeToolData result;
    result.cmakeTool = CMakeToolManager::findByCommand(cmakeToolPath);
    if (!result.cmakeTool) {
        qCDebug(cmInputLog()) << "Creating temporary CMakeTool for" << cmakeToolPath.toUserOutput();
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
    CMakeKitInformation::setCMakeTool(k, Core::Id()); // Always mark Kit as not using this Qt
    CMakeToolManager::deregisterCMakeTool(Core::Id::fromSetting(vl.at(0)));
    qCDebug(cmInputLog()) << "Temporary CMake tool cleaned up.";
}

void CMakeProjectImporter::persistTemporaryCMake(Kit *k, const QVariantList &vl)
{
    if (vl.isEmpty())
        return; // No temporary CMake
    QTC_ASSERT(vl.count() == 1, return);
    const QVariant data = vl.at(0);
    CMakeTool *tmpCmake = CMakeToolManager::findById(Core::Id::fromSetting(data));
    CMakeTool *actualCmake = CMakeKitInformation::cmakeTool(k);

    // User changed Kit away from temporary CMake that was set up:
    if (tmpCmake && actualCmake != tmpCmake)
        CMakeToolManager::deregisterCMakeTool(tmpCmake->id());

    qCDebug(cmInputLog()) << "Temporary CMake tool made persistent.";
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

    Utils::FileName realQmake = qmakeFromCMakeCache(config);
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
            << QByteArrayList({"CXX"})
            << QStringList({"/usr/bin/g++"});
    QTest::newRow("CXX compiler, C compiler")
            << QStringList({"CMAKE_CXX_COMPILER=/usr/bin/g++", "CMAKE_C_COMPILER=/usr/bin/clang"})
            << QByteArrayList({"CXX", "C"})
            << QStringList({"/usr/bin/g++", "/usr/bin/clang"});
    QTest::newRow("CXX compiler, C compiler, strange compiler")
            << QStringList({"CMAKE_CXX_COMPILER=/usr/bin/g++",
                             "CMAKE_C_COMPILER=/usr/bin/clang",
                             "CMAKE_STRANGE_LANGUAGE_COMPILER=/tmp/strange/compiler"})
            << QByteArrayList({"CXX", "C", "STRANGE_LANGUAGE"})
            << QStringList({"/usr/bin/g++", "/usr/bin/clang", "/tmp/strange/compiler"});
    QTest::newRow("CXX compiler, C compiler, strange compiler (with junk)")
            << QStringList({"FOO=test",
                             "CMAKE_CXX_COMPILER=/usr/bin/g++",
                             "CMAKE_BUILD_TYPE=debug",
                             "CMAKE_C_COMPILER=/usr/bin/clang",
                             "SOMETHING_COMPILER=/usr/bin/something",
                             "CMAKE_STRANGE_LANGUAGE_COMPILER=/tmp/strange/compiler",
                             "BAR=more test"})
            << QByteArrayList({"CXX", "C", "STRANGE_LANGUAGE"})
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

    QVector<CMakeToolChainData> tcs = extractToolChainsFromCache(config);
    QCOMPARE(tcs.count(), expectedLanguages.count());
    for (int i = 0; i < tcs.count(); ++i) {
        QCOMPARE(tcs.at(i).languageId, expectedLanguages.at(i));
        QCOMPARE(tcs.at(i).compilerPath.toString(), expectedToolChains.at(i));
    }
}

} // namespace Internal
} // namespace CMakeProjectManager

#endif
