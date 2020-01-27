// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textdocument.h>

namespace Markdown {
namespace Internal {

class MarkdownDocument : public TextEditor::TextDocument
{
    Q_OBJECT
public:
    MarkdownDocument();
};

} // namespace Internal
} // namespace Markdown
