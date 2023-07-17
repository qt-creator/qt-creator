// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

#include <functional>

namespace Cppcheck::Internal {

class CppcheckSettings final : public Utils::AspectContainer
{
public:
    CppcheckSettings();

    std::function<Layouting::LayoutItem()> layouter();

    Utils::FilePathAspect binary{this};
    Utils::BoolAspect warning{this};
    Utils::BoolAspect style{this};
    Utils::BoolAspect performance{this};
    Utils::BoolAspect portability{this};
    Utils::BoolAspect information{this};
    Utils::BoolAspect unusedFunction{this};
    Utils::BoolAspect missingInclude{this};
    Utils::BoolAspect inconclusive{this};
    Utils::BoolAspect forceDefines{this};

    Utils::StringAspect customArguments{this};
    Utils::StringAspect ignoredPatterns{this};
    Utils::BoolAspect showOutput{this};
    Utils::BoolAspect addIncludePaths{this};
    Utils::BoolAspect guessArguments{this};
};

CppcheckSettings &settings();


} // Cppcheck::Internal
