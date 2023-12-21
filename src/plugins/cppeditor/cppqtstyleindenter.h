// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/textindenter.h>

namespace CppEditor {

CPPEDITOR_EXPORT TextEditor::Indenter *createCppQtStyleIndenter(QTextDocument *doc);

} // namespace CppEditor
