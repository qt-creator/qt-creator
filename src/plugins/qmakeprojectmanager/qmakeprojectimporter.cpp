/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmakeprojectimporter.h"

#include "qmakebuildinfo.h"
#include "qmakekitinformation.h"
#include "qmakebuildconfiguration.h"
#include "qmakeproject.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>
#include <utils/qtcprocess.h>
#include <utils/algorithm.h>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <QMessageBox>

static const Core::Id QT_IS_TEMPORARY("Qmake.TempQt");

namespace QmakeProjectManager {
namespace Internal {

QmakeProjectImporter::QmakeProjectImporter(const QString &path) :
    ProjectExplorer::ProjectImporter(path)
{ }

QList<ProjectExplorer::BuildInfo *> QmakeProjectImporter::import(const Utils::FileName &importPath,
                                                                 bool silent)
{
    QList<ProjectExplorer::BuildInfo *> result;
    QFileInfo fi = importPath.toFileInfo();
    if (!fi.exists() && !fi.isDir())
        return result;

    QStringList makefiles = QDir(importPath.toString()).entryList(QStringList(QLatin1String("Makefile*")));

    QtSupport::BaseQtVersion *version = 0;
    bool temporaryVersion = false;

    foreach (const QString &file, makefiles) {
        // find interesting makefiles
        QString makefile = importPath.toString() + QLatin1Char('/') + file;
        Utils::FileName qmakeBinary = QtSupport::QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        QFileInfo qmakeFi = qmakeBinary.toFileInfo();
        Utils::FileName canonicalQmakeBinary = Utils::FileName::fromString(qmakeFi.canonicalFilePath());
        if (canonicalQmakeBinary.isEmpty())
            continue;
        if (QtSupport::QtVersionManager::makefileIsFor(makefile, projectFilePath()) != QtSupport::QtVersionManager::SameProject)
            continue;

        // Find version:
        foreach (QtSupport::BaseQtVersion *v, QtSupport::QtVersionManager::versions()) {
            QFileInfo vfi = v->qmakeCommand().toFileInfo();
            Utils::FileName current = Utils::FileName::fromString(vfi.canonicalFilePath());
            if (current == canonicalQmakeBinary) {
                version = v;
                break;
            }
        }

        if (version) {
            // Check if version is a temporary qt
            int qtId = version->uniqueId();
            temporaryVersion = Utils::anyOf(ProjectExplorer::KitManager::kits(), [&qtId](ProjectExplorer::Kit *k){
                return k->value(QT_IS_TEMPORARY, -1).toInt() == qtId;
            });
        } else {
            // Create a new version if not found:
            // Do not use the canonical path here...
            version = QtSupport::QtVersionFactory::createQtVersionFromQMakePath(qmakeBinary);
            if (!version)
                continue;

            bool oldIsUpdating = setIsUpdating(true);
            QtSupport::QtVersionManager::addVersion(version);
            setIsUpdating(oldIsUpdating);
            temporaryVersion = true;
        }

        // find qmake arguments and mkspec
        QPair<QtSupport::BaseQtVersion::QmakeBuildConfigs, QString> makefileBuildConfig =
                QtSupport::QtVersionManager::scanMakeFile(makefile, version->defaultBuildConfig());

        QString additionalArguments = makefileBuildConfig.second;
        Utils::FileName parsedSpec =
                QmakeBuildConfiguration::extractSpecFromArguments(&additionalArguments, importPath.toString(), version);
        QStringList deducedArguments =
                QmakeBuildConfiguration::extractDeducedArguments(&additionalArguments);

        Utils::FileName versionSpec = version->mkspec();
        if (parsedSpec.isEmpty() || parsedSpec == Utils::FileName::fromLatin1("default"))
            parsedSpec = versionSpec;

        QString specArgument;
        // Compare mkspecs and add to additional arguments
        if (parsedSpec != versionSpec)
            specArgument = QLatin1String("-spec ") + Utils::QtcProcess::quoteArg(parsedSpec.toUserOutput());
        Utils::QtcProcess::addArgs(&specArgument, additionalArguments);

        // Find kits (can be more than one, e.g. (Linux-)Desktop and embedded linux):
        QList<ProjectExplorer::Kit *> kitList;
        foreach (ProjectExplorer::Kit *k, ProjectExplorer::KitManager::kits()) {
            QtSupport::BaseQtVersion *kitVersion = QtSupport::QtKitInformation::qtVersion(k);
            Utils::FileName kitSpec = QmakeKitInformation::mkspec(k);
            ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(k);
            if (kitSpec.isEmpty() && kitVersion)
                kitSpec = kitVersion->mkspecFor(tc);
            QStringList kitDeducedArguments;
            if (tc)
                kitDeducedArguments = QmakeBuildConfiguration::deduceArgumnetsForTargetAbi(tc->targetAbi(), kitVersion);

            if (kitVersion == version
                    && kitSpec == parsedSpec
                    && kitDeducedArguments == deducedArguments)
                kitList.append(k);
        }
        if (kitList.isEmpty())
            kitList.append(createTemporaryKit(version, temporaryVersion, parsedSpec, deducedArguments));

        foreach (ProjectExplorer::Kit *k, kitList) {
            addProject(k);

            QmakeBuildConfigurationFactory *factory
                    = qobject_cast<QmakeBuildConfigurationFactory *>(
                        ProjectExplorer::IBuildConfigurationFactory::find(k, projectFilePath()));

            if (!factory)
                continue;

            // create info:
            QmakeBuildInfo *info = new QmakeBuildInfo(factory);
            if (makefileBuildConfig.first & QtSupport::BaseQtVersion::DebugBuild) {
                info->type = ProjectExplorer::BuildConfiguration::Debug;
                info->displayName = QCoreApplication::translate("QmakeProjectManager::Internal::QmakeProjectImporter", "Debug");
            } else {
                info->type = ProjectExplorer::BuildConfiguration::Release;
                info->displayName = QCoreApplication::translate("QmakeProjectManager::Internal::QmakeProjectImporter", "Release");
            }
            info->kitId = k->id();
            info->buildDirectory = Utils::FileName::fromString(fi.absoluteFilePath());
            info->additionalArguments = additionalArguments;
            info->makefile = makefile;

            bool found = false;
            foreach (ProjectExplorer::BuildInfo *bInfo, result) {
                if (*static_cast<QmakeBuildInfo *>(bInfo) == *info) {
                    found = true;
                    break;
                }
            }
            if (found)
                delete info;
            else
                result << info;
        }
    }

    if (result.isEmpty() && !silent)
        QMessageBox::critical(Core::ICore::mainWindow(),
                              QCoreApplication::translate("QmakeProjectManager::Internal::QmakeProjectImporter", "No Build Found"),
                              QCoreApplication::translate("QmakeProjectManager::Internal::QmakeProjectImporter", "No build found in %1 matching project %2.")
                .arg(importPath.toUserOutput()).arg(QDir::toNativeSeparators(projectFilePath())));

    return result;
}

QStringList QmakeProjectImporter::importCandidates(const Utils::FileName &projectPath)
{
    QStringList candidates;

    QFileInfo pfi = projectPath.toFileInfo();
    const QString prefix = pfi.baseName();
    candidates << pfi.absolutePath();

    QList<ProjectExplorer::Kit *> kitList = ProjectExplorer::KitManager::kits();
    foreach (ProjectExplorer::Kit *k, kitList) {
        QFileInfo fi(QmakeProject::shadowBuildDirectory(projectPath.toString(), k, QString()));
        const QString baseDir = fi.absolutePath();

        foreach (const QString &dir, QDir(baseDir).entryList()) {
            const QString path = baseDir + QLatin1Char('/') + dir;
            if (dir.startsWith(prefix) && !candidates.contains(path))
                candidates << path;
        }
    }
    return candidates;
}

ProjectExplorer::Target *QmakeProjectImporter::preferredTarget(const QList<ProjectExplorer::Target *> &possibleTargets)
{
    // Select active target
    // a) The default target
    // b) Simulator target
    // c) Desktop target
    // d) the first target
    ProjectExplorer::Target *activeTarget = possibleTargets.isEmpty() ? 0 : possibleTargets.at(0);
    int activeTargetPriority = 0;
    foreach (ProjectExplorer::Target *t, possibleTargets) {
        QtSupport::BaseQtVersion *version = QtSupport::QtKitInformation::qtVersion(t->kit());
        if (t->kit() == ProjectExplorer::KitManager::defaultKit()) {
            activeTarget = t;
            activeTargetPriority = 3;
        } else if (activeTargetPriority < 2 && version && version->type() == QLatin1String(QtSupport::Constants::SIMULATORQT)) {
            activeTarget = t;
            activeTargetPriority = 2;
        } else if (activeTargetPriority < 1 && version && version->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)) {
            activeTarget = t;
            activeTargetPriority = 1;
        }
    }
    return activeTarget;
}

void QmakeProjectImporter::cleanupKit(ProjectExplorer::Kit *k)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::version(k->value(QT_IS_TEMPORARY, -1).toInt());
    if (!version)
        return;

