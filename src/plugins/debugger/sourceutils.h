/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QString>

namespace TextEditor {
class TextDocument;
class TextEditorWidget;
}

namespace CPlusPlus { class Snapshot; }

namespace Debugger {
namespace Internal {

class ContextData;

// Editor tooltip support
QString cppExpressionAt(TextEditor::TextEditorWidget *editorWidget, int pos,
                        int *line, int *column, QString *function = 0,
                        int *scopeFromLine = 0, int *scopeToLine = 0);
QString fixCppExpression(const QString &exp);
QString cppFunctionAt(const QString &fileName, int line, int column = 0);

// Get variables that are not initialized at a certain line
// of a function from the code model. Shadowed variables will
// be reported using the debugger naming conventions '<shadowed n>'
bool getUninitializedVariables(const CPlusPlus::Snapshot &snapshot,
   const QString &function, const QString &file, int line,
   QStringList *uninitializedVariables);

ContextData getLocationContext(TextEditor::TextDocument *document, int lineNumber);

} // namespace Internal
} // namespace Debugger
