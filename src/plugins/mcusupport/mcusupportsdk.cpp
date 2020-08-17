/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "mcusupportconstants.h"
#include "mcusupportoptions.h"
#include "mcusupportsdk.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QDirIterator>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

namespace McuSupport {
namespace Internal {
namespace Sdk {

static QString findInProgramFiles(const QString &folder)
{
    for (auto envVar : {"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"}) {
        if (!qEnvironmentVariableIsSet(envVar))
            continue;
        const Utils::FilePath dir =
                Utils::FilePath::fromUserInput(qEnvironmentVariable(envVar) + "/" + folder);
        if (dir.exists())
            return dir.toString();
    }
    return {};
}

McuPackage *createQtForMCUsPackage()
{
    auto result = new McuPackage(
                McuPackage::tr("Qt for MCUs %1+ SDK").arg(
                    McuSupportOptions::minimalQulVersion().toString()),
                QDir::homePath(),
                Utils::HostOsInfo::withExecutableSuffix("bin/qmltocpp"),
                Constants::SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK);
    result->setEnvironmentVariableName("Qul_DIR");
    return result;
}

static McuToolChainPackage *createDesktopToolChainPackage()
{
    return new McuToolChainPackage({}, {}, {}, {}, McuToolChainPackage::TypeDesktop);
}

static McuToolChainPackage *createArmGccPackage()
{
    const char envVar[] = "ARMGCC_DIR";

    QString defaultPath;
    if (qEnvironmentVariableIsSet(envVar))
        defaultPath = qEnvironmentVariable(envVar);
    if (defaultPath.isEmpty() && Utils::HostOsInfo::isWindowsHost()) {
        const QDir installDir(findInProgramFiles("/GNU Tools ARM Embedded/"));
        if (installDir.exists()) {
            // If GNU Tools installation dir has only one sub dir,
            // select the sub dir, otherwise the installation dir.
            const QFileInfoList subDirs =
                    installDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            if (subDirs.count() == 1)
                defaultPath = subDirs.first().filePath() + '/';
        }
    }
    if (defaultPath.isEmpty())
        defaultPath = QDir::homePath();

    auto result = new McuToolChainPackage(
                McuPackage::tr("GNU Arm Embedded Toolchain"),
                defaultPath,
                Utils::HostOsInfo::withExecutableSuffix("bin/arm-none-eabi-g++"),
                "GNUArmEmbeddedToolchain",
                McuToolChainPackage::TypeArmGcc);
    result->setEnvironmentVariableName(envVar);
    return result;
}

static McuToolChainPackage *createGhsToolchainPackage()
{
    const char envVar[] = "GHS_COMPILER_DIR";

    const QString defaultPath =
            qEnvironmentVariableIsSet(envVar) ? qEnvironmentVariable(envVar) : QDir::homePath();

    auto result = new McuToolChainPackage(
                "Green Hills Compiler",
                defaultPath,
                Utils::HostOsInfo::withExecutableSuffix("ccv850"),
                "GHSToolchain",
                McuToolChainPackage::TypeGHS);
    result->setEnvironmentVariableName(envVar);
    return result;
}

static McuPackage *createRGLPackage()
{
    const char envVar[] = "RGL_DIR";

    QString defaultPath;
    if (qEnvironmentVariableIsSet(envVar)) {
        defaultPath = qEnvironmentVariable(envVar);
    } else if (Utils::HostOsInfo::isWindowsHost()) {
        defaultPath = QDir::rootPath() + "Renesas_Electronics/D1x_RGL";
        if (QFileInfo::exists(defaultPath)) {
            const QFileInfoList subDirs =
                    QDir(defaultPath).entryInfoList({QLatin1String("rgl_ghs_D1Mx_*")},
                                                    QDir::Dirs | QDir::NoDotAndDotDot);
            if (subDirs.count() == 1)
                defaultPath = subDirs.first().filePath() + '/';
        }
    }

    auto result = new McuPackage(
                "Renesas Graphics Library",
                defaultPath,
                {},
                "RGL");
    result->setEnvironmentVariableName(envVar);
    return result;
}

static McuPackage *createStm32CubeProgrammerPackage()
{
    QString defaultPath = QDir::homePath();
    if (Utils::HostOsInfo::isWindowsHost()) {
        const QString programPath =
                findInProgramFiles("/STMicroelectronics/STM32Cube/STM32CubeProgrammer/");
        if (!programPath.isEmpty())
            defaultPath = programPath;
    }
    auto result = new McuPackage(
                McuPackage::tr("STM32CubeProgrammer"),
                defaultPath,
                QLatin1String(Utils::HostOsInfo::isWindowsHost() ? "/bin/STM32_Programmer_CLI.exe"
                                                                 : "/bin/STM32_Programmer.sh"),
                "Stm32CubeProgrammer");
    result->setRelativePathModifier("/bin");
    result->setDownloadUrl(
                "https://www.st.com/en/development-tools/stm32cubeprog.html");
    result->setAddToPath(true);
    return result;
}

static McuPackage *createMcuXpressoIdePackage()
{
    const char envVar[] = "MCUXpressoIDE_PATH";

    QString defaultPath;
    if (qEnvironmentVariableIsSet(envVar)) {
        defaultPath = qEnvironmentVariable(envVar);
    } else if (Utils::HostOsInfo::isWindowsHost()) {
        defaultPath = QDir::rootPath() + "nxp";
        if (QFileInfo::exists(defaultPath)) {
            // If default dir has exactly one sub dir that could be the IDE path, pre-select that.
            const QFileInfoList subDirs =
                    QDir(defaultPath).entryInfoList({QLatin1String("MCUXpressoIDE*")},
                                                    QDir::Dirs | QDir::NoDotAndDotDot);
            if (subDirs.count() == 1)
                defaultPath = subDirs.first().filePath() + '/';
        }
    } else {
        defaultPath = "/usr/local/mcuxpressoide/";
    }

    auto result = new McuPackage(
                "MCUXpresso IDE",
                defaultPath,
                Utils::HostOsInfo::withExecutableSuffix("ide/binaries/crt_emu_cm_redlink"),
                "MCUXpressoIDE");
    result->setDownloadUrl("https://www.nxp.com/mcuxpresso/ide");
    result->setEnvironmentVariableName(envVar);
    return result;
}

struct McuTargetDescription
{
    QString qulVersion;
    QString platform;
    QString platformVendor;
    QVector<int> colorDepths;
    QString toolchainId;
    QString boardSdkEnvVar;
    QString boardSdkName;
    QString boardSdkDefaultPath;
    QString freeRTOSEnvVar;
    QString freeRTOSBoardSdkSubDir;
};

static McuPackage *createBoardSdkPackage(const McuTargetDescription& desc)
{
    const auto generateSdkName = [](const QString& envVar) {
        auto postfixPos = envVar.indexOf("_SDK_PATH");
        if (postfixPos < 0) {
            postfixPos = envVar.indexOf("_DIR");
        }
        auto sdkName = postfixPos > 0 ? envVar.left(postfixPos) : envVar;
        return QString::fromLatin1("MCU SDK (%1)").arg(sdkName);
    };
    const QString sdkName = desc.boardSdkName.isEmpty() ? generateSdkName(desc.boardSdkEnvVar) : desc.boardSdkName;

    const QString defaultPath = [&] {
        const auto envVar = desc.boardSdkEnvVar.toLatin1();
        if (qEnvironmentVariableIsSet(envVar)) {
            return qEnvironmentVariable(envVar);
        }
        if (!desc.boardSdkDefaultPath.isEmpty()) {
            QString defaultPath = QDir::rootPath() + desc.boardSdkDefaultPath;
            if (QFileInfo::exists(defaultPath)) {
                return defaultPath;
            }
        }
        return QDir::homePath();
    }();

    auto result = new McuPackage(
                sdkName,
                defaultPath,
                {},
                desc.boardSdkEnvVar);
    result->setEnvironmentVariableName(desc.boardSdkEnvVar);
    return result;
}

static McuPackage *createFreeRTOSSourcesPackage(const QString &envVar, const QString &boardSdkDir,
                                                const QString &freeRTOSBoardSdkSubDir)
{
    const QString envVarPrefix = envVar.chopped(strlen("_FREERTOS_DIR"));

    QString defaultPath;
    if (qEnvironmentVariableIsSet(envVar.toLatin1()))
        defaultPath = qEnvironmentVariable(envVar.toLatin1());
    else if (!boardSdkDir.isEmpty() && !freeRTOSBoardSdkSubDir.isEmpty())
        defaultPath = boardSdkDir + "/" + freeRTOSBoardSdkSubDir;
    else
        defaultPath = QDir::homePath();

    auto result = new McuPackage(
                QString::fromLatin1("FreeRTOS Sources (%1)").arg(envVarPrefix),
                defaultPath,
                {},
                QString::fromLatin1("FreeRTOSSourcePackage_%1").arg(envVarPrefix));
    result->setDownloadUrl("https://freertos.org");
    result->setEnvironmentVariableName(envVar);
    return result;
}

struct McuTargetFactory
{
    McuTargetFactory(const QHash<QString, McuToolChainPackage *> &tcPkgs,
                     const QHash<QString, McuPackage *> &vendorPkgs)
        : tcPkgs(tcPkgs)
        , vendorPkgs(vendorPkgs)
    {}

