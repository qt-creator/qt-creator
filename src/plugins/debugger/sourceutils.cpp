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

#include "sourceutils.h"

#include "debuggerinternalconstants.h"
#include "debuggerengine.h"
#include "disassemblerlines.h"
#include "watchdata.h"
#include "watchutils.h"

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <cpptools/abstracteditorsupport.h>
#include <cpptools/cppprojectfile.h>
#include <cpptools/cppmodelmanager.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Overview.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTextDocument>
#include <QTextBlock>

#include <string.h>
#include <ctype.h>

enum { debug = 0 };

using namespace CppTools;
using namespace CPlusPlus;
using namespace TextEditor;

namespace CPlusPlus {

static void debugCppSymbolRecursion(QTextStream &str, const Overview &o,
                                    const Symbol &s, bool doRecurse = true,
                                    int recursion = 0)
{
    for (int i = 0; i < recursion; i++)
        str << "  ";
    str << "Symbol: " << o.prettyName(s.name()) << " at line " << s.line();
    if (s.isFunction())
        str << " function";
    if (s.isClass())
        str << " class";
    if (s.isDeclaration())
        str << " declaration";
    if (s.isBlock())
        str << " block";
    if (doRecurse && s.isScope()) {
        const Scope *scoped = s.asScope();
        const int size =  scoped->memberCount();
        str << " scoped symbol of " << size << '\n';
        for (int m = 0; m < size; m++)
            debugCppSymbolRecursion(str, o, *scoped->memberAt(m), true, recursion + 1);
    } else {
        str << '\n';
    }
}

QDebug operator<<(QDebug d, const Symbol &s)
{
    QString output;
    Overview o;
    QTextStream str(&output);
    debugCppSymbolRecursion(str, o, s, true, 0);
    d.nospace() << output;
    return d;
}

QDebug operator<<(QDebug d, const Scope &scope)
{
    QString output;
    Overview o;
    QTextStream str(&output);
    const int size =  scope.memberCount();
    str << "Scope of " << size;
    if (scope.isNamespace())
        str << " namespace";
    if (scope.isClass())
        str << " class";
    if (scope.isEnum())
        str << " enum";
    if (scope.isBlock())
        str << " block";
    if (scope.isFunction())
        str << " function";
    if (scope.isDeclaration())
        str << " prototype";
#if 0 // ### port me
    if (const Symbol *owner = &scope) {
        str << " owner: ";
        debugCppSymbolRecursion(str, o, *owner, false, 0);
    } else {
        str << " 0-owner\n";
    }
#endif
    for (int s = 0; s < size; s++)
        debugCppSymbolRecursion(str, o, *scope.memberAt(s), true, 2);
    d.nospace() << output;
    return d;
}

} // namespace CPlusPlus

