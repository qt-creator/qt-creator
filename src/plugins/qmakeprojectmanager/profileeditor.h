// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace QmakeProjectManager {
namespace Internal {

class ProFileEditorFactory : public TextEditor::TextEditorFactory
{
public:
    ProFileEditorFactory();
};

} // namespace Internal
} // namespace QmakeProjectManager
