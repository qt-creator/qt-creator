// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/minisplitter.h>
#include <utils/fileutils.h>

namespace Core { class IDocument; }

namespace Markdown {
namespace Internal {

class MarkdownViewerWidget : public Core::MiniSplitter
{
    Q_OBJECT
    Q_PROPERTY(bool editorVisible READ isEditorVisible WRITE setEditorVisible NOTIFY editorVisibleChanged)
public:
    explicit MarkdownViewerWidget();
    ~MarkdownViewerWidget() override;

    Core::IDocument *document() const;

    bool isEditorVisible() const;

public slots:
    void setEditorVisible(bool editorVisible);

signals:
    void editorVisibleChanged(bool editorVisible);

private:
    struct MarkdownViewerWidgetPrivate *d;
};

} // namespace Internal
} // namespace Markdown