namespace Debugger {
namespace Internal {

/* getUninitializedVariables(): Get variables that are not initialized
 * at a certain line of a function from the code model to be able to
 * indicate them as not in scope in the locals view.
 * Find document + function in the code model, do a double check and
 * collect declarative symbols that are in the function past or on
 * the current line. blockRecursion() recurses up the scopes
 * and collect symbols declared past or on the current line.
 * Recursion goes up from the innermost scope, keeping a map
 * of occurrences seen, to be able to derive the names of
 * shadowed variables as the debugger sees them:
\code
int x;             // Occurrence (1), should be reported as "x <shadowed 1>"
if (true) {
   int x = 5; (2)  // Occurrence (2), should be reported as "x"
}
\endcode
 */

typedef QHash<QString, int> SeenHash;

static void blockRecursion(const Overview &overview,
                           const Scope *scope,
                           unsigned line,
                           QStringList *uninitializedVariables,
                           SeenHash *seenHash,
                           int level = 0)
{
    // Go backwards in case someone has identical variables in the same scope.
    // Fixme: loop variables or similar are currently seen in the outer scope
    for (int s = scope->memberCount() - 1; s >= 0; --s){
        const Symbol *symbol = scope->memberAt(s);
        if (symbol->isDeclaration()) {
            // Find out about shadowed symbols by bookkeeping
            // the already seen occurrences in a hash.
            const QString name = overview.prettyName(symbol->name());
            SeenHash::iterator it = seenHash->find(name);
            if (it == seenHash->end())
                it = seenHash->insert(name, 0);
            else
                ++(it.value());
            // Is the declaration on or past the current line, that is,
            // the variable not initialized.
            if (symbol->line() >= line)
                uninitializedVariables->push_back(WatchItem::shadowedName(name, it.value()));
        }
    }
    // Next block scope.
    if (const Scope *enclosingScope = scope->enclosingBlock())
        blockRecursion(overview, enclosingScope, line, uninitializedVariables, seenHash, level + 1);
}

// Inline helper with integer error return codes.
static inline
int getUninitializedVariablesI(const Snapshot &snapshot,
                               const QString &functionName,
                               const QString &file,
                               int line,
                               QStringList *uninitializedVariables)
{
    uninitializedVariables->clear();
    // Find document
    if (snapshot.isEmpty() || functionName.isEmpty() || file.isEmpty() || line < 1)
        return 1;
    const Snapshot::const_iterator docIt = snapshot.find(file);
    if (docIt == snapshot.end())
        return 2;
    const Document::Ptr doc = docIt.value();
    // Look at symbol at line and find its function. Either it is the
    // function itself or some expression/variable.
    const Symbol *symbolAtLine = doc->lastVisibleSymbolAt(line, 0);
    if (!symbolAtLine)
        return 4;
    // First figure out the function to do a safety name check
    // and the innermost scope at cursor position
    const Function *function = 0;
    const Scope *innerMostScope = 0;
    if (symbolAtLine->isFunction()) {
        function = symbolAtLine->asFunction();
        if (function->memberCount() == 1) // Skip over function block
            if (Block *block = function->memberAt(0)->asBlock())
                innerMostScope = block;
    } else {
        if (const Scope *functionScope = symbolAtLine->enclosingFunction()) {
            function = functionScope->asFunction();
            innerMostScope = symbolAtLine->isBlock() ?
                             symbolAtLine->asBlock() :
                             symbolAtLine->enclosingBlock();
        }
    }
    if (!function || !innerMostScope)
        return 7;
    // Compare function names with a bit off fuzz,
    // skipping modules from a CDB symbol "lib!foo" or namespaces
    // that the code model does not show at this point
    Overview overview;
    const QString name = overview.prettyName(function->name());
    if (!functionName.endsWith(name))
        return 11;
    if (functionName.size() > name.size()) {
        const char previousChar = functionName.at(functionName.size() - name.size() - 1).toLatin1();
        if (previousChar != ':' && previousChar != '!' )
            return 11;
    }
    // Starting from the innermost block scope, collect declarations.
    SeenHash seenHash;
    blockRecursion(overview, innerMostScope, line, uninitializedVariables, &seenHash);
    return 0;
}

bool getUninitializedVariables(const Snapshot &snapshot,
                               const QString &function,
                               const QString &file,
                               int line,
                               QStringList *uninitializedVariables)
{
    const int rc = getUninitializedVariablesI(snapshot, function, file, line, uninitializedVariables);
    if (debug) {
        QString msg;
        QTextStream str(&msg);
        str << "getUninitializedVariables() " << function << ' ' << file << ':' << line
                << " returns (int) " << rc << " '"
                << uninitializedVariables->join(QLatin1Char(',')) << '\'';
        if (rc)
            str << " of " << snapshot.size() << " documents";
        qDebug() << msg;
    }
    return rc == 0;
}


QString cppFunctionAt(const QString &fileName, int line, int column)
{
    const Snapshot snapshot = CppModelManager::instance()->snapshot();
    if (const Document::Ptr document = snapshot.document(fileName))
        return document->functionAt(line, column);

    return QString();
}


// Return the Cpp expression, and, if desired, the function
QString cppExpressionAt(TextEditorWidget *editorWidget, int pos,
                        int *line, int *column, QString *function,
                        int *scopeFromLine, int *scopeToLine)
{
    if (function)
        function->clear();

    const QString fileName = editorWidget->textDocument()->filePath().toString();
    const Snapshot snapshot = CppModelManager::instance()->snapshot();
    const Document::Ptr document = snapshot.document(fileName);
    QTextCursor tc = editorWidget->textCursor();
    QString expr = tc.selectedText();
    if (expr.isEmpty()) {
        tc.setPosition(pos);
        const QChar ch = editorWidget->characterAt(pos);
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
            tc.movePosition(QTextCursor::EndOfWord);

        // Fetch the expression's code.
        ExpressionUnderCursor expressionUnderCursor(document ? document->languageFeatures()
                                                             : LanguageFeatures::defaultFeatures());
        expr = expressionUnderCursor(tc);
    }

    *column = tc.positionInBlock();
    *line = tc.blockNumber() + 1;

    if (!expr.isEmpty() && document) {
        QString func = document->functionAt(*line, *column, scopeFromLine, scopeToLine);
        if (function)
            *function = func;
    }

    return expr;
}

// Ensure an expression can be added as side-effect
// free debugger expression.
QString fixCppExpression(const QString &expIn)
{
    QString exp = expIn.trimmed();
    // Extract the first identifier, everything else is considered
    // too dangerous.
    int pos1 = 0, pos2 = exp.size();
    bool inId = false;
    for (int i = 0; i != exp.size(); ++i) {
        const QChar c = exp.at(i);
        const bool isIdChar = c.isLetterOrNumber() || c.unicode() == '_';
        if (inId && !isIdChar) {
            pos2 = i;
            break;
        }
        if (!inId && isIdChar) {
            inId = true;
            pos1 = i;
        }
    }
    exp = exp.mid(pos1, pos2 - pos1);
    return removeObviousSideEffects(exp);
}

ContextData getLocationContext(TextDocument *document, int lineNumber)
{
    ContextData data;
    QTC_ASSERT(document, return data);
    if (document->property(Constants::OPENED_WITH_DISASSEMBLY).toBool()) {
        QString line = document->document()->findBlockByNumber(lineNumber - 1).text();
        DisassemblerLine l;
        l.fromString(line);
        if (l.address) {
            data.type = LocationByAddress;
            data.address = l.address;
        } else {
            QString fileName = document->property(Constants::DISASSEMBLER_SOURCE_FILE).toString();
            if (!fileName.isEmpty()) {
                // Possibly one of the  "27 [1] foo = x" lines
                int pos = line.indexOf(QLatin1Char('['));
                int ln = line.leftRef(pos - 1).toInt();
                if (ln > 0) {
                    data.type = LocationByFile;
                    data.fileName = fileName;
                    data.lineNumber = ln;
                }
            }
        }
    } else {
        data.type = LocationByFile;
        data.fileName = document->filePath().toString();
        data.lineNumber = lineNumber;
    }
    return data;
}

} // namespace Internal
} // namespace Debugger
