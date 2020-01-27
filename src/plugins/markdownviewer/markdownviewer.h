// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

namespace Markdown {
namespace Internal {
class MarkdownDocument;

class MarkdownViewer : public Core::IEditor
{
    Q_OBJECT
public:
    explicit MarkdownViewer();
    ~MarkdownViewer() override;

    Core::IDocument *document() const override;
    QWidget *toolBar() override;

private:
    void ctor();

    struct MarkdownViewerPrivate *d;
};

} // namespace Internal
} // namespace Markdown
