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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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
#include <QMessageBox>
#include <QStringList>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

const Core::Id QT_IS_TEMPORARY("Qmake.TempQt");

QmakeProjectImporter::QmakeProjectImporter(const QString &path) :
    ProjectImporter(path)
{ }

QList<BuildInfo *> QmakeProjectImporter::import(const FileName &importPath, bool silent)
{
    QList<BuildInfo *> result;
    QFileInfo fi = importPath.toFileInfo();
    if (!fi.exists() && !fi.isDir())
        return result;

    QStringList makefiles = QDir(importPath.toString()).entryList(QStringList(QLatin1String("Makefile*")));

    BaseQtVersion *version = 0;
    bool temporaryVersion = false;

    foreach (const QString &file, makefiles) {
        // find interesting makefiles
        QString makefile = importPath.toString() + QLatin1Char('/') + file;
        FileName qmakeBinary = QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        QFileInfo qmakeFi = qmakeBinary.toFileInfo();
        FileName canonicalQmakeBinary = FileName::fromString(qmakeFi.canonicalFilePath());
        if (canonicalQmakeBinary.isEmpty())
            continue;
        if (QtVersionManager::makefileIsFor(makefile, projectFilePath()) != QtVersionManager::SameProject)
            continue;

        // Find version:
        foreach (BaseQtVersion *v, QtVersionManager::versions()) {
            QFileInfo vfi = v->qmakeCommand().toFileInfo();
            FileName current = FileName::fromString(vfi.canonicalFilePath());
            if (current == canonicalQmakeBinary) {
                version = v;
                break;
            }
        }

        if (version) {
            // Check if version is a temporary qt
            int qtId = version->uniqueId();
            temporaryVersion = Utils::anyOf(KitManager::kits(), [&qtId](Kit *k){
                return k->value(QT_IS_TEMPORARY, -1).toInt() == qtId;
            });
        } else {
            // Create a new version if not found:
            // Do not use the canonical path here...
            version = QtVersionFactory::createQtVersionFromQMakePath(qmakeBinary);
            if (!version)
                continue;

            bool oldIsUpdating = setIsUpdating(true);
            QtVersionManager::addVersion(version);
            setIsUpdating(oldIsUpdating);
            temporaryVersion = true;
        }

        // find qmake arguments and mkspec
        QPair<BaseQtVersion::QmakeBuildConfigs, QString> makefileBuildConfig =
                QtVersionManager::scanMakeFile(makefile, version->defaultBuildConfig());

        QString additionalArguments = makefileBuildConfig.second;
        FileName parsedSpec =
                QmakeBuildConfiguration::extractSpecFromArguments(&additionalArguments, importPath.toString(), version);
        QStringList deducedArguments =
                QmakeBuildConfiguration::extractDeducedArguments(&additionalArguments);

        FileName versionSpec = version->mkspec();
        if (parsedSpec.isEmpty() || parsedSpec == FileName::fromLatin1("default"))
            parsedSpec = versionSpec;

        QString specArgument;
        // Compare mkspecs and add to additional arguments
        if (parsedSpec != versionSpec)
            specArgument = QLatin1String("-spec ") + QtcProcess::quoteArg(parsedSpec.toUserOutput());
        QtcProcess::addArgs(&specArgument, additionalArguments);

        // Find kits (can be more than one, e.g. (Linux-)Desktop and embedded linux):
        QList<Kit *> kitList;
        foreach (Kit *k, KitManager::kits()) {
            BaseQtVersion *kitVersion = QtKitInformation::qtVersion(k);
            FileName kitSpec = QmakeKitInformation::mkspec(k);
            ToolChain *tc = ToolChainKitInformation::toolChain(k);
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

        foreach (Kit *k, kitList) {
            addProject(k);

            auto factory = qobject_cast<QmakeBuildConfigurationFactory *>(
                        IBuildConfigurationFactory::find(k, projectFilePath()));

            if (!factory)
                continue;

            // create info:
            QmakeBuildInfo *info = new QmakeBuildInfo(factory);
            if (makefileBuildConfig.first & BaseQtVersion::DebugBuild) {
                info->type = BuildConfiguration::Debug;
                info->displayName = QCoreApplication::translate("QmakeProjectManager::Internal::QmakeProjectImporter", "Debug");
            } else {
                info->type = BuildConfiguration::Release;
                info->displayName = QCoreApplication::translate("QmakeProjectManager::Internal::QmakeProjectImporter", "Release");
            }
            info->kitId = k->id();
            info->buildDirectory = FileName::fromString(fi.absoluteFilePath());
            info->additionalArguments = additionalArguments;
            info->makefile = makefile;

            bool found = false;
            foreach (BuildInfo *bInfo, result) {
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

QStringList QmakeProjectImporter::importCandidates(const FileName &projectPath)
{
    QStringList candidates;

    QFileInfo pfi = projectPath.toFileInfo();
    const QString prefix = pfi.baseName();
    candidates << pfi.absolutePath();

    foreach (Kit *k, KitManager::kits()) {
        QFileInfo fi(QmakeBuildConfiguration::shadowBuildDirectory(projectPath.toString(), k, QString()));
        const QString baseDir = fi.absolutePath();

        foreach (const QString &dir, QDir(baseDir).entryList()) {
            const QString path = baseDir + QLatin1Char('/') + dir;
            if (dir.startsWith(prefix) && !candidates.contains(path))
                candidates << path;
        }
    }
    return candidates;
}

Target *QmakeProjectImporter::preferredTarget(const QList<Target *> &possibleTargets)
{
    // Select active target
    // a) The default target
    // b) Simulator target
    // c) Desktop target
    // d) the first target
    Target *activeTarget = possibleTargets.isEmpty() ? 0 : possibleTargets.at(0);
    int activeTargetPriority = 0;
    foreach (Target *t, possibleTargets) {
        BaseQtVersion *version = QtKitInformation::qtVersion(t->kit());
        if (t->kit() == KitManager::defaultKit()) {
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

void QmakeProjectImporter::cleanupKit(Kit *k)
{
    BaseQtVersion *version = QtVersionManager::version(k->value(QT_IS_TEMPORARY, -1).toInt());
    if (!version)
        return;

    // count how many kits are using this version
    int qtId = version->uniqueId();
    int users = Utils::count(KitManager::kits(), [qtId](Kit *k) {
        return k->value(QT_IS_TEMPORARY, -1).toInt() == qtId;
    });

    if (users == 0) // Remove if no other kit is using it. (The Kit k is not in KitManager::kits()
        QtVersionManager::removeVersion(version);
}

void QmakeProjectImporter::makePermanent(Kit *k)
{
    if (!isTemporaryKit(k))
        return;
    setIsUpdating(true);
    int tempId = k->value(QT_IS_TEMPORARY, -1).toInt();
    int qtId = QtKitInformation::qtVersionId(k);
    if (tempId != qtId) {
        BaseQtVersion *version = QtVersionManager::version(tempId);
        int users = count(KitManager::kits(), [tempId](Kit *k) {
            return k->value(QT_IS_TEMPORARY, -1).toInt() == tempId;
        });
        if (users == 0)
            QtVersionManager::removeVersion(version);
    }

    foreach (Kit *kit, KitManager::kits())
        if (kit->value(QT_IS_TEMPORARY, -1).toInt() == tempId)
            kit->removeKeySilently(QT_IS_TEMPORARY);
    setIsUpdating(false);
    ProjectImporter::makePermanent(k);
}

static ToolChain *preferredToolChain(BaseQtVersion *qtVersion, const FileName &ms, const QStringList &deducedArguments)
{
    const FileName spec = ms.isEmpty() ? qtVersion->mkspec() : ms;

    QList<ToolChain *> toolchains = ToolChainManager::toolChains();
    QList<Abi> qtAbis = qtVersion->qtAbis();
    return findOr(toolchains,
                         toolchains.isEmpty() ? 0 : toolchains.first(),
                         [&spec, &deducedArguments, &qtAbis](ToolChain *tc) -> bool{
                                return qtAbis.contains(tc->targetAbi())
                                        && tc->suggestedMkspecList().contains(spec)
                                        && QmakeBuildConfiguration::deduceArgumnetsForTargetAbi(tc->targetAbi(), 0) == deducedArguments;
                          });
}

Kit *QmakeProjectImporter::createTemporaryKit(BaseQtVersion *version,
                                              bool temporaryVersion,
                                              const FileName &parsedSpec,
                                              const QStringList &deducedQmakeArguments)
{
    Kit *k = new Kit;
    bool oldIsUpdating = setIsUpdating(true);
    {
        KitGuard guard(k);

        QtKitInformation::setQtVersion(k, version);
        ToolChainKitInformation::setToolChain(k, preferredToolChain(version, parsedSpec, deducedQmakeArguments));
        QmakeKitInformation::setMkspec(k, parsedSpec);

        markTemporary(k);
        if (temporaryVersion)
            k->setValue(QT_IS_TEMPORARY, version->uniqueId());

        // Set up other values:
        foreach (KitInformation *ki, KitManager::kitInformation()) {
            if (ki->id() == ToolChainKitInformation::id() || ki->id() == QtKitInformation::id())
                continue;
            ki->setup(k);
        }
        k->setUnexpandedDisplayName(version->displayName());;
    } // ~KitGuard, sending kitUpdated

    KitManager::registerKit(k); // potentially adds kits to other targetsetuppages
    setIsUpdating(oldIsUpdating);
    return k;
}

} // namespace Internal
} // namespace QmakeProjectManager
