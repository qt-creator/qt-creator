// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <texteditor/snippets/snippetparser.h>

namespace LanguageClient {

LANGUAGECLIENT_EXPORT TextEditor::SnippetParseResult parseSnippet(const QString &snippet);

} // namespace LanguageClient