    // count how many kits are using this version
    int qtId = version->uniqueId();
    int count = Utils::count(ProjectExplorer::KitManager::kits(), [qtId](ProjectExplorer::Kit *k) {
        return k->value(QT_IS_TEMPORARY, -1).toInt() == qtId;
    });

    if (count == 0) // Remove if no other kit is using it. (The Kit k is not in KitManager::kits()
        QtSupport::QtVersionManager::removeVersion(version);
}

void QmakeProjectImporter::makePermanent(ProjectExplorer::Kit *k)
{
    if (!isTemporaryKit(k))
        return;
    setIsUpdating(true);
    int tempId = k->value(QT_IS_TEMPORARY, -1).toInt();
    int qtId = QtSupport::QtKitInformation::qtVersionId(k);
    if (tempId != qtId) {
        QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::version(tempId);
        int count = Utils::count(ProjectExplorer::KitManager::kits(), [tempId](ProjectExplorer::Kit *k) {
            return k->value(QT_IS_TEMPORARY, -1).toInt() == tempId;
        });
        if (count == 0)
            QtSupport::QtVersionManager::removeVersion(version);
    }

    foreach (ProjectExplorer::Kit *kit, ProjectExplorer::KitManager::kits())
        if (kit->value(QT_IS_TEMPORARY, -1).toInt() == tempId)
            kit->removeKeySilently(QT_IS_TEMPORARY);
    setIsUpdating(false);
    ProjectImporter::makePermanent(k);
}

