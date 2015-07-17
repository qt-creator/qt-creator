/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SOURCE_UTILS_H
#define SOURCE_UTILS_H

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

#endif // SOURCE_UTILS_H
