// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/aspects.h>

namespace Core::Internal {

class EnvVarSeparatorAspect : public Utils::StringListAspect
{
public:
    EnvVarSeparatorAspect(Utils::AspectContainer *container = nullptr);

    void addToLayoutImpl(Layouting::Layout &parent) override;

    void writeSettings() const override { Utils::StringListAspect::writeSettings(); }
    void readSettings() override { Utils::StringListAspect::readSettings(); }
};

} // namespace Core::Internal
