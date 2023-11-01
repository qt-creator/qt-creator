// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sourceutils.h"

#include "debuggerinternalconstants.h"
#include "debuggerengine.h"
#include "debuggertr.h"
#include "disassemblerlines.h"
#include "watchdata.h"
#include "watchutils.h"

#include <coreplugin/editormanager/ieditor.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/LookupItem.h>
#include <cplusplus/Overview.h>

#include <cppeditor/abstracteditorsupport.h>
#include <cppeditor/cppprojectfile.h>
#include <cppeditor/cppmodelmanager.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTextDocument>
#include <QTextBlock>

#include <string.h>
#include <ctype.h>

enum { debug = 0 };

using namespace CppEditor;
using namespace CPlusPlus;
using namespace TextEditor;
using namespace Utils;

namespace CPlusPlus {

static void debugCppSymbolRecursion(QTextStream &str, const Overview &o,
                                    const Symbol &s, bool doRecurse = true,
                                    int recursion = 0)
{
    for (int i = 0; i < recursion; i++)
        str << "  ";
    str << "Symbol: " << o.prettyName(s.name()) << " at line " << s.line();
    if (s.asFunction())
        str << " function";
    if (s.asClass())
        str << " class";
    if (s.asDeclaration())
        str << " declaration";
    if (s.asBlock())
        str << " block";
    if (doRecurse && s.asScope()) {
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
    if (scope.asNamespace())
        str << " namespace";
    if (scope.asClass())
        str << " class";
    if (scope.asEnum())
        str << " enum";
    if (scope.asBlock())
        str << " block";
    if (scope.asFunction())
        str << " function";
    if (scope.asDeclaration())
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

namespace Debugger::Internal {

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

using SeenHash = QHash<QString, int>;

static void blockRecursion(const Overview &overview,
                           const Scope *scope,
                           int line,
                           QStringList *uninitializedVariables,
                           SeenHash *seenHash,
                           int level = 0)
{
    // Go backwards in case someone has identical variables in the same scope.
    // Fixme: loop variables or similar are currently seen in the outer scope
    for (int s = scope->memberCount() - 1; s >= 0; --s){
        const CPlusPlus::Symbol *symbol = scope->memberAt(s);
        if (symbol->asDeclaration()) {
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

QStringList getUninitializedVariables(const Snapshot &snapshot,
                                      const QString &functionName,
                                      const FilePath &file,
                                      int line)
{
    QStringList result;
    // Find document
    if (snapshot.isEmpty() || functionName.isEmpty() || file.isEmpty() || line < 1)
        return result;
    const Snapshot::const_iterator docIt = snapshot.find(file);
    if (docIt == snapshot.end())
        return result;
    const Document::Ptr doc = docIt.value();
    // Look at symbol at line and find its function. Either it is the
    // function itself or some expression/variable.
    const CPlusPlus::Symbol *symbolAtLine = doc->lastVisibleSymbolAt(line, 0);
    if (!symbolAtLine)
        return result;
    // First figure out the function to do a safety name check
    // and the innermost scope at cursor position
    const Function *function = nullptr;
    const Scope *innerMostScope = nullptr;
    if (symbolAtLine->asFunction()) {
        function = symbolAtLine->asFunction();
        if (function->memberCount() == 1) // Skip over function block
            if (Block *block = function->memberAt(0)->asBlock())
                innerMostScope = block;
    } else {
        if (const Scope *functionScope = symbolAtLine->enclosingFunction()) {
            function = functionScope->asFunction();
            innerMostScope = symbolAtLine->asBlock() ?
                             symbolAtLine->asBlock() :
                             symbolAtLine->enclosingBlock();
        }
    }
    if (!function || !innerMostScope)
        return result;
    // Compare function names with a bit off fuzz,
    // skipping modules from a CDB symbol "lib!foo" or namespaces
    // that the code model does not show at this point
    Overview overview;
    const QString name = overview.prettyName(function->name());
    if (!functionName.endsWith(name))
        return result;
    if (functionName.size() > name.size()) {
        const char previousChar = functionName.at(functionName.size() - name.size() - 1).toLatin1();
        if (previousChar != ':' && previousChar != '!' )
            return result;
    }
    // Starting from the innermost block scope, collect declarations.
    SeenHash seenHash;
    blockRecursion(overview, innerMostScope, line, &result, &seenHash);
    return result;
}

QString cppFunctionAt(const FilePath &filePath, int line, int column)
{
    const Snapshot snapshot = CppModelManager::snapshot();
    if (const Document::Ptr document = snapshot.document(filePath))
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

    const FilePath filePath = editorWidget->textDocument()->filePath();
    const Snapshot snapshot = CppModelManager::snapshot();
    const Document::Ptr document = snapshot.document(filePath);
    QTextCursor tc = editorWidget->textCursor();
    QString expr;
    if (tc.hasSelection() && pos >= tc.selectionStart() && pos <= tc.selectionEnd()) {
        expr = tc.selectedText();
    } else {
        tc.setPosition(pos);
        const QChar ch = editorWidget->characterAt(pos);
        if (ch.isLetterOrNumber() || ch == '_')
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
                int pos = line.indexOf('[');
                int ln = line.left(pos - 1).toInt();
                if (ln > 0) {
                    data.type = LocationByFile;
                    data.fileName = Utils::FilePath::fromString(fileName);
                    data.textPosition.line = ln;
                }
            }
        }
    } else {
        data.type = LocationByFile;
        data.fileName = document->filePath();
        data.textPosition.line = lineNumber;
    }
    return data;
}

//
// Annotations
//
class DebuggerValueMark : public TextEditor::TextMark
{
public:
    DebuggerValueMark(const FilePath &fileName, int lineNumber, const QString &value)
        : TextMark(fileName,
                   lineNumber,
                   {Tr::tr("Debugger Value"), Constants::TEXT_MARK_CATEGORY_VALUE})
    {
        setPriority(TextEditor::TextMark::HighPriority);
        setToolTipProvider([] { return QString(); });
        setLineAnnotation(value);
    }
};

static QList<DebuggerValueMark *> marks;

// Stolen from CPlusPlus::Document::functionAt(...)
static int firstRelevantLine(const Document::Ptr document, int line, int column)
{
    QTC_ASSERT(line > 0 && column > 0, return 0);
    CPlusPlus::Symbol *symbol = document->lastVisibleSymbolAt(line, column);
    if (!symbol)
        return 0;

    // Find the enclosing function scope (which might be several levels up,
    // or we might be standing on it)
    Scope *scope = symbol->asScope();
    if (!scope)
        scope = symbol->enclosingScope();

    while (scope && !scope->asFunction() )
        scope = scope->enclosingScope();

    if (!scope)
        return 0;

    return scope->line();
}

static void setValueAnnotationsHelper(BaseTextEditor *textEditor,
                                      const Location &loc,
                                      QMap<QString, QString> values)
{
    TextEditorWidget *widget = textEditor->editorWidget();
    TextDocument *textDocument = widget->textDocument();
    const FilePath filePath = loc.fileName();
    const Snapshot snapshot = CppModelManager::snapshot();
    const Document::Ptr cppDocument = snapshot.document(filePath);
    if (!cppDocument) // For non-C++ documents.
        return;

    const int firstLine = firstRelevantLine(cppDocument, loc.textPosition().line, 1);
    if (firstLine < 1)
        return;

    CPlusPlus::ExpressionUnderCursor expressionUnderCursor(cppDocument->languageFeatures());
    QTextCursor tc = widget->textCursor();
    for (int lineNumber = loc.textPosition().line; lineNumber >= firstLine; --lineNumber) {
        const QTextBlock block = textDocument->document()->findBlockByNumber(lineNumber - 1);
        tc.setPosition(block.position());
        for (; !tc.atBlockEnd(); tc.movePosition(QTextCursor::NextCharacter)) {
            const QString expression = expressionUnderCursor(tc);
            if (expression.isEmpty())
                continue;
            const QString value = escapeUnprintable(values.take(expression)); // Show value one only once.
            if (value.isEmpty())
                continue;
            const QString annotation = QString("%1: %2").arg(expression, value);
            marks.append(new DebuggerValueMark(filePath, lineNumber, annotation));
        }
    }
}

void setValueAnnotations(const Location &loc, const QMap<QString, QString> &values)
{
    qDeleteAll(marks);
    marks.clear();
    if (values.isEmpty())
        return;

    const QList<Core::IEditor *> editors = Core::EditorManager::visibleEditors();
    for (Core::IEditor *editor : editors) {
        if (auto textEditor = qobject_cast<BaseTextEditor *>(editor)) {
            if (textEditor->textDocument()->filePath() == loc.fileName())
                setValueAnnotationsHelper(textEditor, loc, values);
        }
    }
}

} // Debugger::Internal
