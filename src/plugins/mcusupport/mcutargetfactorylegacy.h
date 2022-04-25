/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#pragma once

#include "mcuabstracttargetfactory.h"
#include "mcutargetdescription.h"
#include "settingshandler.h"

#include <functional>
#include <QHash>
#include <QPair>

namespace McuSupport::Internal {

class McuPackage;

using ToolchainCompilerCreator = std::function<McuToolChainPackagePtr()>;

class McuTargetFactoryLegacy : public McuAbstractTargetFactory
{
public:
    McuTargetFactoryLegacy(const QHash<QString, ToolchainCompilerCreator> &toolchainCreators,
                           const QHash<QString, McuPackagePtr> &toolchainFiles,
                           const QHash<QString, McuPackagePtr> &vendorPkgs,
                           const SettingsHandler::Ptr &);

    QPair<Targets, Packages> createTargets(const Sdk::McuTargetDescription &,
                                           const Utils::FilePath &qtForMCUSdkPath) override;
    AdditionalPackages getAdditionalPackages() const override;

    McuToolChainPackagePtr getToolchainCompiler(const Sdk::McuTargetDescription::Toolchain &) const;
    McuPackagePtr getToolchainFile(const Utils::FilePath &qtForMCUSdkPath,
                                   const QString &toolchainName) const;

private:
    QHash<QString, ToolchainCompilerCreator> toolchainCreators;
    const QHash<QString, McuPackagePtr> toolchainFiles;
    const QHash<QString, McuPackagePtr> vendorPkgs;

    SettingsHandler::Ptr settingsHandler;
}; // struct McuTargetFactoryLegacy

} // namespace McuSupport::Internal