    QVector<McuTarget *> createTargets(const McuTargetDescription& description)
    {
        if (description.toolchainId == "desktop") {
            return createDesktopTargets(description);
        }
        auto qulVersion = QVersionNumber::fromString(description.qulVersion);
        if (qulVersion <= QVersionNumber({1,3})) {
            // There was a platform backends related refactoring in Qul 1.4
            // This requires different processing of McuTargetDescriptions
            return createMcuTargetsLegacy(description);
        }
        return createMcuTargets(description);
    }

    QVector<McuPackage *> getMcuPackages() const
    {
        QVector<McuPackage *> packages;
        packages.append(boardSdkPkgs.values().toVector());
        packages.append(freeRTOSPkgs.values().toVector());
        return packages;
    }

protected:
    // Implementation for Qul version <= 1.3
    QVector<McuTarget *> createMcuTargetsLegacy(const McuTargetDescription& desc)
    {
        QVector<McuTarget *> mcuTargets;
        auto tcPkg = tcPkgs.value(desc.toolchainId);
        for (auto os : {McuTarget::OS::BareMetal, McuTarget::OS::FreeRTOS}) {
            for (int colorDepth : desc.colorDepths) {
                QVector<McuPackage*> required3rdPartyPkgs = { tcPkg };
                if (vendorPkgs.contains(desc.platformVendor)) {
                   required3rdPartyPkgs.push_back(vendorPkgs.value(desc.platformVendor));
                }
                QString boardSdkDefaultPath;
                if (!desc.boardSdkEnvVar.isEmpty()) {
                    if (!boardSdkPkgs.contains(desc.boardSdkEnvVar)) {
                        auto boardSdkPkg = desc.boardSdkEnvVar != "RGL_DIR"
                                            ? createBoardSdkPackage(desc)
                                            : createRGLPackage();
                        boardSdkPkgs.insert(desc.boardSdkEnvVar, boardSdkPkg);
                    }
                    auto boardSdkPkg = boardSdkPkgs.value(desc.boardSdkEnvVar);
                    boardSdkDefaultPath = boardSdkPkg->defaultPath();
                    required3rdPartyPkgs.append(boardSdkPkg);
                }
                if (os == McuTarget::OS::FreeRTOS) {
                    if (desc.freeRTOSEnvVar.isEmpty()) {
                        continue;
                    } else {
                        if (!freeRTOSPkgs.contains(desc.freeRTOSEnvVar)) {
                            freeRTOSPkgs.insert(desc.freeRTOSEnvVar, createFreeRTOSSourcesPackage(
                                                    desc.freeRTOSEnvVar, boardSdkDefaultPath,
                                                    desc.freeRTOSBoardSdkSubDir));
                        }
                        required3rdPartyPkgs.append(freeRTOSPkgs.value(desc.freeRTOSEnvVar));
                    }
                }

                auto mcuTarget = new McuTarget(QVersionNumber::fromString(desc.qulVersion),
                                               desc.platformVendor, desc.platform, os,
                                               required3rdPartyPkgs, tcPkg);
                if (desc.colorDepths.count() > 1)
                    mcuTarget->setColorDepth(colorDepth);
                mcuTargets.append(mcuTarget);
            }
        }
        return mcuTargets;
    }

