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

#include "qmakeprojectimporter.h"

#include "qmakebuildinfo.h"
#include "qmakekitinformation.h"
#include "qmakebuildconfiguration.h"
#include "qmakeproject.h"
#include "makefileparse.h"
#include "qmakestep.h"

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

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStringList>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

const Core::Id QT_IS_TEMPORARY("Qmake.TempQt");
const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios"; // ugly

QmakeProjectImporter::QmakeProjectImporter(const QString &path) :
    QtProjectImporter(path)
{ }

QList<BuildInfo *> QmakeProjectImporter::import(const FileName &importPath, bool silent)
{
    const auto &logs = MakeFileParse::logging();
    qCDebug(logs) << "QmakeProjectImporter::import" << importPath << silent;

    QList<BuildInfo *> result;
    QFileInfo fi = importPath.toFileInfo();
    if (!fi.exists() && !fi.isDir()) {
        qCDebug(logs) << "**doesn't exist";
        return result;
    }

    QStringList makefiles = QDir(importPath.toString()).entryList(QStringList(QLatin1String("Makefile*")));
    qCDebug(logs) << "  Makefiles:" << makefiles;

    foreach (const QString &file, makefiles) {
        qCDebug(logs) << "  Parsing makefile" << file;
        // find interesting makefiles
        QString makefile = importPath.toString() + QLatin1Char('/') + file;
        MakeFileParse parse(makefile);
        if (parse.makeFileState() != MakeFileParse::Okay)
            continue;
        QFileInfo qmakeFi = parse.qmakePath().toFileInfo();
        FileName canonicalQmakeBinary = FileName::fromString(qmakeFi.canonicalFilePath());
        if (canonicalQmakeBinary.isEmpty()) {
            qCDebug(logs) << "  " << parse.qmakePath() << "doesn't exist anymore";
            continue;
        }
        if (parse.srcProFile() != projectFilePath()) {
            qCDebug(logs) << "  pro files doesn't match" << parse.srcProFile() << projectFilePath();
            continue;
        }

        qCDebug(logs) << "  QMake:" << canonicalQmakeBinary;

        QtProjectImporter::QtVersionData data
                = QtProjectImporter::findOrCreateQtVersion(canonicalQmakeBinary);
        QTC_ASSERT(data.version, continue);
        qCDebug(logs) << "  qt version:" << data.version->displayName()
                      << " temporary:" << data.isTemporaryVersion;

        QMakeStepConfig::TargetArchConfig archConfig = parse.config().archConfig;
        QMakeStepConfig::OsType osType = parse.config().osType;
        qCDebug(logs) << "  archConfig:" << archConfig;
        qCDebug(logs) << "  osType:    " << osType;
        if (data.version->type() == QLatin1String(IOSQT)
                && osType == QMakeStepConfig::NoOsType) {
            osType = QMakeStepConfig::IphoneOS;
            qCDebug(logs) << "  IOS found without osType, adjusting osType" << osType;
        }

        if (data.version->type() == QLatin1String(Constants::DESKTOPQT)) {
            QList<ProjectExplorer::Abi> abis = data.version->qtAbis();
            if (!abis.isEmpty()) {
                ProjectExplorer::Abi abi = abis.first();
                if (abi.os() == ProjectExplorer::Abi::DarwinOS) {
                    if (abi.wordWidth() == 64)
                        archConfig = QMakeStepConfig::X86_64;
                    else
                        archConfig = QMakeStepConfig::X86;
                    qCDebug(logs) << "  OS X found without targetarch, adjusting archType" << archConfig;
                }
            }
        }

        // find qmake arguments and mkspec
        QString additionalArguments = parse.unparsedArguments();
        qCDebug(logs) << "  Unparsed arguments:" << additionalArguments;
        FileName parsedSpec =
                QmakeBuildConfiguration::extractSpecFromArguments(&additionalArguments,
                                                                  importPath.toString(), data.version);
        qCDebug(logs) << "  Extracted spec:" << parsedSpec;
        qCDebug(logs) << "  Arguments now:" << additionalArguments;

        FileName versionSpec = data.version->mkspec();
        if (parsedSpec.isEmpty() || parsedSpec == FileName::fromLatin1("default")) {
            parsedSpec = versionSpec;
            qCDebug(logs) << "  No parsed spec or default spec => parsed spec now:" << parsedSpec;
        }

        qCDebug(logs) << "*******************";
        qCDebug(logs) << "* Looking for kits";
        // Find kits (can be more than one, e.g. (Linux-)Desktop and embedded linux):
        QList<Kit *> kitList;
        foreach (Kit *k, KitManager::kits()) {
            BaseQtVersion *kitVersion = QtKitInformation::qtVersion(k);
            FileName kitSpec = QmakeKitInformation::mkspec(k);
            ToolChain *tc = ToolChainKitInformation::toolChain(k, ToolChain::Language::Cxx);
            if (kitSpec.isEmpty() && kitVersion)
                kitSpec = kitVersion->mkspecFor(tc);
            QMakeStepConfig::TargetArchConfig kitTargetArch = QMakeStepConfig::NoArch;
            QMakeStepConfig::OsType kitOsType = QMakeStepConfig::NoOsType;
            if (tc) {
                kitTargetArch = QMakeStepConfig::targetArchFor(tc->targetAbi(), kitVersion);
                kitOsType = QMakeStepConfig::osTypeFor(tc->targetAbi(), kitVersion);
            }
            qCDebug(logs) << k->displayName()
                          << "version:" << (kitVersion == data.version)
                          << "spec:" << (kitSpec == parsedSpec)
                          << "targetarch:" << (kitTargetArch == archConfig)
                          << "ostype:" << (kitOsType == osType);
            if (kitVersion == data.version
                    && kitSpec == parsedSpec
                    && kitTargetArch == archConfig
                    && kitOsType == osType)
                kitList.append(k);
        }
        if (kitList.isEmpty()) {
            kitList.append(createTemporaryKit(data, parsedSpec, archConfig, osType));
            qCDebug(logs) << "  No matching kits found, created new kit";
        }

        foreach (Kit *k, kitList) {
            addProject(k);

            auto factory = qobject_cast<QmakeBuildConfigurationFactory *>(
                        IBuildConfigurationFactory::find(k, projectFilePath()));

            if (!factory)
                continue;

            // create info:
            QmakeBuildInfo *info = new QmakeBuildInfo(factory);
            BaseQtVersion::QmakeBuildConfigs buildConfig = parse.effectiveBuildConfig(data.version->defaultBuildConfig());
            if (buildConfig & BaseQtVersion::DebugBuild) {
                info->buildType = BuildConfiguration::Debug;
                info->displayName = QCoreApplication::translate("QmakeProjectManager::Internal::QmakeProjectImporter", "Debug");
            } else {
                info->buildType = BuildConfiguration::Release;
                info->displayName = QCoreApplication::translate("QmakeProjectManager::Internal::QmakeProjectImporter", "Release");
            }
            info->kitId = k->id();
            info->buildDirectory = FileName::fromString(fi.absoluteFilePath());
            info->additionalArguments = additionalArguments;
            info->config = parse.config();
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

QStringList QmakeProjectImporter::importCandidates()
{
    QStringList candidates;

    QFileInfo pfi = QFileInfo(projectFilePath());
    const QString prefix = pfi.baseName();
    candidates << pfi.absolutePath();

    foreach (Kit *k, KitManager::kits()) {
        QFileInfo fi(QmakeBuildConfiguration::shadowBuildDirectory(projectFilePath(), k,
                                                                   QString(), BuildConfiguration::Unknown));
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
        } else if (activeTargetPriority < 1 && version && version->type() == QLatin1String(QtSupport::Constants::DESKTOPQT)) {
            activeTarget = t;
            activeTargetPriority = 1;
        }
    }
    return activeTarget;
}

static ToolChain *preferredToolChain(BaseQtVersion *qtVersion, const FileName &ms, const QMakeStepConfig::TargetArchConfig &archConfig)
{
    const FileName spec = ms.isEmpty() ? qtVersion->mkspec() : ms;

    QList<ToolChain *> toolchains = ToolChainManager::toolChains();
    QList<Abi> qtAbis = qtVersion->qtAbis();
    return findOr(toolchains,
                         toolchains.isEmpty() ? 0 : toolchains.first(),
                         [&spec, &archConfig, &qtAbis, &qtVersion](ToolChain *tc) -> bool{
                                return qtAbis.contains(tc->targetAbi())
                                        && tc->suggestedMkspecList().contains(spec)
                                        && QMakeStepConfig::targetArchFor(tc->targetAbi(), qtVersion) == archConfig;
                          });
}

Kit *QmakeProjectImporter::createTemporaryKit(const QtProjectImporter::QtVersionData &data,
                                              const FileName &parsedSpec,
                                              const QMakeStepConfig::TargetArchConfig &archConfig,
                                              const QMakeStepConfig::OsType &osType)
{
    Q_UNUSED(osType); // TODO use this to select the right toolchain?
    return QtProjectImporter::createTemporaryKit(data,
                                                 [&data, parsedSpec, archConfig](Kit *k) -> void {
        ToolChainKitInformation::setToolChain(k, preferredToolChain(data.version, parsedSpec, archConfig));
        if (parsedSpec != data.version->mkspec())
            QmakeKitInformation::setMkspec(k, parsedSpec);
    });
}

} // namespace Internal
} // namespace QmakeProjectManager
