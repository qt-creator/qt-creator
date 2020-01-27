// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "markdownviewerfactory.h"
#include "markdownviewerconstants.h"
#include "markdownviewer.h"

#include <QCoreApplication>

namespace Markdown {
namespace Internal {

MarkdownViewerFactory::MarkdownViewerFactory()
{
    setId(Constants::MARKDOWNVIEWER_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", Constants::MARKDOWNVIEWER_DISPLAY_NAME));
    addMimeType(Constants::MARKDOWNVIEWER_MIME_TYPE);
    setEditorCreator([] { return new MarkdownViewer(); });
}

} // namespace Internal
} // namespace Markdown