    QVector<McuTarget *> createMcuTargets(const McuTargetDescription& desc)
    {
        QVector<McuTarget *> mcuTargets;
        auto tcPkg = tcPkgs.value(desc.toolchainId);
        for (int colorDepth : desc.colorDepths) {
            QVector<McuPackage*> required3rdPartyPkgs = { tcPkg };
            if (vendorPkgs.contains(desc.platformVendor)) {
                required3rdPartyPkgs.push_back(vendorPkgs.value(desc.platformVendor));
            }
            QString boardSdkDefaultPath;
            if (!desc.boardSdkEnvVar.isEmpty()) {
                if (!boardSdkPkgs.contains(desc.boardSdkEnvVar)) {
                    auto boardSdkPkg = createBoardSdkPackage(desc);
                    boardSdkPkgs.insert(desc.boardSdkEnvVar, boardSdkPkg);
                }
                auto boardSdkPkg = boardSdkPkgs.value(desc.boardSdkEnvVar);
                boardSdkDefaultPath = boardSdkPkg->defaultPath();
                required3rdPartyPkgs.append(boardSdkPkg);
            }

            auto os = McuTarget::OS::BareMetal;
            if (!desc.freeRTOSEnvVar.isEmpty()) {
                os = McuTarget::OS::FreeRTOS;
                if (!freeRTOSPkgs.contains(desc.freeRTOSEnvVar)) {
                    freeRTOSPkgs.insert(desc.freeRTOSEnvVar, createFreeRTOSSourcesPackage(
                                            desc.freeRTOSEnvVar, boardSdkDefaultPath,
                                            desc.freeRTOSBoardSdkSubDir));
                }
                required3rdPartyPkgs.append(freeRTOSPkgs.value(desc.freeRTOSEnvVar));
            }

            auto mcuTarget = new McuTarget(QVersionNumber::fromString(desc.qulVersion),
                                            desc.platformVendor, desc.platform, os,
                                            required3rdPartyPkgs, tcPkg);
            mcuTarget->setColorDepth(colorDepth);
            mcuTargets.append(mcuTarget);
        }
        return mcuTargets;
    }

