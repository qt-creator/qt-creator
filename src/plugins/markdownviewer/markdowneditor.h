// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace Markdown {
namespace Internal {

class MarkdownEditor : public TextEditor::TextEditorWidget
{
    Q_OBJECT
public:
    MarkdownEditor(QWidget *parent = nullptr);
};

} // namespace Internal
} // namespace Markdown
