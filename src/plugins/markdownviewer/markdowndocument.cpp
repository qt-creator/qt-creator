// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdowndocument.h"
#include "markdownviewerconstants.h"

namespace Markdown {
namespace Internal {

MarkdownDocument::MarkdownDocument()
{
    setId(Constants::MARKDOWNVIEWER_ID);
    setMimeType(QLatin1String(Constants::MARKDOWNVIEWER_MIME_TYPE));
    connect(this, &MarkdownDocument::mimeTypeChanged, this, &MarkdownDocument::changed);
}

} // namespace Internal
} // namespace Markdown
