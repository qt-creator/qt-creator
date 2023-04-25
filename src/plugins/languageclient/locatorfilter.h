// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <coreplugin/locator/ilocatorfilter.h>

namespace LanguageServerProtocol { class DocumentSymbol; };

namespace LanguageClient {

class Client;
class CurrentDocumentSymbolsData;

using DocSymbolModifier = std::function<void(Core::LocatorFilterEntry &,
    const LanguageServerProtocol::DocumentSymbol &, const Core::LocatorFilterEntry &)>;

Core::LocatorFilterEntries LANGUAGECLIENT_EXPORT currentDocumentSymbols(const QString &input,
    const CurrentDocumentSymbolsData &currentSymbolsData, const DocSymbolModifier &docSymbolModifier);

Core::LocatorMatcherTasks LANGUAGECLIENT_EXPORT languageClientMatchers(
    Core::MatcherType type, const QList<Client *> &clients = {}, int maxResultCount = 0);

class LanguageAllSymbolsFilter : public Core::ILocatorFilter
{
public:
    LanguageAllSymbolsFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class LanguageClassesFilter : public Core::ILocatorFilter
{
public:
    LanguageClassesFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class LanguageFunctionsFilter : public Core::ILocatorFilter
{
public:
    LanguageFunctionsFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

class LanguageCurrentDocumentFilter : public Core::ILocatorFilter
{
public:
    LanguageCurrentDocumentFilter();

private:
    Core::LocatorMatcherTasks matchers() final;
};

} // namespace LanguageClient
