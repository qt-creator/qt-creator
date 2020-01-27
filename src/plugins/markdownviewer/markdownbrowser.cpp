// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdownbrowser.h"

#include <QScrollBar>

namespace Markdown {
namespace Internal {

MarkdownBrowser::MarkdownBrowser(QWidget *parent)
    : QTextBrowser(parent)
{}

void MarkdownBrowser::setMarkdown(const QString &markdown)
{
    QHash<QScrollBar *, int> positions;
    const auto scrollBars = findChildren<QScrollBar *>();

    // save scroll positions
    for (const auto scrollBar : scrollBars)
        positions.insert(scrollBar, scrollBar->value());

    QTextBrowser::setMarkdown(markdown);

    // restore scroll positions
    for (auto scrollBar : scrollBars)
        scrollBar->setValue(positions.value(scrollBar));
}

} // namespace Internal
} // namespace Markdown
