/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sourceutils.h"

#include "debuggerprotocol.h"
#include "debuggerstringutils.h"
#include "watchdata.h"
#include "watchutils.h"

#include <utils/qtcassert.h>

#include <coreplugin/idocument.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <cpptools/cpptoolsconstants.h>
#include <cpptools/abstracteditorsupport.h>

#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Overview.h>
#include <Symbols.h>
#include <Scope.h>

#include <extensionsystem/pluginmanager.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QStringList>
#include <QTextStream>
#include <QTime>

#include <QTextCursor>
#include <QPlainTextEdit>

#include <string.h>
#include <ctype.h>

enum { debug = 0 };

// Debug helpers for code model. @todo: Move to some CppTools library?
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
    CPlusPlus::Overview o;
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
    if (scope.isFunction())
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

static void blockRecursion(const CPlusPlus::Overview &overview,
                           const CPlusPlus::Scope *scope,
                           unsigned line,
                           QStringList *uninitializedVariables,
                           SeenHash *seenHash,
                           int level = 0)
{
    // Go backwards in case someone has identical variables in the same scope.
    // Fixme: loop variables or similar are currently seen in the outer scope
    for (int s = scope->memberCount() - 1; s >= 0; --s){
        const CPlusPlus::Symbol *symbol = scope->memberAt(s);
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
                uninitializedVariables->push_back(WatchData::shadowedName(name, it.value()));
        }
    }
    // Next block scope.
    if (const CPlusPlus::Scope *enclosingScope = scope->enclosingBlock())
        blockRecursion(overview, enclosingScope, line, uninitializedVariables, seenHash, level + 1);
}

