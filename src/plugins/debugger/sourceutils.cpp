// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sourceutils.h"

#include "debuggerinternalconstants.h"
#include "debuggerengine.h"
#include "disassemblerlines.h"
#include "watchdata.h"
#include "watchutils.h"

#include <cplusplus/AST.h>
#include <cplusplus/ASTVisitor.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TranslationUnit.h>
#include <cplusplus/TypeOfExpression.h>

#include <cppeditor/cppprojectfile.h>
#include <cppeditor/functionutils.h>
#include <cppeditor/cppmodelmanager.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTextDocument>
#include <QTextBlock>

#include <algorithm>

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

[[maybe_unused]] static QDebug operator<<(QDebug d, const Symbol &s)
{
    QString output;
    Overview o;
    QTextStream str(&output);
    debugCppSymbolRecursion(str, o, s, true, 0);
    d.nospace() << output;
    return d;
}

[[maybe_unused]] static QDebug operator<<(QDebug d, const Scope &scope)
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

namespace {

// Collects the name tokens of the calls whose function name is on the given line.
class CallCollector : public ASTVisitor
{
public:
    CallCollector(TranslationUnit *unit, int line)
        : ASTVisitor(unit), m_line(line)
    {}

    QList<std::pair<int, CallAST *>> calls;

private:
    bool preVisit(AST *ast) override
    {
        const int first = ast->firstToken();
        const int last = ast->lastToken() - 1;
        // The positions of tokens that a macro generated are not reliable.
        if (last < first || tokenAt(first).generated() || tokenAt(last).generated())
            return true;
        int firstLine = 0;
        int lastLine = 0;
        getTokenStartPosition(first, &firstLine, nullptr);
        getTokenEndPosition(last, &lastLine, nullptr);
        return firstLine <= m_line && m_line <= lastLine;
    }

    bool visit(CallAST *ast) override
    {
        const int nameToken = ast->base_expression->lastToken() - 1;
        if (!tokenAt(nameToken).generated()) {
            int line = 0;
            getTokenStartPosition(nameToken, &line, nullptr);
            if (line == m_line)
                calls.append({nameToken, ast});
        }
        return true;
    }

    const int m_line;
};

} // namespace

// The name of an anonymous namespace is not part of the name that is looked up.
static bool inAnonymousNamespace(const CPlusPlus::Symbol *symbol)
{
    for (const Scope *scope = symbol->enclosingScope(); scope; scope = scope->enclosingScope()) {
        const Namespace *ns = scope->asNamespace();
        if (ns && !ns->name() && scope->enclosingScope())
            return true;
    }
    return false;
}

// The functions that a line of C++ source calls, in the order of their
// appearance, as targets for "Run to Function".
QList<RunToTarget> functionCallTargets(const Snapshot &snapshot, const FilePath &filePath,
                                       const QByteArray &source, int lineNumber)
{
    const Document::Ptr document = snapshot.preprocessedDocument(source, filePath);
    document->check();
    TranslationUnit *unit = document->translationUnit();
    if (!unit->ast())
        return {};
    CallCollector collector(unit, lineNumber);
    collector.accept(unit->ast());
    std::sort(collector.calls.begin(), collector.calls.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    TypeOfExpression typeOf;
    typeOf.init(document, snapshot);
    const Overview overview;
    QList<RunToTarget> result;
    for (const auto &[nameToken, call] : std::as_const(collector.calls)) {
        int line = 0;
        int column = 0;
        unit->getTokenPosition(nameToken, &line, &column);
        Scope *scope = document->scopeAt(line, column);
        if (!scope)
            continue;
        // An explicitly qualified call such as Base::paint() does not dispatch.
        const bool qualified = nameToken > 0 && unit->tokenAt(nameToken - 1).is(T_COLON_COLON);
        for (const LookupItem &item : typeOf(call->base_expression, document, scope)) {
            CPlusPlus::Symbol *function = item.declaration();
            if (Template *templ = function ? function->asTemplate() : nullptr)
                function = templ->declaration();
            if (!function || !function->type()->asFunctionType())
                continue;
            // A virtual call runs an override, so only the bare name can match it.
            RunToTarget target;
            target.anyClass = !qualified && CppEditor::isVirtualFunction(
                function->type()->asFunctionType(), typeOf.context());
            target.function = target.anyClass
                    ? overview.prettyName(function->name())
                    : overview.prettyName(LookupContext::fullyQualifiedName(function));
            target.exact = !target.anyClass && !inAnonymousNamespace(function);
            if (!result.contains(target))
                result.append(target);
        }
    }
    return result;
}

QList<RunToTarget> functionCallTargets(const FilePath &filePath, const QByteArray &source,
                                       int lineNumber)
{
    return functionCallTargets(CppModelManager::snapshot(), filePath, source, lineNumber);
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
            const Utils::FilePath fileName = Utils::FilePath::fromVariant(
                document->property(Constants::DISASSEMBLER_SOURCE_FILE));
            if (!fileName.isEmpty()) {
                // Possibly one of the  "27 [1] foo = x" lines
                int pos = line.indexOf('[');
                int ln = line.left(pos - 1).toInt();
                if (ln > 0) {
                    data.type = LocationByFile;
                    data.fileName = fileName;
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

} // Debugger::Internal
