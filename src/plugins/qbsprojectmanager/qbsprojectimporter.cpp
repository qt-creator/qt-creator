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

#include "qbsprojectimporter.h"

#include "qbsbuildconfiguration.h"
#include "qbsbuildinfo.h"
#include "qbspmlogging.h"

#include <coreplugin/documentmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <qbs.h>

#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

struct BuildGraphData
{
    FileName bgFilePath;
    QVariantMap overriddenProperties;
    FileName cCompilerPath;
    FileName cxxCompilerPath;
    FileName qtBinPath;
    FileName sysroot;
    QString buildVariant;
};
static BuildGraphData extractBgData(const qbs::Project::BuildGraphInfo &bgInfo)
{
    BuildGraphData bgData;
    bgData.bgFilePath = FileName::fromString(bgInfo.bgFilePath);
    bgData.overriddenProperties = bgInfo.overriddenProperties;
    const QVariantMap &moduleProps = bgInfo.requestedProperties;
    const QVariantMap prjCompilerPathByLanguage
            = moduleProps.value("cpp.compilerPathByLanguage").toMap();
    const QString prjCompilerPath = moduleProps.value("cpp.compilerPath").toString();
    const QStringList prjToolchain = moduleProps.value("qbs.toolchain").toStringList();
    const bool prjIsMsvc = prjToolchain.contains("msvc");
    bgData.cCompilerPath = FileName::fromString(
                prjIsMsvc ? prjCompilerPath : prjCompilerPathByLanguage.value("c").toString());
    bgData.cxxCompilerPath = FileName::fromString(
                prjIsMsvc ? prjCompilerPath : prjCompilerPathByLanguage.value("cpp").toString());
    bgData.qtBinPath = FileName::fromString(moduleProps.value("Qt.core.binPath").toString());
    bgData.sysroot = FileName::fromString(moduleProps.value("qbs.sysroot").toString());
    bgData.buildVariant = moduleProps.value("qbs.buildVariant").toString();
    return bgData;
}

QbsProjectImporter::QbsProjectImporter(const FileName &path) : QtProjectImporter(path)
{
}

static QString buildDir(const QString &projectFilePath, const Kit *k)
{
    const QString projectName = QFileInfo(projectFilePath).completeBaseName();
    ProjectMacroExpander expander(projectFilePath, projectName, k, QString(),
                                  BuildConfiguration::Unknown);
    const QString projectDir
            = Project::projectDirectory(FileName::fromString(projectFilePath)).toString();
    const QString buildPath = expander.expand(Core::DocumentManager::buildDirectory());
    return FileUtils::resolvePath(projectDir, buildPath);
}

static bool hasBuildGraph(const QString &dir)
{
    const QString &dirName = QFileInfo(dir).fileName();
    return QFileInfo::exists(dir + QLatin1Char('/') + dirName + QLatin1String(".bg"));
}

static QStringList candidatesForDirectory(const QString &dir)
{
    QStringList candidates;
    const QStringList &subDirs = QDir(dir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &subDir : subDirs) {
        const QString absSubDir = dir + QLatin1Char('/') + subDir;
        if (hasBuildGraph(absSubDir))
            candidates << absSubDir;
    }
    return candidates;
}

QStringList QbsProjectImporter::importCandidates()
{
    const QString projectDir = projectFilePath().toFileInfo().absolutePath();
    QStringList candidates = candidatesForDirectory(projectDir);

    QSet<QString> seenCandidates;
    seenCandidates.insert(projectDir);
    const auto &kits = KitManager::kits();
    for (Kit * const k : kits) {
        QFileInfo fi(buildDir(projectFilePath().toString(), k));
        const QString candidate = fi.absolutePath();
        if (!seenCandidates.contains(candidate)) {
            seenCandidates.insert(candidate);
            candidates << candidatesForDirectory(candidate);
        }
    }
    qCDebug(qbsPmLog) << "build directory candidates:" << candidates;
    return candidates;
}

QList<void *> QbsProjectImporter::examineDirectory(const FileName &importPath) const
{
    qCDebug(qbsPmLog) << "examining build directory" << importPath.toUserOutput();
    QList<void *> data;
    const QString bgFilePath = importPath.toString() + QLatin1Char('/') + importPath.fileName()
            + QLatin1String(".bg");
    const QStringList relevantProperties({
            "qbs.buildVariant", "qbs.sysroot", "qbs.toolchain",
            "cpp.compilerPath", "cpp.compilerPathByLanguage",
            "Qt.core.binPath"
    });
    const qbs::Project::BuildGraphInfo bgInfo
            = qbs::Project::getBuildGraphInfo(bgFilePath, relevantProperties);
    if (bgInfo.error.hasError()) {
        qCDebug(qbsPmLog) << "error getting build graph info:" << bgInfo.error.toString();
        return data;
    }
    qCDebug(qbsPmLog) << "retrieved build graph info:" << bgInfo.requestedProperties;
    data << new BuildGraphData(extractBgData(bgInfo));
    return data;
}

