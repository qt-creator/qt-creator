// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textsuggestion.h"

#include "textdocumentlayout.h"

namespace TextEditor {

TextSuggestion::TextSuggestion()
{
    m_replacementDocument.setDocumentLayout(new TextDocumentLayout(&m_replacementDocument));
    m_replacementDocument.setDocumentMargin(0);
}

TextSuggestion::~TextSuggestion() = default;

} // namespace TextEditor
