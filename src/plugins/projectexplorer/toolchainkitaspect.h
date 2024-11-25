// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/id.h>

namespace ProjectExplorer {
class Abi;
class Kit;
class Toolchain;
class ToolchainBundle;

class PROJECTEXPLORER_EXPORT ToolchainKitAspect
{
public:
    static Utils::Id id();
    static QByteArray toolchainId(const Kit *k, Utils::Id language);
    static Toolchain *toolchain(const Kit *k, Utils::Id language);
    static Toolchain *cToolchain(const Kit *k);
    static Toolchain *cxxToolchain(const Kit *k);
    static QList<Toolchain *> toolChains(const Kit *k);
    static void setToolchain(Kit *k, Toolchain *tc);
    static void setBundle(Kit *k, const ToolchainBundle &bundle);
    static void clearToolchain(Kit *k, Utils::Id language);
    static Abi targetAbi(const Kit *k);

    static QString msgNoToolchainInTarget();
};

} // namespace ProjectExplorer
