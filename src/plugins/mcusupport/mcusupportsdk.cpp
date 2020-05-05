/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include <utils/hostosinfo.h>
#include <utils/fileutils.h>

#include <QDir>

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
                McuPackage::tr("Qt for MCUs SDK"),
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
    result->setDownloadUrl(
                "https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads");
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

static McuPackage *createStm32CubeFwF7SdkPackage()
{
    auto result = new McuPackage(
                McuPackage::tr("STM32Cube SDK"),
                "%{Env:STM32Cube_FW_F7_SDK_PATH}",
                "Drivers/STM32F7xx_HAL_Driver",
                "Stm32CubeFwF7Sdk");
    result->setDownloadUrl(
                "https://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubef7.html");
    result->setEnvironmentVariableName("STM32Cube_FW_F7_SDK_PATH");
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

static McuPackage *createEvkbImxrt1050SdkPackage()
{
    auto result = new McuPackage(
                McuPackage::tr("NXP i.MXRT SDK"),
                "%{Env:EVKB_IMXRT1050_SDK_PATH}", // TODO: Try to not use 1050 specifics
                "EVKB-IMXRT1050_manifest_v3_5.xml",
                "EvkbImxrt1050Sdk");
    result->setDownloadUrl("https://mcuxpresso.nxp.com/en/welcome");
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

static McuPackage *createFreeRTOSSourcesPackage(const QString &envVarPrefix)
{
    const QString envVar = envVarPrefix + "_FREERTOS_DIR";

    const QString defaultPath =
            qEnvironmentVariableIsSet(envVar.toLatin1()) ?
                qEnvironmentVariable(envVar.toLatin1()) : QDir::homePath();

    auto result = new McuPackage(
                QString::fromLatin1("FreeRTOS Sources (%1)").arg(envVarPrefix),
                defaultPath,
                {},
                QString::fromLatin1("FreeRTOSSourcePackage_%1").arg(envVarPrefix));
    result->setDownloadUrl("https://freertos.org");
    result->setEnvironmentVariableName(envVar);
    return result;
}

void hardcodedTargetsAndPackages(const Utils::FilePath &dir, QVector<McuPackage *> *packages,
                                 QVector<McuTarget *> *mcuTargets)
{
    McuToolChainPackage* armGccPackage = Sdk::createArmGccPackage();
    McuToolChainPackage *ghsToolchainPackage = createGhsToolchainPackage();
    McuToolChainPackage* desktopToolChainPackage = createDesktopToolChainPackage();
    McuPackage* stm32CubeFwF7SdkPackage = Sdk::createStm32CubeFwF7SdkPackage();
    McuPackage* stm32CubeProgrammerPackage = Sdk::createStm32CubeProgrammerPackage();
    McuPackage* evkbImxrt1050SdkPackage = Sdk::createEvkbImxrt1050SdkPackage();
    McuPackage *mcuXpressoIdePackage = createMcuXpressoIdePackage();
    McuPackage *rglPackage = createRGLPackage();
    McuPackage *freeRTOSSTM32F7Package = createFreeRTOSSourcesPackage("STM32F7");
    McuPackage *freeRTOSIMXRT1050Package = createFreeRTOSSourcesPackage("IMXRT1050");
    McuPackage *freeRTOSIMXRT1064Package = createFreeRTOSSourcesPackage("IMXRT1064");

    QVector<McuPackage*> stmEvalPackages = {
        armGccPackage, stm32CubeProgrammerPackage};
    QVector<McuPackage*> nxpEvalPackages = {
        armGccPackage, mcuXpressoIdePackage};
    QVector<McuPackage*> renesasEvalPackages = {
        ghsToolchainPackage, rglPackage};
    QVector<McuPackage*> desktopPackages = {};
    *packages = {
        armGccPackage, desktopToolChainPackage, ghsToolchainPackage,
        stm32CubeFwF7SdkPackage, stm32CubeProgrammerPackage, evkbImxrt1050SdkPackage,
        mcuXpressoIdePackage, rglPackage,
        freeRTOSSTM32F7Package, freeRTOSIMXRT1050Package, freeRTOSIMXRT1064Package};

    const QString vendorStm = "STM";
    const QString vendorNxp = "NXP";
    const QString vendorQt = "Qt";
    const QString vendorRenesas = "Renesas";

    const struct {
        const QString &vendor;
        const QString qulPlatform;
        const QVector<McuPackage*> &packages;
        McuToolChainPackage *toolchainPackage;
        McuPackage *freeRTOSPackage;
        const QVector<int> colorDepths;
    } targets[] = {
        {vendorNxp, {"MIMXRT1050-EVK"}, nxpEvalPackages, armGccPackage,
                freeRTOSIMXRT1050Package, {16}},
        {vendorNxp, {"MIMXRT1064-EVK"}, nxpEvalPackages, armGccPackage,
                freeRTOSIMXRT1064Package, {16}},
        {vendorQt, {"Qt"}, desktopPackages, desktopToolChainPackage,
                nullptr, {32}},
        {vendorRenesas, {"RH850-D1M1A"}, renesasEvalPackages, ghsToolchainPackage,
                nullptr, {32}},
        {vendorStm, {"STM32F469I-DISCOVERY"}, stmEvalPackages, armGccPackage,
                nullptr, {24}},
        {vendorStm, {"STM32F7508-DISCOVERY"}, stmEvalPackages, armGccPackage,
                freeRTOSSTM32F7Package, {32, 16}},
        {vendorStm, {"STM32F769I-DISCOVERY"}, stmEvalPackages, armGccPackage,
                freeRTOSSTM32F7Package, {32}},
        {vendorStm, {"STM32H750B-DISCOVERY"}, stmEvalPackages, armGccPackage,
                nullptr, {32}},
        {vendorStm, {"STM32L4R9I-DISCOVERY"}, stmEvalPackages, armGccPackage,
                nullptr, {24}},
        {vendorStm, {"STM32L4R9I-EVAL"}, stmEvalPackages, armGccPackage,
                nullptr, {24}}
    };

    const QString QulTargetTemplate =
            dir.toString() + "/lib/cmake/Qul/QulTargets/QulTargets_%1_%2.cmake";
    for (const auto target : targets) {
        for (auto os : {McuTarget::OS::Desktop, McuTarget::OS::BareMetal,
                        McuTarget::OS::FreeRTOS}) {
            for (int colorDepth : target.colorDepths) {
                QVector<McuPackage*> required3rdPartyPackages = target.packages;
                if (os == McuTarget::OS::FreeRTOS) {
                    if (target.freeRTOSPackage)
                        required3rdPartyPackages.append(target.freeRTOSPackage);
                    else
                        continue;
                } else if (os == McuTarget::OS::Desktop && target.toolchainPackage->type()
                        != McuToolChainPackage::TypeDesktop) {
                    continue;
                }

                const QString QulTarget =
                        QulTargetTemplate.arg(target.qulPlatform, QString::number(colorDepth));
                if (!Utils::FilePath::fromUserInput(QulTarget).exists())
                    continue;
                auto mcuTarget = new McuTarget(target.vendor, target.qulPlatform, os,
                                               required3rdPartyPackages, target.toolchainPackage);
                if (target.colorDepths.count() > 1)
                    mcuTarget->setColorDepth(colorDepth);
                mcuTargets->append(mcuTarget);
            }
        }
    }
}

} // namespace Sdk
} // namespace Internal
} // namespace McuSupport
