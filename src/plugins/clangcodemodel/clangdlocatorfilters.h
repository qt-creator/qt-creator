// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace ClangCodeModel::Internal {

class ClangdAllSymbolsFilter final : public Core::ILocatorFilter
{
public:
    ClangdAllSymbolsFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class ClangdClassesFilter final : public Core::ILocatorFilter
{
public:
    ClangdClassesFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class ClangdFunctionsFilter final : public Core::ILocatorFilter
{
public:
    ClangdFunctionsFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class ClangdCurrentDocumentFilter final : public Core::ILocatorFilter
{
public:
    ClangdCurrentDocumentFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

} // namespace ClangCodeModel::Internal
