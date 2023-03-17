// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

namespace LanguageClient { class WorkspaceLocatorFilter; }

namespace ClangCodeModel {
namespace Internal {

class ClangGlobalSymbolFilter : public Core::ILocatorFilter
{
public:
    ClangGlobalSymbolFilter();
    ClangGlobalSymbolFilter(Core::ILocatorFilter *cppFilter,
                            Core::ILocatorFilter *lspFilter);
    ~ClangGlobalSymbolFilter() override;

private:
    void prepareSearch(const QString &entry) override;
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    Core::ILocatorFilter * const m_cppFilter;
    Core::ILocatorFilter * const m_lspFilter;
};

class ClangClassesFilter : public ClangGlobalSymbolFilter
{
public:
    ClangClassesFilter();
};

class ClangFunctionsFilter : public ClangGlobalSymbolFilter
{
public:
    ClangFunctionsFilter();
};

class ClangdCurrentDocumentFilter : public Core::ILocatorFilter
{
public:
    ClangdCurrentDocumentFilter();
    ~ClangdCurrentDocumentFilter() override;

private:
    void prepareSearch(const QString &entry) override;
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    class Private;
    Private * const d;
};

} // namespace Internal
} // namespace ClangCodeModel