// Inline helper with integer error return codes.
static inline
int getUninitializedVariablesI(const CPlusPlus::Snapshot &snapshot,
                               const QString &functionName,
                               const QString &file,
                               int line,
                               QStringList *uninitializedVariables)
{
    uninitializedVariables->clear();
    // Find document
    if (snapshot.isEmpty() || functionName.isEmpty() || file.isEmpty() || line < 1)
        return 1;
    const CPlusPlus::Snapshot::const_iterator docIt = snapshot.find(file);
    if (docIt == snapshot.end())
        return 2;
    const CPlusPlus::Document::Ptr doc = docIt.value();
    // Look at symbol at line and find its function. Either it is the
    // function itself or some expression/variable.
    const CPlusPlus::Symbol *symbolAtLine = doc->lastVisibleSymbolAt(line, 0);
    if (!symbolAtLine)
        return 4;
    // First figure out the function to do a safety name check
    // and the innermost scope at cursor position
    const CPlusPlus::Function *function = 0;
    const CPlusPlus::Scope *innerMostScope = 0;
    if (symbolAtLine->isFunction()) {
        function = symbolAtLine->asFunction();
        if (function->memberCount() == 1) // Skip over function block
            if (CPlusPlus::Block *block = function->memberAt(0)->asBlock())
                innerMostScope = block;
    } else {
        if (const CPlusPlus::Scope *functionScope = symbolAtLine->enclosingFunction()) {
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
    CPlusPlus::Overview overview;
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

bool getUninitializedVariables(const CPlusPlus::Snapshot &snapshot,
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
                << uninitializedVariables->join(QString(QLatin1Char(','))) << '\'';
        if (rc)
            str << " of " << snapshot.size() << " documents";
        qDebug() << msg;
    }
    return rc == 0;
}

//QByteArray gdbQuoteTypes(const QByteArray &type)
//{
//    // gdb does not understand sizeof(Core::IDocument*).
//    // "sizeof('Core::IDocument*')" is also not acceptable,
//    // it needs to be "sizeof('Core::IDocument'*)"
//    //
//    // We never will have a perfect solution here (even if we had a full blown
//    // C++ parser as we do not have information on what is a type and what is
//    // a variable name. So "a<b>::c" could either be two comparisons of values
//    // 'a', 'b' and '::c', or a nested type 'c' in a template 'a<b>'. We
//    // assume here it is the latter.
//    //return type;

//    // (*('myns::QPointer<myns::QObject>*'*)0x684060)" is not acceptable
//    // (*('myns::QPointer<myns::QObject>'**)0x684060)" is acceptable
//    if (isPointerType(type))
//        return gdbQuoteTypes(stripPointerType(type)) + '*';

//    QByteArray accu;
//    QByteArray result;
//    int templateLevel = 0;

//    const char colon = ':';
//    const char singleQuote = '\'';
//    const char lessThan = '<';
//    const char greaterThan = '>';
//    for (int i = 0; i != type.size(); ++i) {
//        const char c = type.at(i);
//        if (isLetterOrNumber(c) || c == '_' || c == colon || c == ' ') {
//            accu += c;
//        } else if (c == lessThan) {
//            ++templateLevel;
//            accu += c;
//        } else if (c == greaterThan) {
//            --templateLevel;
//            accu += c;
//        } else if (templateLevel > 0) {
//            accu += c;
//        } else {
//            if (accu.contains(colon) || accu.contains(lessThan))
//                result += singleQuote + accu + singleQuote;
//            else
//                result += accu;
//            accu.clear();
//            result += c;
//        }
//    }
//    if (accu.contains(colon) || accu.contains(lessThan))
//        result += singleQuote + accu + singleQuote;
//    else
//        result += accu;
//    //qDebug() << "GDB_QUOTING" << type << " TO " << result;

//    return result;
//}

// Utilities to decode string data returned by the dumper helpers.


// Editor tooltip support
bool isCppEditor(Core::IEditor *editor)
{
    using namespace CppTools::Constants;
    const Core::IDocument *document= editor->document();
    if (!document)
        return false;
    const QByteArray mimeType = document->mimeType().toLatin1();
    return mimeType == C_SOURCE_MIMETYPE
        || mimeType == CPP_SOURCE_MIMETYPE
        || mimeType == CPP_HEADER_MIMETYPE
        || mimeType == OBJECTIVE_CPP_SOURCE_MIMETYPE;
}

// Return the Cpp expression, and, if desired, the function
QString cppExpressionAt(TextEditor::ITextEditor *editor, int pos,
                        int *line, int *column, QString *function /* = 0 */)
{
    using namespace CppTools;
    using namespace CPlusPlus;
    *line = *column = 0;
    if (function)
        function->clear();

    const QPlainTextEdit *plaintext = qobject_cast<QPlainTextEdit*>(editor->widget());
    if (!plaintext)
        return QString();

    QString expr = plaintext->textCursor().selectedText();
    CppModelManagerInterface *modelManager = CppModelManagerInterface::instance();
    if (expr.isEmpty() && modelManager) {
        QTextCursor tc(plaintext->document());
        tc.setPosition(pos);

        const QChar ch = editor->characterAt(pos);
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
            tc.movePosition(QTextCursor::EndOfWord);

        // Fetch the expression's code.
        CPlusPlus::ExpressionUnderCursor expressionUnderCursor;
        expr = expressionUnderCursor(tc);
        *column = tc.positionInBlock();
        *line = tc.blockNumber();
    } else {
        const QTextCursor tc = plaintext->textCursor();
        *column = tc.positionInBlock();
        *line = tc.blockNumber();
    }

    if (function && !expr.isEmpty())
        if (const Core::IDocument *document= editor->document())
            if (modelManager)
                *function = AbstractEditorSupport::functionAt(modelManager,
                    document->fileName(), *line, *column);

    return expr;
}

// Ensure an expression can be added as side-effect
// free debugger expression.
QString fixCppExpression(const QString &expIn)
{
    QString exp = expIn.trimmed();;
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

QString cppFunctionAt(const QString &fileName, int line)
{
    using namespace CppTools;
    using namespace CPlusPlus;
    CppModelManagerInterface *modelManager = CppModelManagerInterface::instance();
    return AbstractEditorSupport::functionAt(modelManager,
                                             fileName, line, 1);
}

} // namespace Internal
} // namespace Debugger
