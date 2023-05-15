// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Cppcheck::Internal {

class CppcheckTool;
class CppcheckTrigger;

class CppcheckOptions final : public Core::PagedSettings
{
public:
    CppcheckOptions();

    std::function<void(QWidget *widget)> layouter();

    Utils::StringAspect binary;
    Utils::BoolAspect warning;
    Utils::BoolAspect style;
    Utils::BoolAspect performance;
    Utils::BoolAspect portability;
    Utils::BoolAspect information;
    Utils::BoolAspect unusedFunction;
    Utils::BoolAspect missingInclude;
    Utils::BoolAspect inconclusive;
    Utils::BoolAspect forceDefines;

    Utils::StringAspect customArguments;
    Utils::StringAspect ignoredPatterns;
    Utils::BoolAspect showOutput;
    Utils::BoolAspect addIncludePaths;
    Utils::BoolAspect guessArguments;
};

} // Cppcheck::Internal
