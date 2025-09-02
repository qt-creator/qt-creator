// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <coreplugin/locator/ilocatorfilter.h>

namespace CppEditor {

Core::LocatorMatcherTasks CPPEDITOR_EXPORT cppMatchers(Core::MatcherType type);

class CppAllSymbolsFilter final : public Core::ILocatorFilter
{
public:
    CppAllSymbolsFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class CppClassesFilter final : public Core::ILocatorFilter
{
public:
    CppClassesFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class CppFunctionsFilter final : public Core::ILocatorFilter
{
public:
    CppFunctionsFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class CppCurrentDocumentFilter final : public  Core::ILocatorFilter
{
public:
    CppCurrentDocumentFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

} // namespace CppEditor