bool QbsProjectImporter::matchKit(void *directoryData, const Kit *k) const
{
    const auto * const bgData = static_cast<BuildGraphData *>(directoryData);
    qCDebug(qbsPmLog) << "matching kit" << k->displayName() << "against imported build"
                      << bgData->bgFilePath.toUserOutput();
    if (ToolChainKitInformation::toolChains(k).isEmpty() && bgData->cCompilerPath.isEmpty()
            && bgData->cxxCompilerPath.isEmpty()) {
        return true;
    }
    const ToolChain * const cToolchain
            = ToolChainKitInformation::toolChain(k, Constants::C_LANGUAGE_ID);
    const ToolChain * const cxxToolchain
            = ToolChainKitInformation::toolChain(k, Constants::CXX_LANGUAGE_ID);
    if (!bgData->cCompilerPath.isEmpty()) {
        if (!cToolchain)
            return false;
        if (bgData->cCompilerPath != cToolchain->compilerCommand())
            return false;
    }
    if (!bgData->cxxCompilerPath.isEmpty()) {
        if (!cxxToolchain)
            return false;
        if (bgData->cxxCompilerPath != cxxToolchain->compilerCommand())
            return false;
    }
    const QtSupport::BaseQtVersion * const qtVersion = QtSupport::QtKitInformation::qtVersion(k);
    if (!bgData->qtBinPath.isEmpty()) {
        if (!qtVersion)
            return false;
        if (bgData->qtBinPath != qtVersion->binPath())
            return false;
    }
    if (bgData->sysroot != SysRootKitInformation::sysRoot(k))
        return false;

    qCDebug(qbsPmLog) << "Kit matches";
    return true;
}

Kit *QbsProjectImporter::createKit(void *directoryData) const
{
    const auto * const bgData = static_cast<BuildGraphData *>(directoryData);
    qCDebug(qbsPmLog) << "creating kit for imported build" << bgData->bgFilePath.toUserOutput();
    QtVersionData qtVersionData;
    if (!bgData->qtBinPath.isEmpty()) {
        FileName qmakeFilePath = bgData->qtBinPath;
        qmakeFilePath.appendPath(HostOsInfo::withExecutableSuffix("qmake"));
        qtVersionData = findOrCreateQtVersion(qmakeFilePath);
    }
    return createTemporaryKit(qtVersionData,[this, bgData](Kit *k) -> void {
        QList<ToolChainData> tcData;
        if (!bgData->cxxCompilerPath.isEmpty())
            tcData << findOrCreateToolChains(bgData->cxxCompilerPath, Constants::CXX_LANGUAGE_ID);
        if (!bgData->cCompilerPath.isEmpty())
            tcData << findOrCreateToolChains(bgData->cCompilerPath, Constants::C_LANGUAGE_ID);
        foreach (const ToolChainData &tc, tcData) {
            if (!tc.tcs.isEmpty())
                ToolChainKitInformation::setToolChain(k, tc.tcs.first());
        }
        SysRootKitInformation::setSysRoot(k, bgData->sysroot);
    });
}

QList<BuildInfo *> QbsProjectImporter::buildInfoListForKit(const Kit *k, void *directoryData) const
{
    qCDebug(qbsPmLog) << "creating build info for kit" << k->displayName();
    QList<BuildInfo *> result;
    const auto factory = qobject_cast<QbsBuildConfigurationFactory *>(
                IBuildConfigurationFactory::find(k, projectFilePath().toString()));
    if (!factory) {
        qCDebug(qbsPmLog) << "no build config factory found";
        return result;
    }
    const auto * const bgData = static_cast<BuildGraphData *>(directoryData);
    QbsBuildInfo * const buildInfo = new QbsBuildInfo(factory);
    buildInfo->displayName = bgData->bgFilePath.toFileInfo().completeBaseName();
    buildInfo->buildType = bgData->buildVariant == "debug"
            ? BuildConfiguration::Debug : BuildConfiguration::Release;
    buildInfo->kitId = k->id();
    buildInfo->buildDirectory = bgData->bgFilePath.parentDir().parentDir();
    buildInfo->config = bgData->overriddenProperties;
    buildInfo->config.insert("configName", buildInfo->displayName);
    return result << buildInfo;
}

void QbsProjectImporter::deleteDirectoryData(void *directoryData) const
{
    delete static_cast<BuildGraphData *>(directoryData);
}

} // namespace Internal
} // namespace QbsProjectManager