    QVector<McuTarget *> createDesktopTargets(const McuTargetDescription& desc)
    {
        auto tcPkg = tcPkgs.value(desc.toolchainId);
        auto desktopTarget = new McuTarget(QVersionNumber::fromString(desc.qulVersion),
                                        desc.platformVendor, desc.platform,
                                        McuTarget::OS::Desktop, {}, tcPkg);
        return { desktopTarget };
    }

private:
    const QHash<QString, McuToolChainPackage *> &tcPkgs;
    const QHash<QString, McuPackage *> &vendorPkgs;

    QHash<QString, McuPackage *> boardSdkPkgs;
    QHash<QString, McuPackage *> freeRTOSPkgs;
};

static QVector<McuTarget *> targetsFromDescriptions(const QList<McuTargetDescription> &descriptions,
                                                    QVector<McuPackage *> *packages)
{
    const QHash<QString, McuToolChainPackage *> tcPkgs = {
        {{"armgcc"}, createArmGccPackage()},
        {{"greenhills"}, createGhsToolchainPackage()},
        {{"desktop"}, createDesktopToolChainPackage()},
    };

    const QHash<QString, McuPackage *> vendorPkgs = {
        {{"ST"}, createStm32CubeProgrammerPackage()},
        {{"NXP"}, createMcuXpressoIdePackage()},
    };

    McuTargetFactory targetFactory(tcPkgs, vendorPkgs);
    QVector<McuTarget *> mcuTargets;

    for (const auto &desc : descriptions) {
        auto newTargets = targetFactory.createTargets(desc);
        mcuTargets.append(newTargets);
    }

    packages->append(Utils::transform<QVector<McuPackage *> >(
                         tcPkgs.values(), [&](McuToolChainPackage *tcPkg) { return tcPkg; }));
    packages->append(vendorPkgs.values().toVector());
    packages->append(targetFactory.getMcuPackages());

    return  mcuTargets;
}

static QFileInfoList targetDescriptionFiles(const Utils::FilePath &dir)
{
    const QDir kitsDir(dir.toString() + "/kits/", "*.json");
    return kitsDir.entryInfoList();
}

static McuTargetDescription parseDescriptionJson(const QByteArray &data)
{
    const QJsonDocument document = QJsonDocument::fromJson(data);
    const QJsonObject target = document.object();
    const QJsonObject toolchain = target.value("toolchain").toObject();
    const QJsonObject boardSdk = target.value("boardSdk").toObject();
    const QJsonObject freeRTOS = target.value("freeRTOS").toObject();

    const QString platform = target.value("platform").toString();

    const QVariantList colorDepths = target.value("colorDepths").toArray().toVariantList();
    const auto colorDepthsVector = Utils::transform<QVector<int> >(
                colorDepths, [&](const QVariant &colorDepth) { return colorDepth.toInt(); });

    return {
        target.value("qulVersion").toString(),
        platform,
        target.value("platformVendor").toString(),
        colorDepthsVector,
        toolchain.value("id").toString(),
        boardSdk.value("envVar").toString(),
        boardSdk.value("name").toString(),
        boardSdk.value("defaultPath").toString(),
        freeRTOS.value("envVar").toString(),
        freeRTOS.value("boardSdkSubDir").toString()
    };
}

void targetsAndPackages(const Utils::FilePath &dir, QVector<McuPackage *> *packages,
                        QVector<McuTarget *> *mcuTargets)
{
    QList<McuTargetDescription> descriptions;

    for (const QFileInfo &fileInfo : targetDescriptionFiles(dir)) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QFile::ReadOnly))
            continue;
        const McuTargetDescription desc = parseDescriptionJson(file.readAll());
        if (QVersionNumber::fromString(desc.qulVersion) < McuSupportOptions::minimalQulVersion())
            return; // Invalid version means invalid SDK installation.
        descriptions.append(desc);
    }

    // Workaround for missing JSON file for Desktop target:
    if (dir.pathAppended("/lib/QulQuickUltralite_QT_32bpp_Windows_Release.lib").exists()) {
        const QString qulVersion = descriptions.empty() ?
                    McuSupportOptions::minimalQulVersion().toString()
                  : descriptions.first().qulVersion;
        descriptions.prepend({qulVersion, {"Qt"}, {"Qt"}, {32}, {"desktop"}, {}, {}, {}});
    }

    mcuTargets->append(targetsFromDescriptions(descriptions, packages));
}

} // namespace Sdk
} // namespace Internal
} // namespace McuSupport
