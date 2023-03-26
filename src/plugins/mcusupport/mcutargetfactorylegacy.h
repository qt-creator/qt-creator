// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcuabstracttargetfactory.h"
#include "mcutargetdescription.h"
#include "settingshandler.h"

#include <functional>
#include <QHash>
#include <QPair>

namespace McuSupport::Internal {

class McuPackage;

namespace Legacy {

using ToolchainCompilerCreator = std::function<McuToolChainPackagePtr(const QStringList &version)>;

class McuTargetFactory : public McuAbstractTargetFactory
{
public:
    McuTargetFactory(const QHash<QString, ToolchainCompilerCreator> &toolchainCreators,
                     const QHash<QString, McuPackagePtr> &toolchainFiles,
                     const QHash<QString, McuPackagePtr> &vendorPkgs,
                     const SettingsHandler::Ptr &);

    QPair<Targets, Packages> createTargets(const McuTargetDescription &,
                                           const McuPackagePtr &qtForMCUsPackage) override;
    AdditionalPackages getAdditionalPackages() const override;

    McuToolChainPackagePtr getToolchainCompiler(const McuTargetDescription::Toolchain &) const;
    McuPackagePtr getToolchainFile(const Utils::FilePath &qtForMCUSdkPath,
                                   const QString &toolchainName) const;

private:
    QHash<QString, ToolchainCompilerCreator> toolchainCreators;
    const QHash<QString, McuPackagePtr> toolchainFiles;
    const QHash<QString, McuPackagePtr> vendorPkgs;

    SettingsHandler::Ptr settingsHandler;
}; // struct McuTargetFactory

} // namespace Legacy
} // namespace McuSupport::Internal
