// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ClangFormat {

class ClangFormatSettings final : public Utils::AspectContainer
{
public:
    ClangFormatSettings();

    enum Mode {
        Indenting = 0,
        Formatting,
        Disable
    };

    Utils::TypedSelectionAspect<Mode> mode{this};
    Utils::BoolAspect useCustomSettings{this};
    Utils::BoolAspect formatWhileTyping{this};
    Utils::BoolAspect formatOnSave{this};
    Utils::IntegerAspect fileSizeThreshold{this};
};

ClangFormatSettings &clangFormatSettings();

} // namespace ClangFormat
