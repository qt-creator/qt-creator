// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmakeprojectimporter.h"

#include "makefileparse.h"
#include "qmakebuildconfiguration.h"
#include "qmakebuildinfo.h"
#include "qmakekitaspect.h"
#include "qmakeproject.h"
#include "qmakeprojectmanagertr.h"
#include "qmakestep.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionfactory.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/algorithm.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QLoggingCategory>
#include <QSet>
#include <QStringList>

#include <memory>

using namespace ProjectExplorer;
using namespace QtSupport;
using namespace Utils;

namespace QmakeProjectManager::Internal {

const Id QT_IS_TEMPORARY("Qmake.TempQt");
const char IOSQT[] = "Qt4ProjectManager.QtVersion.Ios"; // ugly

struct DirectoryData
{
    QString makefile;
    FilePath buildDirectory;
    FilePath canonicalQmakeBinary;
    QtProjectImporter::QtVersionData qtVersionData;
    QString parsedSpec;
    QtVersion::QmakeBuildConfigs buildConfig;
    QString additionalArguments;
    QMakeStepConfig config;
    QMakeStepConfig::OsType osType;
};

QmakeProjectImporter::QmakeProjectImporter(const FilePath &path) :
    QtProjectImporter(path)
{ }

FilePaths QmakeProjectImporter::importCandidates()
{
    FilePaths candidates{projectFilePath().absolutePath()};

    QSet<FilePath> seenBaseDirs;
    for (Kit *k : KitManager::kits()) {
        const FilePath sbdir = QmakeBuildConfiguration::shadowBuildDirectory
                    (projectFilePath(), k, QString(), BuildConfiguration::Unknown);

        const FilePath baseDir = sbdir.absolutePath();
        if (!Utils::insert(seenBaseDirs, baseDir))
            continue;
        for (const FilePath &path : baseDir.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (!candidates.contains(path))
                candidates << path;
        }
    }
    return candidates;
}

QList<void *> QmakeProjectImporter::examineDirectory(const FilePath &importPath,
                                                     QString *warningMessage) const
{
    Q_UNUSED(warningMessage)
    QList<void *> result;
    const QLoggingCategory &logs = MakeFileParse::logging();

    const QStringList makefiles = QDir(importPath.toString()).entryList(QStringList(("Makefile*")));
    qCDebug(logs) << "  Makefiles:" << makefiles;

    for (const QString &file : makefiles) {
        std::unique_ptr<DirectoryData> data(new DirectoryData);
        data->makefile = file;
        data->buildDirectory = importPath;

        qCDebug(logs) << "  Parsing makefile" << file;
        // find interesting makefiles
        const FilePath makefile = importPath / file;
        MakeFileParse parse(makefile, MakeFileParse::Mode::FilterKnownConfigValues);
        if (parse.makeFileState() != MakeFileParse::Okay) {
            qCDebug(logs) << "  Parsing the makefile failed" << makefile;
            continue;
        }
        if (parse.srcProFile() != projectFilePath()) {
            qCDebug(logs) << "  pro files doesn't match" << parse.srcProFile() << projectFilePath();
            continue;
        }

        data->canonicalQmakeBinary = parse.qmakePath().canonicalPath();
        if (data->canonicalQmakeBinary.isEmpty()) {
            qCDebug(logs) << "  " << parse.qmakePath() << "doesn't exist anymore";
            continue;
        }

        qCDebug(logs) << "  QMake:" << data->canonicalQmakeBinary;

        data->qtVersionData = QtProjectImporter::findOrCreateQtVersion(data->canonicalQmakeBinary);
        QtVersion *version = data->qtVersionData.qt;
        bool isTemporaryVersion = data->qtVersionData.isTemporary;

        QTC_ASSERT(version, continue);

        qCDebug(logs) << "  qt version:" << version->displayName() << " temporary:" << isTemporaryVersion;

        data->osType = parse.config().osType;

        qCDebug(logs) << "  osType:    " << data->osType;
        if (version->type() == QLatin1String(IOSQT)
                && data->osType == QMakeStepConfig::NoOsType) {
            data->osType = QMakeStepConfig::IphoneOS;
            qCDebug(logs) << "  IOS found without osType, adjusting osType" << data->osType;
        }

        // find qmake arguments and mkspec
        data->additionalArguments = parse.unparsedArguments();
        qCDebug(logs) << "  Unparsed arguments:" << data->additionalArguments;
        data->parsedSpec =
                QmakeBuildConfiguration::extractSpecFromArguments(&(data->additionalArguments), importPath, version);
        qCDebug(logs) << "  Extracted spec:" << data->parsedSpec;
        qCDebug(logs) << "  Arguments now:" << data->additionalArguments;

        const QString versionSpec = version->mkspec();
        if (data->parsedSpec.isEmpty() || data->parsedSpec == "default") {
            data->parsedSpec = versionSpec;
            qCDebug(logs) << "  No parsed spec or default spec => parsed spec now:" << data->parsedSpec;
        }
        data->buildConfig = parse.effectiveBuildConfig(data->qtVersionData.qt->defaultBuildConfig());
        data->config = parse.config();

        result.append(data.release());
    }
    return result;
}

bool QmakeProjectImporter::matchKit(void *directoryData, const Kit *k) const
{
    auto *data = static_cast<DirectoryData *>(directoryData);
    const QLoggingCategory &logs = MakeFileParse::logging();

    QtVersion *kitVersion = QtKitAspect::qtVersion(k);
    QString kitSpec = QmakeKitAspect::mkspec(k);
    ToolChain *tc = ToolChainKitAspect::cxxToolChain(k);
    if (kitSpec.isEmpty() && kitVersion)
        kitSpec = kitVersion->mkspecFor(tc);
    QMakeStepConfig::OsType kitOsType = QMakeStepConfig::NoOsType;
    if (tc) {
        kitOsType = QMakeStepConfig::osTypeFor(tc->targetAbi(), kitVersion);
    }
    qCDebug(logs) << k->displayName()
                  << "version:" << (kitVersion == data->qtVersionData.qt)
                  << "spec:" << (kitSpec == data->parsedSpec)
                  << "ostype:" << (kitOsType == data->osType);
    return kitVersion == data->qtVersionData.qt
            && kitSpec == data->parsedSpec
            && kitOsType == data->osType;
}

Kit *QmakeProjectImporter::createKit(void *directoryData) const
{
    auto *data = static_cast<DirectoryData *>(directoryData);
    return createTemporaryKit(data->qtVersionData, data->parsedSpec, data->osType);
}

const QList<BuildInfo> QmakeProjectImporter::buildInfoList(void *directoryData) const
{
    auto *data = static_cast<DirectoryData *>(directoryData);

    // create info:
    BuildInfo info;
    if (data->buildConfig & QtVersion::DebugBuild) {
        info.buildType = BuildConfiguration::Debug;
        info.displayName = Tr::tr("Debug");
    } else {
        info.buildType = BuildConfiguration::Release;
        info.displayName = Tr::tr("Release");
    }
    info.buildDirectory = data->buildDirectory;

    QmakeExtraBuildInfo extra;
    extra.additionalArguments = data->additionalArguments;
    extra.config = data->config;
    extra.makefile = data->makefile;
    info.extraInfo = QVariant::fromValue(extra);

    return {info};
}

void QmakeProjectImporter::deleteDirectoryData(void *directoryData) const
{
    delete static_cast<DirectoryData *>(directoryData);
}

static const Toolchains preferredToolChains(QtVersion *qtVersion, const QString &ms)
{
    const QString spec = ms.isEmpty() ? qtVersion->mkspec() : ms;

    const Toolchains toolchains = ToolChainManager::toolchains();
    const Abis qtAbis = qtVersion->qtAbis();
    const auto matcher = [&](const ToolChain *tc) {
        return qtAbis.contains(tc->targetAbi()) && tc->suggestedMkspecList().contains(spec);
    };
    ToolChain * const cxxToolchain = findOrDefault(toolchains, [matcher](const ToolChain *tc) {
        return tc->language() == ProjectExplorer::Constants::CXX_LANGUAGE_ID && matcher(tc);
    });
    ToolChain * const cToolchain = findOrDefault(toolchains, [matcher](const ToolChain *tc) {
        return tc->language() == ProjectExplorer::Constants::C_LANGUAGE_ID && matcher(tc);
    });
    Toolchains chosenToolchains;
    for (ToolChain * const tc : {cxxToolchain, cToolchain}) {
        if (tc)
            chosenToolchains << tc;
    };
    return chosenToolchains;
}

Kit *QmakeProjectImporter::createTemporaryKit(const QtProjectImporter::QtVersionData &data,
                                              const QString &parsedSpec,
                                              const QMakeStepConfig::OsType &osType) const
{
    Q_UNUSED(osType) // TODO use this to select the right toolchain?
    return QtProjectImporter::createTemporaryKit(data, [&data, parsedSpec](Kit *k) -> void {
        for (ToolChain *const tc : preferredToolChains(data.qt, parsedSpec))
            ToolChainKitAspect::setToolChain(k, tc);
        if (parsedSpec != data.qt->mkspec())
            QmakeKitAspect::setMkspec(k, parsedSpec, QmakeKitAspect::MkspecSource::Code);
    });
}

} // QmakeProjectManager::Internal
