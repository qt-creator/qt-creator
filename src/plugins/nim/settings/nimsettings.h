// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Nim {

class NimSettings final : public Utils::AspectContainer
{
public:
    NimSettings();

    Utils::FilePathAspect nimSuggestPath{this};
};

NimSettings &settings();

} // Nim

