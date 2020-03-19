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
                "QtForMCUsSdk");
    result->setEnvironmentVariableName("Qul_DIR");
    return result;
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

static McuPackage *createSeggerJLinkPackage()
{
    QString defaultPath = QString("%{Env:SEGGER_JLINK_SOFTWARE_AND_DOCUMENTATION_PATH}");
    if (Utils::HostOsInfo::isWindowsHost()) {
        const QString programPath = findInProgramFiles("/SEGGER/JLink");
        if (!programPath.isEmpty())
            defaultPath = programPath;
    }
    auto result = new McuPackage(
                McuPackage::tr("SEGGER JLink"),
                defaultPath,
                Utils::HostOsInfo::withExecutableSuffix("JLink"),
                "SeggerJLink");
    result->setDownloadUrl("https://www.segger.com/downloads/jlink");
    result->setEnvironmentVariableName("SEGGER_JLINK_SOFTWARE_AND_DOCUMENTATION_PATH");
    return result;
}

void hardcodedTargetsAndPackages(const Utils::FilePath &dir, QVector<McuPackage *> *packages,
                                 QVector<McuTarget *> *mcuTargets)
{
    McuToolChainPackage* armGccPackage = Sdk::createArmGccPackage();
    McuPackage* stm32CubeFwF7SdkPackage = Sdk::createStm32CubeFwF7SdkPackage();
    McuPackage* stm32CubeProgrammerPackage = Sdk::createStm32CubeProgrammerPackage();
    McuPackage* evkbImxrt1050SdkPackage = Sdk::createEvkbImxrt1050SdkPackage();
    McuPackage* seggerJLinkPackage = Sdk::createSeggerJLinkPackage();

    QVector<McuPackage*> stmEvalPackages = {
        armGccPackage, stm32CubeProgrammerPackage};
    QVector<McuPackage*> nxpEvalPackages = {
        armGccPackage, seggerJLinkPackage};
    QVector<McuPackage*> renesasEvalPackages = {
        armGccPackage, seggerJLinkPackage};
    QVector<McuPackage*> desktopPackages = {};
    *packages = {
        armGccPackage, stm32CubeFwF7SdkPackage, stm32CubeProgrammerPackage, evkbImxrt1050SdkPackage,
        seggerJLinkPackage};

    const QString vendorStm = "STM";
    const QString vendorNxp = "NXP";
    const QString vendorQt = "Qt";
    const QString vendorRenesas = "Renesas";

    const struct {
        const QString &vendor;
        const QString qulPlatform;
        const QVector<McuPackage*> &packages;
        McuToolChainPackage *toolchainPackage;
        const QVector<int> colorDepths;
    } targets[] = {
        {vendorNxp, {"MIMXRT1050-EVK"}, nxpEvalPackages, armGccPackage, {16}},
        {vendorNxp, {"MIMXRT1064-EVK"}, nxpEvalPackages, armGccPackage, {16}},
        {vendorQt, {"Qt"}, desktopPackages, nullptr, {32}},
        {vendorRenesas, {"RH850-D1M1A"}, renesasEvalPackages, armGccPackage, {32}},
        {vendorStm, {"STM32F469I-DISCOVERY"}, stmEvalPackages, armGccPackage, {24}},
        {vendorStm, {"STM32F7508-DISCOVERY"}, stmEvalPackages, armGccPackage, {32, 16}},
        {vendorStm, {"STM32F769I-DISCOVERY"}, stmEvalPackages, armGccPackage, {32}},
        {vendorStm, {"STM32H750B-DISCOVERY"}, stmEvalPackages, armGccPackage, {32}},
        {vendorStm, {"STM32L4R9I-DISCOVERY"}, stmEvalPackages, armGccPackage, {24}},
        {vendorStm, {"STM32L4R9I-EVAL"}, stmEvalPackages, armGccPackage, {24}}
    };

    const QString QulTargetTemplate =
            dir.toString() + "/lib/cmake/Qul/QulTargets/QulTargets_%1_%2.cmake";
    for (auto target : targets) {
        for (int colorDepth : target.colorDepths) {
            const QString QulTarget =
                    QulTargetTemplate.arg(target.qulPlatform, QString::number(colorDepth));
            if (!Utils::FilePath::fromUserInput(QulTarget).exists())
                continue;
            auto mcuTarget = new McuTarget(target.vendor, target.qulPlatform, target.packages,
                                           target.toolchainPackage);
            if (target.colorDepths.count() > 1)
                mcuTarget->setColorDepth(colorDepth);
            mcuTargets->append(mcuTarget);
        }
    }
}

} // namespace Sdk
} // namespace Internal
} // namespace McuSupport
