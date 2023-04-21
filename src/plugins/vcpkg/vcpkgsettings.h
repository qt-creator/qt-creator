// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/aspects.h>

namespace Vcpkg::Internal {

class VcpkgSettings : public Utils::AspectContainer
{
public:
    VcpkgSettings();

    static VcpkgSettings *instance();
    bool vcpkgRootValid() const;

    Utils::StringAspect vcpkgRoot;
};

class VcpkgSettingsPage final : public Core::IOptionsPage
{
public:
    VcpkgSettingsPage();
};

} // namespace Vcpkg::Internal
