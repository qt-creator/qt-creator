// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Haskell::Internal {

class HaskellSettings final : public Utils::AspectContainer
{
public:
    HaskellSettings();

    Utils::FilePathAspect stackPath{this};
};

HaskellSettings &settings();

} // Haskell::Internal
