// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mcutargetfactory.h"
#include "mcuhelpers.h"
#include "mcupackage.h"
#include "mcusupportconstants.h"
#include "mcusupportversiondetection.h"
#include "mcutarget.h"
#include "mcutargetdescription.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QVersionNumber>

namespace McuSupport::Internal {

bool isToolchainDescriptionValid(const McuTargetDescription::Toolchain &t)
{
    return !t.id.isEmpty() && !t.compiler.cmakeVar.isEmpty() && !t.file.cmakeVar.isEmpty();
}

bool isDesktopToolchain(McuToolChainPackage::ToolChainType type)
{
    return type == McuToolChainPackage::ToolChainType::MSVC
           || type == McuToolChainPackage::ToolChainType::GCC;
}

McuPackageVersionDetector *createVersionDetection(const VersionDetection &versionDetection)
{
    if (!versionDetection.xmlElement.isEmpty() && !versionDetection.xmlAttribute.isEmpty())
        return new McuPackageXmlVersionDetector{versionDetection.filePattern,
                                                versionDetection.xmlElement,
                                                versionDetection.xmlAttribute,
                                                versionDetection.regex};
    else if (!versionDetection.executableArgs.isEmpty())
        return new McuPackageExecutableVersionDetector{Utils::FilePath::fromUserInput(
                                                           versionDetection.filePattern),
                                                       QStringList{versionDetection.executableArgs},
                                                       versionDetection.regex};
    else
        return new McuPackageDirectoryVersionDetector(versionDetection.filePattern,
                                                      versionDetection.regex,
                                                      versionDetection.isFile);
}

static void evaluateVariables(McuTarget &target)
{
    const static QRegularExpression variableRegex{R"(\${*\w+}*)",
                                                  QRegularExpression::CaseInsensitiveOption};

    for (const auto &package : target.packages()) {
        const QRegularExpressionMatch match{variableRegex.match(package->path().toString())};
        if (!match.hasMatch())
            continue;
        const QString variable{match.captured(0).remove(0, 1)};

        McuPackagePtr packageDefiningVariable{
            Utils::findOrDefault(target.packages(), [variable](const McuPackagePtr &pkg) {
                return pkg->cmakeVariableName() == variable
                       || pkg->environmentVariableName() == variable;
            })};

        if (packageDefiningVariable == nullptr) // nothing provides the variable
            continue;

        const auto evaluatedPath{Utils::FilePath::fromUserInput(
            package->path().toString().replace(match.capturedStart(),
                                               match.capturedLength(),
                                               packageDefiningVariable->path().toString()))};
        package->setPath(evaluatedPath);
    }
}

McuTargetFactory::McuTargetFactory(const SettingsHandler::Ptr &settingsHandler)
    : settingsHandler{settingsHandler}
{}

QPair<Targets, Packages> McuTargetFactory::createTargets(const McuTargetDescription &desc,
                                                         const Utils::FilePath & /*qtForMCUSdkPath*/)
{
    Targets mcuTargets;
    Packages packages;

    for (int colorDepth : desc.platform.colorDepths) {
        const McuTarget::Platform platform(
            {desc.platform.id, desc.platform.name, desc.platform.vendor});

        auto *toolchain = createToolchain(desc.toolchain);
        McuPackagePtr toolchainFile{createPackage(desc.toolchain.file)};
        //Skip target with incorrect toolchain dir or toolchain file.
        if (!toolchain || !toolchainFile)
            continue;
        Packages targetPackages = createPackages(desc);
        McuToolChainPackagePtr toolchainPtr{toolchain};
        targetPackages.insert({toolchainPtr});
        targetPackages.unite({toolchainFile});
        packages.unite(targetPackages);

        McuTargetPtr target{new McuTarget{QVersionNumber::fromString(desc.qulVersion),
                                          platform,
                                          deduceOperatingSystem(desc),
                                          targetPackages,
                                          toolchainPtr,
                                          toolchainFile,
                                          colorDepth}};

        evaluateVariables(*target);
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
                                        pkgDesc.validationPath,
                                        pkgDesc.setting,
                                        pkgDesc.cmakeVar,
                                        pkgDesc.envVar,
                                        pkgDesc.versions,
                                        {},
                                        createVersionDetection(pkgDesc.versionDetection)}};
}

McuToolChainPackage *McuTargetFactory::createToolchain(
    const McuTargetDescription::Toolchain &toolchain)
{
    const static QMap<QString, McuToolChainPackage::ToolChainType> toolchainTypeMapping{
        {"iar", McuToolChainPackage::ToolChainType::IAR},
        {"keil", McuToolChainPackage::ToolChainType::KEIL},
        {"msvc", McuToolChainPackage::ToolChainType::MSVC},
        {"gcc", McuToolChainPackage::ToolChainType::GCC},
        {"armgcc", McuToolChainPackage::ToolChainType::ArmGcc},
        {"ghs", McuToolChainPackage::ToolChainType::GHS},
        {"ghsarm", McuToolChainPackage::ToolChainType::GHSArm},
    };

    const PackageDescription compilerDescription{toolchain.compiler};

    McuToolChainPackage::ToolChainType toolchainType
        = toolchainTypeMapping.value(toolchain.id, McuToolChainPackage::ToolChainType::Unsupported);

    if (isDesktopToolchain(toolchainType)) {
        return new McuToolChainPackage{settingsHandler,
                                       compilerDescription.label,
                                       compilerDescription.defaultPath,
                                       compilerDescription.validationPath,
                                       {},
                                       toolchainType,
                                       toolchain.versions,
                                       compilerDescription.cmakeVar,
                                       {},
                                       createVersionDetection(compilerDescription.versionDetection)};
    } else if (!isToolchainDescriptionValid(toolchain))
        toolchainType = McuToolChainPackage::ToolChainType::Unsupported;

    return new McuToolChainPackage{settingsHandler,
                                   compilerDescription.label,
                                   compilerDescription.defaultPath,
                                   compilerDescription.validationPath,
                                   compilerDescription.setting,
                                   toolchainType,
                                   toolchain.versions,
                                   compilerDescription.cmakeVar,
                                   compilerDescription.envVar,
                                   createVersionDetection(compilerDescription.versionDetection)};
}

} // namespace McuSupport::Internal
