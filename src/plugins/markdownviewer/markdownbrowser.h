// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QTextBrowser>

namespace Core { class IDocument; }

namespace Markdown {
namespace Internal {

class MarkdownBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit MarkdownBrowser(QWidget *parent = nullptr);

public slots:
    void setMarkdown(const QString &markdown);
};

} // namespace Internal
} // namespace Markdown
