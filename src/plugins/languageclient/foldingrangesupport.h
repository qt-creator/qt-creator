// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

namespace TextEditor {
class BaseTextEditor;
class TextDocument;
}

namespace LanguageClient {
class Client;

namespace Internal {

class FoldingRangeSupportImpl;

class FoldingRangeSupport
{
public:
    FoldingRangeSupport(Client *client);
    ~FoldingRangeSupport();

    void requestFoldingRanges(TextEditor::TextDocument *doc);
    void foldOrUnfoldCommentBlocks(TextEditor::BaseTextEditor *editor, bool fold);
    void deactivate(TextEditor::TextDocument *doc);
    void refresh();

private:
    FoldingRangeSupportImpl * const d;
};

} // namespace Internal
} // namespace LanguageClient
