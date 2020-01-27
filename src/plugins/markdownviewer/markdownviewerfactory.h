// Copyright (C) 2023 Tasuku Suzuki
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditorfactory.h>

namespace Markdown {
namespace Internal {

class MarkdownViewerFactory final : public Core::IEditorFactory
{
public:
    MarkdownViewerFactory();
};

} // namespace Internal
} // namespace Markdown
