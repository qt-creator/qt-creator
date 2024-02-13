// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcuhelpers.h"
#include "mcupackage.h"
#include "mcusupport_global.h"
#include "mcusupportplugin.h"
#include "mcusupporttr.h"
#include "mcusupportversiondetection.h"
#include "mcutarget.h"
#include "mcutargetdescription.h"
#include "mcutargetfactory.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QVersionNumber>

namespace McuSupport::Internal {

bool isDesktopToolchain(McuToolchainPackage::ToolchainType type)
{
    return type == McuToolchainPackage::ToolchainType::MSVC
           || type == McuToolchainPackage::ToolchainType::GCC
           || type == McuToolchainPackage::ToolchainType::MinGW;
}

McuPackageVersionDetector *createVersionDetection(const VersionDetection &versionDetection)
{
    if (!versionDetection.xmlElement.isEmpty() && !versionDetection.xmlAttribute.isEmpty())
        return new McuPackageXmlVersionDetector{versionDetection.filePattern,
                                                versionDetection.xmlElement,
                                                versionDetection.xmlAttribute,
                                                versionDetection.regex};
    else if (!versionDetection.executableArgs.isEmpty())
        return new McuPackageExecutableVersionDetector{{Utils::FilePath::fromUserInput(
                                                           versionDetection.filePattern)},
                                                       QStringList{versionDetection.executableArgs},
                                                       versionDetection.regex};
    else if (!versionDetection.filePattern.isEmpty() && !versionDetection.regex.isEmpty())
        return new McuPackageDirectoryEntriesVersionDetector(versionDetection.filePattern,
                                                             versionDetection.regex);
    else if (!versionDetection.regex.isEmpty())
        return new McuPackagePathVersionDetector(versionDetection.regex);
    else {
        // In this case the JSON entry is either invalid or missing.
        // After refactoring, this should raise a JSON error to the user.
        return nullptr;
    }
}

static void removeEmptyPackages(Packages &packages)
{
    Packages::const_iterator it = packages.constBegin();
    while (it != packages.constEnd()) {
        const auto &pkg = *it;
        if (pkg->cmakeVariableName().isEmpty() && pkg->path().isEmpty()) {
            it = packages.erase(it);
        } else {
            ++it;
        }
    }
}

McuTargetFactory::McuTargetFactory(const SettingsHandler::Ptr &settingsHandler)
    : settingsHandler{settingsHandler}
{}

QPair<Targets, Packages> McuTargetFactory::createTargets(const McuTargetDescription &desc,
                                                         const McuPackagePtr &qtForMCUsPackage)
{
    Targets mcuTargets;
    Packages packages;

    for (int colorDepth : desc.platform.colorDepths) {
        const McuTarget::Platform platform(
            {desc.platform.id, desc.platform.name, desc.platform.vendor});

        auto *toolchain = createToolchain(desc.toolchain, desc.sourceFile);
        McuPackagePtr toolchainFile{createPackage(desc.toolchain.file)};
        //Skip target with incorrect toolchain dir or toolchain file.
        if (!toolchain || !toolchainFile)
            continue;
        Packages targetPackages = createPackages(desc);
        McuToolchainPackagePtr toolchainPtr{toolchain};
        targetPackages.insert({toolchainPtr});
        targetPackages.insert({qtForMCUsPackage});
        targetPackages.unite({toolchainFile});

        removeEmptyPackages(targetPackages);

        packages.unite(targetPackages);

        McuTargetPtr target{new McuTarget{QVersionNumber::fromString(desc.qulVersion),
                                          platform,
                                          deduceOperatingSystem(desc),
                                          targetPackages,
                                          toolchainPtr,
                                          toolchainFile,
                                          colorDepth}};

        mcuTargets.append(target);
    }
    return {mcuTargets, packages};
}

QList<PackageDescription> aggregatePackageEntries(const McuTargetDescription &desc)
{
    QList<PackageDescription> result;
    result.append(desc.platform.entries);
    result.append(desc.boardSdk);
    result.append(desc.freeRTOS.package);
    return result;
}

Packages McuTargetFactory::createPackages(const McuTargetDescription &desc)
{
    Packages packages;
    QList<PackageDescription> packageDescriptions = aggregatePackageEntries(desc);

    for (const PackageDescription &pkgDesc : packageDescriptions) {
        packages.insert(createPackage(pkgDesc));
    }

    return packages;
}

McuPackagePtr McuTargetFactory::createPackage(const PackageDescription &pkgDesc)
{
    return McuPackagePtr{new McuPackage{settingsHandler,
                                        pkgDesc.label,
                                        pkgDesc.defaultPath,
                                        pkgDesc.detectionPaths,
                                        pkgDesc.setting,
                                        pkgDesc.cmakeVar,
                                        pkgDesc.envVar,
                                        pkgDesc.versions,
                                        {},
                                        createVersionDetection(pkgDesc.versionDetection),
                                        pkgDesc.shouldAddToSystemPath,
                                        pkgDesc.type}};
}

McuToolchainPackage *McuTargetFactory::createToolchain(
    const McuTargetDescription::Toolchain &toolchain,
    const Utils::FilePath &sourceFile
    )
{
    const static QMap<QString, McuToolchainPackage::ToolchainType> toolchainTypeMapping{
        {"iar", McuToolchainPackage::ToolchainType::IAR},
        {"keil", McuToolchainPackage::ToolchainType::KEIL},
        {"msvc", McuToolchainPackage::ToolchainType::MSVC},
        {"gcc", McuToolchainPackage::ToolchainType::GCC},
        {"mingw", McuToolchainPackage::ToolchainType::MinGW},
        {"armgcc", McuToolchainPackage::ToolchainType::ArmGcc},
        {"greenhills", McuToolchainPackage::ToolchainType::GHS},
        {"arm-greenhills", McuToolchainPackage::ToolchainType::GHSArm},
    };

    const PackageDescription compilerDescription{toolchain.compiler};

    McuToolchainPackage::ToolchainType toolchainType
        = toolchainTypeMapping.value(toolchain.id, McuToolchainPackage::ToolchainType::Unsupported);

    if (isDesktopToolchain(toolchainType)) {
        return new McuToolchainPackage{settingsHandler,
                                       compilerDescription.label,
                                       compilerDescription.defaultPath,
                                       compilerDescription.detectionPaths,
                                       {},
                                       toolchainType,
                                       toolchain.versions,
                                       compilerDescription.cmakeVar,
                                       {},
                                       createVersionDetection(compilerDescription.versionDetection)};
    }

    // Validate toolchain and provide a proper error message
    QString errorMessage;

    if (toolchain.id.isEmpty()) {
        errorMessage = Tr::tr("the toolchain.id JSON entry is empty");
    } else if (!toolchainTypeMapping.contains(toolchain.id)) {
        errorMessage = Tr::tr("the given toolchain \"%1\" is not supported").arg(toolchain.id);
    } else if (toolchain.compiler.cmakeVar.isEmpty()) {
        errorMessage = Tr::tr("the toolchain.compiler.cmakeVar JSON entry is empty");
    } else if (toolchain.file.cmakeVar.isEmpty()) {
        errorMessage = Tr::tr("the toolchain.file.cmakeVar JSON entry is empty");
    }

    if (!errorMessage.isEmpty()) {
        toolchainType = McuToolchainPackage::ToolchainType::Unsupported;

        if (toolchain.id.isEmpty()) {
            printMessage(Tr::tr("Toolchain is invalid because %2 in file \"%3\".")
                         .arg(errorMessage).arg(sourceFile.toUserOutput()),
                         true);
        } else {
            printMessage(Tr::tr("Toolchain description for \"%1\" is invalid because %2 in file \"%3\".")
                         .arg(toolchain.id).arg(errorMessage).arg(sourceFile.toUserOutput()),
                         true);
        }

    }

    return new McuToolchainPackage{settingsHandler,
                                   compilerDescription.label,
                                   compilerDescription.defaultPath,
                                   compilerDescription.detectionPaths,
                                   compilerDescription.setting,
                                   toolchainType,
                                   toolchain.versions,
                                   compilerDescription.cmakeVar,
                                   compilerDescription.envVar,
                                   createVersionDetection(compilerDescription.versionDetection)};
}

} // namespace McuSupport::Internal