namespace {
ProjectExplorer::ToolChain *preferredToolChain(QtSupport::BaseQtVersion *qtVersion, const Utils::FileName &ms, const QStringList &deducedArguments)
{
    const Utils::FileName spec = ms.isEmpty() ? qtVersion->mkspec() : ms;

    QList<ProjectExplorer::ToolChain *> toolchains = ProjectExplorer::ToolChainManager::toolChains();
    QList<ProjectExplorer::Abi> qtAbis = qtVersion->qtAbis();
    return Utils::findOr(toolchains,
                         toolchains.isEmpty() ? 0 : toolchains.first(),
                         [&spec, &deducedArguments, &qtAbis](ProjectExplorer::ToolChain *tc) -> bool{
                                return qtAbis.contains(tc->targetAbi())
                                        && tc->suggestedMkspecList().contains(spec)
                                        && QmakeBuildConfiguration::deduceArgumnetsForTargetAbi(tc->targetAbi(), 0) == deducedArguments;
                          });
}
}

ProjectExplorer::Kit *QmakeProjectImporter::createTemporaryKit(QtSupport::BaseQtVersion *version,
                                                               bool temporaryVersion,
                                                               const Utils::FileName &parsedSpec,
                                                               const QStringList &deducedQmakeArguments)
{
    ProjectExplorer::Kit *k = new ProjectExplorer::Kit;
    bool oldIsUpdating = setIsUpdating(true);
    {
        ProjectExplorer::KitGuard guard(k);

        QtSupport::QtKitInformation::setQtVersion(k, version);
        ProjectExplorer::ToolChainKitInformation::setToolChain(k, preferredToolChain(version, parsedSpec, deducedQmakeArguments));
        QmakeKitInformation::setMkspec(k, parsedSpec);

        markTemporary(k);
        if (temporaryVersion)
            k->setValue(QT_IS_TEMPORARY, version->uniqueId());

        // Set up other values:
        foreach (ProjectExplorer::KitInformation *ki, ProjectExplorer::KitManager::kitInformation()) {
            if (ki->id() == ProjectExplorer::ToolChainKitInformation::id()
                    || ki->id() == QtSupport::QtKitInformation::id())
                continue;
            ki->setup(k);
        }
        k->setUnexpandedDisplayName(version->displayName());;
    } // ~KitGuard, sending kitUpdated

    ProjectExplorer::KitManager::registerKit(k); // potentially adds kits to other targetsetuppages
    setIsUpdating(oldIsUpdating);
    return k;
}

} // namespace Internal
} // namespace QmakeProjectManager
