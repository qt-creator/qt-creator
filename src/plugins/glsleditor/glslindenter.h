// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textindenter.h>

namespace GlslEditor::Internal {

TextEditor::TextIndenter *createGlslIndenter(QTextDocument *doc);

} // namespace GlslEditor::Internal
