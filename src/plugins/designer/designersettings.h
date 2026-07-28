// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Designer::Internal {

class DesignerSettings final : public Utils::AspectContainer
{
public:
    DesignerSettings();

    Utils::BoolAspect generatePointerToMemberConnections{this};
};

DesignerSettings &designerSettings();

void setupDesignerSettingsPage();

} // namespace Designer::Internal
