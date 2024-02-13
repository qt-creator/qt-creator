// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <texteditor/textindenter.h>

namespace QmlJSEditor {

QMLJSTOOLS_EXPORT TextEditor::TextIndenter *createQmlJsIndenter(QTextDocument *doc);

QMLJSTOOLS_EXPORT void indentQmlJs(QTextDocument *doc, int startLine, int endLine,
                                   const TextEditor::TabSettings &tabSettings);

} // QmlJSEditor
