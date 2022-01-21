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

#pragma once

#include "toolchain.h"
#include "abi.h"

#include <QList>
#include <QSet>

#include <functional>

namespace ProjectExplorer {
class ClangToolChain;
class GccToolChain;

namespace Internal {

class GccToolChainFactory : public ToolChainFactory
{
public:
    GccToolChainFactory();

    Toolchains autoDetect(const ToolchainDetector &detector) const override;
    Toolchains detectForImport(const ToolChainDescription &tcd) const override;

protected:
    enum class DetectVariants { Yes, No };
    using ToolchainChecker = std::function<bool(const ToolChain *)>;
    Toolchains autoDetectToolchains(
            const QString &compilerName, DetectVariants detectVariants, const Utils::Id language,
            const Utils::Id requiredTypeId, const ToolchainDetector &detector,
            const ToolchainChecker &checker = {}) const;
    Toolchains autoDetectToolChain(
            const ToolChainDescription &tcd,
            const ToolchainChecker &checker = {}) const;
};

// --------------------------------------------------------------------------
// ClangToolChainFactory
// --------------------------------------------------------------------------

class ClangToolChainFactory : public GccToolChainFactory
{
public:
    ClangToolChainFactory();

    Toolchains autoDetect(const ToolchainDetector &detector) const final;
    Toolchains detectForImport(const ToolChainDescription &tcd) const final;
};

// --------------------------------------------------------------------------
// MingwToolChainFactory
// --------------------------------------------------------------------------

class MingwToolChainFactory : public GccToolChainFactory
{
public:
    MingwToolChainFactory();

    Toolchains autoDetect(const ToolchainDetector &detector) const final;
    Toolchains detectForImport(const ToolChainDescription &tcd) const final;
};

// --------------------------------------------------------------------------
// LinuxIccToolChainFactory
// --------------------------------------------------------------------------

class LinuxIccToolChainFactory : public GccToolChainFactory
{
public:
    LinuxIccToolChainFactory();

    Toolchains autoDetect(const ToolchainDetector &detector) const final;
    Toolchains detectForImport(const ToolChainDescription &tcd) const final;
};

} // namespace Internal
} // namespace ProjectExplorer
