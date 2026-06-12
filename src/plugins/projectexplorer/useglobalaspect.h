// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/aspects.h>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT UseGlobalAspect : public Utils::BoolAspect
{
public:
    explicit UseGlobalAspect(Utils::Id settingsPageId = {}, Utils::AspectContainer *container = nullptr);

    void setSettingsPageId(Utils::Id settingsPageId);

private:
    void addToLayoutImpl(Layouting::Layout &parent) override;

    Utils::Id m_settingsPageId;
};

} // namespace ProjectExplorer
