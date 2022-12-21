// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace GlslEditor {
namespace Internal {

int languageVariant(const QString &mimeType);

class GlslEditorFactory : public TextEditor::TextEditorFactory
{
public:
    GlslEditorFactory();
};

} // namespace Internal
} // namespace GlslEditor
