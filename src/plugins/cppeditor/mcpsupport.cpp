// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "cppcanonicalsymbol.h"
#include "cppeditorwidget.h"
#include "cppelementevaluator.h"
#include "cppindexingsupport.h"
#include "cpplocatordata.h"
#include "cppmodelmanager.h"
#include "cpprefactoringchanges.h"
#include "cppworkingcopy.h"
#include "functionutils.h"
#include "indexitem.h"
#include "searchsymbols.h"
#include "symbolfinder.h"

#include "quickfixes/cppquickfix.h"
#include "quickfixes/cppquickfixassistant.h"

#include <mcp/server/toolregistry.h>

#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/texteditor.h>
#include <texteditor/codeassist/assistenums.h>
#include <texteditor/quickfix.h>

#include <cplusplus/AST.h>
#include <cplusplus/ASTVisitor.h>
#include <cplusplus/Control.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/FindUsages.h>
#include <cplusplus/SimpleLexer.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TranslationUnit.h>

#include <utils/changeset.h>
#include <utils/filepath.h>
#include <utils/result.h>

#include <QFuture>
#include <QFutureInterface>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

#include <algorithm>
#include <functional>

using namespace Utils;

namespace CppEditor::Internal {

static QString itemKind(IndexItem::ItemType type)
{
    switch (type) {
    case IndexItem::Enum: return QStringLiteral("enum");
    case IndexItem::Class: return QStringLiteral("class");
    case IndexItem::Function: return QStringLiteral("function");
    case IndexItem::Declaration: return QStringLiteral("declaration");
    default: return QStringLiteral("unknown");
    }
}

static QString diagnosticSeverity(int level)
{
    switch (level) {
    case CPlusPlus::Document::DiagnosticMessage::Warning: return QStringLiteral("warning");
    case CPlusPlus::Document::DiagnosticMessage::Error: return QStringLiteral("error");
    case CPlusPlus::Document::DiagnosticMessage::Fatal: return QStringLiteral("fatal");
    default: return QStringLiteral("unknown");
    }
}

static QByteArray fileSource(const Utils::FilePath &filePath, const WorkingCopy &workingCopy)
{
    if (const std::optional<QByteArray> source = workingCopy.source(filePath))
        return *source;
    if (const Utils::Result<QByteArray> contents = filePath.fileContents())
        return *contents;
    return {};
}

static QString usageKind(CPlusPlus::Usage::Tags tags)
{
    if (tags.testFlag(CPlusPlus::Usage::Tag::Declaration))
        return QStringLiteral("declaration");
    if (tags.testAnyFlags({CPlusPlus::Usage::Tag::Write, CPlusPlus::Usage::Tag::WritableRef}))
        return QStringLiteral("write");
    if (tags.testFlag(CPlusPlus::Usage::Tag::Read))
        return QStringLiteral("read");
    return QStringLiteral("other");
}

constexpr int defaultResultLimit = 200;
constexpr int maxResultLimit = 1000;

// Parse an optional "limit" argument. Clamped, so a client cannot use it to ask
// for an unbounded response again.
static int resultLimit(const QJsonObject &args)
{
    const int limit = args.value("limit").toInt();
    return std::clamp(limit > 0 ? limit : defaultResultLimit, 1, maxResultLimit);
}

// Cap a result array to a limit, reporting the pre-cap total and whether it was
// truncated, so a query on a large project cannot return an unbounded response.
static QJsonArray capResults(const QJsonArray &results, int limit, int *total, bool *truncated)
{
    *total = int(results.size());
    *truncated = *total > limit;
    if (!*truncated)
        return results;
    QJsonArray capped;
    for (int i = 0; i < limit; ++i)
        capped.append(results.at(i));
    return capped;
}

// The output-schema shape of a source location, as reported by location().
static QJsonObject locationSchema()
{
    return QJsonObject{
        {"type", "object"},
        {"properties",
         QJsonObject{{"file", QJsonObject{{"type", "string"}}},
                     {"line", QJsonObject{{"type", "integer"}}},
                     {"column", QJsonObject{{"type", "integer"}}}}}};
}

// The input-schema property for the optional result limit.
static QJsonObject limitProperty()
{
    return QJsonObject{{"type", "integer"},
                       {"description", QString("Maximum number of results (default %1, "
                                               "capped at %2).")
                                           .arg(defaultResultLimit).arg(maxResultLimit)}};
}

static QString symbolKind(const CPlusPlus::Symbol *symbol)
{
    if (symbol->asClass())
        return QStringLiteral("class");
    if (symbol->asEnum())
        return QStringLiteral("enum");
    if (symbol->asNamespace())
        return QStringLiteral("namespace");
    if (symbol->asTemplate())
        return QStringLiteral("template");
    if (symbol->asFunction())
        return QStringLiteral("function");
    if (const CPlusPlus::Declaration *decl = symbol->asDeclaration())
        return decl->type()->asFunctionType() ? QStringLiteral("function")
                                              : QStringLiteral("variable");
    return QStringLiteral("symbol");
}

// Resolve the C++ symbol at a 1-based line/column in a file, returning it and
// (via context) its lookup context, or nullptr if there is none. The context's
// document is a fresh parse of the file's current text, with its AST: a local
// symbol is matched by the identity of its scope, so a usages search over the
// symbol's own file has to run on this very document (see symbolUsages()).
static CPlusPlus::Symbol *symbolAt(const Utils::FilePath &filePath, int line, int column,
                                   CPlusPlus::LookupContext *context)
{
    const CPlusPlus::Snapshot snapshot = CppModelManager::snapshot();
    if (!snapshot.contains(filePath))
        return nullptr;
    const QByteArray source = fileSource(filePath, CppModelManager::workingCopy());
    CPlusPlus::Document::Ptr doc = snapshot.preprocessedDocument(source, filePath);
    doc->tokenize();
    doc->check();
    QTextDocument textDocument(QString::fromUtf8(source));
    const QTextBlock block = textDocument.findBlockByNumber(line - 1);
    if (!block.isValid())
        return nullptr;
    QTextCursor cursor(&textDocument);
    cursor.setPosition(block.position() + qMax(0, column - 1));

    CanonicalSymbol canonicalSymbol(doc, snapshot);
    CPlusPlus::Symbol *symbol = canonicalSymbol(cursor);
    *context = canonicalSymbol.context();
    return symbol;
}

// Collect all usages of a symbol across the snapshot. Files that do not mention
// the symbol's identifier are skipped cheaply, mirroring CppFindReferences. The
// optional handler sees each file's parsed document together with the usages in
// it, while that document is still alive.
using UsagesInFileHandler = std::function<void(const CPlusPlus::Document::Ptr &document,
                                               const QList<CPlusPlus::Usage> &usages)>;
static QList<CPlusPlus::Usage> symbolUsages(CPlusPlus::Symbol *symbol,
                                            const CPlusPlus::LookupContext &context,
                                            const UsagesInFileHandler &perFile = {})
{
    const CPlusPlus::Identifier *id = symbol->identifier();
    if (!id)
        return {};
    const CPlusPlus::Snapshot snapshot = context.snapshot();
    const WorkingCopy workingCopy = CppModelManager::workingCopy();

    QList<CPlusPlus::Usage> usages;
    for (auto it = snapshot.begin(), end = snapshot.end(); it != end; ++it) {
        const Utils::FilePath filePath = it.key();
        if (!it.value()->control()->findIdentifier(id->chars(), id->size()))
            continue; // The file does not mention the name at all.

        const QByteArray source = fileSource(filePath, workingCopy);
        CPlusPlus::Document::Ptr doc;
        const CPlusPlus::Document::Ptr symbolDocument = context.thisDocument();
        if (symbolDocument && symbolDocument->filePath() == filePath
                && symbolDocument->translationUnit()->ast()) {
            // The symbol's own file: search the document it was resolved in,
            // as a local symbol is matched by the identity of its scope.
            doc = symbolDocument;
        } else {
            doc = snapshot.preprocessedDocument(source, filePath);
            doc->tokenize();
            if (!doc->control()->findIdentifier(id->chars(), id->size()))
                continue;
            doc->check();
        }
        CPlusPlus::FindUsages findUsages(source, doc, snapshot, /*categorize=*/true);
        findUsages(symbol);
        const QList<CPlusPlus::Usage> fileUsages = findUsages.usages();
        if (perFile)
            perFile(doc, fileUsages);
        usages += fileUsages;
    }
    return usages;
}

static QJsonObject hierarchyToJson(const CppClass &cppClass)
{
    QJsonObject node{{"name", cppClass.name}};
    if (!cppClass.qualifiedName.isEmpty() && cppClass.qualifiedName != cppClass.name)
        node.insert("qualified_name", cppClass.qualifiedName);
    if (!cppClass.link.targetFilePath.isEmpty()) {
        node.insert("file", cppClass.link.targetFilePath.toUserOutput());
        node.insert("line", cppClass.link.target.line);
        node.insert("column", cppClass.link.target.column);
    }
    QJsonArray bases;
    for (const CppClass &base : cppClass.bases)
        bases.append(hierarchyToJson(base));
    if (!bases.isEmpty())
        node.insert("bases", bases);
    QJsonArray derived;
    for (const CppClass &sub : cppClass.derived)
        derived.append(hierarchyToJson(sub));
    if (!derived.isEmpty())
        node.insert("derived", derived);
    return node;
}

// Collects the call sites within one function definition, identified by its
// 1-based definition line/column, for the outgoing call hierarchy.
class CalleeCollector : public CPlusPlus::ASTVisitor
{
public:
    CalleeCollector(CPlusPlus::TranslationUnit *unit, int defLine, int defColumn)
        : ASTVisitor(unit), m_defLine(defLine), m_defColumn(defColumn)
    {}

    // The (line, column) of each called function's name token, in source order.
    QList<QPair<int, int>> callPositions()
    {
        QList<QPair<int, int>> positions;
        if (m_firstToken < 0)
            return positions;
        for (int i = 0; i < m_callNameTokens.size(); ++i) {
            const int token = m_callNameTokens.at(i);
            if (token < m_firstToken || token > m_lastToken)
                continue;
            int line = 0;
            int column = 0;
            translationUnit()->getTokenPosition(token, &line, &column);
            positions.append({line, column});
        }
        return positions;
    }

private:
    bool visit(CPlusPlus::FunctionDefinitionAST *ast) override
    {
        if (ast->symbol && ast->symbol->line() == m_defLine
                && ast->symbol->column() == m_defColumn) {
            m_firstToken = ast->firstToken();
            m_lastToken = ast->lastToken();
        }
        return true;
    }

    bool visit(CPlusPlus::CallAST *ast) override
    {
        if (ast->base_expression && ast->base_expression->lastToken() > 0)
            m_callNameTokens.append(ast->base_expression->lastToken() - 1);
        return true;
    }

    const int m_defLine;
    const int m_defColumn;
    int m_firstToken = -1;
    int m_lastToken = -1;
    QList<int> m_callNameTokens;
};

// The source text of an AST node with its whitespace collapsed. Token positions
// are 1-based and refer to the original file, so the text is cut from its lines
// rather than from the preprocessed source the tokens index.
static QString astText(const QStringList &lines, CPlusPlus::TranslationUnit *unit,
                       const CPlusPlus::AST *ast)
{
    if (!ast || ast->lastToken() <= ast->firstToken())
        return {};
    int startLine = 0;
    int startColumn = 0;
    int endLine = 0;
    int endColumn = 0;
    unit->getTokenPosition(ast->firstToken(), &startLine, &startColumn);
    unit->getTokenEndPosition(ast->lastToken() - 1, &endLine, &endColumn);
    if (startLine <= 0 || endLine < startLine)
        return {};
    if (startLine == endLine) {
        return lines.value(startLine - 1)
            .mid(startColumn - 1, endColumn - startColumn).simplified();
    }
    QString text = lines.value(startLine - 1).mid(startColumn - 1);
    for (int line = startLine + 1; line < endLine; ++line)
        text += ' ' + lines.value(line - 1);
    text += ' ' + lines.value(endLine - 1).left(endColumn - 1);
    return text.simplified();
}

// The identifier a call is made through: "connect" for both connect(...) and
// QObject::connect(...) and obj->connect(...). Empty if there is none.
static QByteArray calleeName(const CPlusPlus::CallAST *call)
{
    if (!call->base_expression)
        return {};
    const CPlusPlus::NameAST *nameAst = nullptr;
    if (const CPlusPlus::IdExpressionAST *id = call->base_expression->asIdExpression())
        nameAst = id->name;
    else if (const CPlusPlus::MemberAccessAST *access = call->base_expression->asMemberAccess())
        nameAst = access->member_name;
    if (!nameAst || !nameAst->name)
        return {};
    const CPlusPlus::Identifier *id = nameAst->name->identifier();
    return id ? QByteArray(id->chars(), id->size()) : QByteArray();
}

// How a connect() argument names its signal or slot.
static QString connectArgumentKind(CPlusPlus::ExpressionAST *arg,
                                   CPlusPlus::TranslationUnit *unit)
{
    if (arg->asQtMethod())
        return QStringLiteral("qt4_macro");
    if (arg->asLambdaExpression())
        return QStringLiteral("lambda");
    if (const CPlusPlus::UnaryExpressionAST *unary = arg->asUnaryExpression()) {
        if (unit->tokenAt(unary->unary_op_token).is(CPlusPlus::T_AMPER) && unary->expression
                && unary->expression->asIdExpression()) {
            return QStringLiteral("member_pointer");
        }
    }
    return QStringLiteral("expression");
}

// The method name written inside a SIGNAL()/SLOT() macro, or empty.
static QByteArray qtMethodName(CPlusPlus::ExpressionAST *arg)
{
    const CPlusPlus::QtMethodAST *method = arg->asQtMethod();
    if (!method || !method->declarator || !method->declarator->core_declarator)
        return {};
    const CPlusPlus::DeclaratorIdAST *id = method->declarator->core_declarator->asDeclaratorId();
    if (!id || !id->name || !id->name->name)
        return {};
    const CPlusPlus::Identifier *identifier = id->name->name->identifier();
    return identifier ? QByteArray(identifier->chars(), identifier->size()) : QByteArray();
}

// The token to point a caller at for a signal or slot argument: the method name
// inside SIGNAL()/SLOT(), the member name of &Class::member, or else the start
// of the expression.
static int argumentNameToken(CPlusPlus::ExpressionAST *arg)
{
    if (const CPlusPlus::QtMethodAST *method = arg->asQtMethod()) {
        if (method->declarator && method->declarator->core_declarator) {
            const CPlusPlus::DeclaratorIdAST *id
                = method->declarator->core_declarator->asDeclaratorId();
            if (id && id->name)
                return id->name->firstToken();
        }
        return method->firstToken();
    }
    CPlusPlus::ExpressionAST *expression = arg;
    if (const CPlusPlus::UnaryExpressionAST *unary = arg->asUnaryExpression()) {
        if (unary->expression)
            expression = unary->expression;
    }
    if (const CPlusPlus::IdExpressionAST *id = expression->asIdExpression()) {
        if (id->name) {
            if (const CPlusPlus::QualifiedNameAST *qualified = id->name->asQualifiedName()) {
                if (qualified->unqualified_name)
                    return qualified->unqualified_name->firstToken();
            }
            return id->name->firstToken();
        }
    }
    return arg->firstToken();
}

// Collects the connect() and disconnect() calls of a translation unit.
class ConnectCallCollector : public CPlusPlus::ASTVisitor
{
public:
    explicit ConnectCallCollector(CPlusPlus::TranslationUnit *unit) : ASTVisitor(unit) {}

    QList<CPlusPlus::CallAST *> calls;

private:
    bool visit(CPlusPlus::CallAST *ast) override
    {
        const QByteArray name = calleeName(ast);
        if (name == "connect" || name == "disconnect")
            calls.append(ast);
        return true;
    }
};

// Whether two functions take the same parameters, whatever they are called -
// the part of a signature that survives a rename. Function::isSignatureEqualTo
// compares the names as well, which is the one thing that is about to change.
static bool sameParameters(const CPlusPlus::Function *a, const CPlusPlus::Function *b)
{
    if (a->isConst() != b->isConst() || a->isVolatile() != b->isVolatile()
            || a->isVariadic() != b->isVariadic() || a->argumentCount() != b->argumentCount()) {
        return false;
    }
    for (int i = 0; i < a->argumentCount(); ++i) {
        if (!a->argumentAt(i)->type().match(b->argumentAt(i)->type()))
            return false;
    }
    return true;
}

void registerMcpTools()
{
    using namespace Mcp::Schema;
    using Mcp::ToolRegistry;

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_get_file_symbols")
            .title("Get C++ symbols in a file")
            .description(
                "Returns the C++ symbols (classes, functions, enums, declarations) in a file "
                "from Qt Creator's C++ code model, each with its kind, fully qualified scope, "
                "type/signature and 1-based line and column. The file must be known to the "
                "code model, i.e. a C++ source or header that belongs to an open project.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Absolute path to the C++ source or header file."}})
                    .addProperty("limit", limitProperty())
                    .addRequired("file"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "symbols",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description",
                             "Symbols defined in the file, in declaration order."}})
                    .addProperty("total", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addRequired("symbols")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            if (file.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(
                    TextContent{}.text("Missing required argument \"file\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            const CPlusPlus::Document::Ptr doc = CppModelManager::document(filePath);
            if (!doc) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ code model document for \"%1\". Is it a C++ file that "
                            "belongs to an open project?")
                        .arg(filePath.toUserOutput())));
            }

            SearchSymbols searcher;
            searcher.setSymbolsToSearchFor(SymbolType::AllTypes);
            const IndexItem::Ptr root = searcher(doc);

            QJsonArray symbols;
            if (root) {
                root->visitAllChildren([&symbols](const IndexItem::Ptr &item) {
                    QJsonObject obj{
                        {"name", item->symbolName()},
                        {"kind", itemKind(item->type())},
                        {"line", item->line()},
                        {"column", item->column() + 1}}; // IndexItem::column() is 0-based.
                    if (!item->symbolScope().isEmpty())
                        obj.insert("scope", item->symbolScope());
                    if (!item->symbolType().isEmpty())
                        obj.insert("type", item->symbolType());
                    if (item->type() == IndexItem::Function)
                        obj.insert("is_definition", item->isFunctionDefinition());
                    symbols.append(obj);
                    return IndexItem::Recurse;
                });
            }

            int total = 0;
            bool truncated = false;
            const QJsonArray capped = capResults(symbols, resultLimit(args), &total, &truncated);
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                {"symbols", capped}, {"total", total}, {"truncated", truncated}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_get_file_problems")
            .title("Get C++ diagnostics for a file")
            .description(
                "Returns the C++ code model diagnostics (parser and semantic warnings and "
                "errors) for a file, each with its severity and 1-based line and column. The "
                "file must be known to the code model, i.e. a C++ source or header that "
                "belongs to an open project.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Absolute path to the C++ source or header file."}})
                    .addRequired("file"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "problems",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description", "Diagnostics reported for the file."}})
                    .addRequired("problems")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            if (file.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(
                    TextContent{}.text("Missing required argument \"file\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            const CPlusPlus::Document::Ptr doc = CppModelManager::document(filePath);
            if (!doc) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ code model document for \"%1\". Is it a C++ file that "
                            "belongs to an open project?")
                        .arg(filePath.toUserOutput())));
            }

            QJsonArray problems;
            const QList<CPlusPlus::Document::DiagnosticMessage> messages
                = doc->diagnosticMessages();
            for (const CPlusPlus::Document::DiagnosticMessage &m : messages) {
                problems.append(QJsonObject{
                    {"severity", diagnosticSeverity(m.level())},
                    {"line", m.line()},
                    {"column", m.column()},
                    {"text", m.text()}});
            }

            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"problems", problems}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_find_references")
            .title("Find C++ references")
            .description(
                "Finds all references (usages) of the C++ symbol at a position, using the "
                "C++ code model. Give the file and a 1-based line and column pointing at an "
                "identifier; returns each usage with its file, 1-based line and column, the "
                "source line text, the containing function, and whether it is a read, write "
                "or declaration. The file must belong to an open project.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the C++ file containing the symbol."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line of the identifier to resolve."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the identifier to resolve."}})
                    .addProperty("limit", limitProperty())
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "references",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description", "Usages of the symbol."}})
                    .addProperty("total", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addRequired("references")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            if (file.isEmpty() || line <= 0 || column <= 0) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\" and 1-based \"line\" and \"column\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            CPlusPlus::LookupContext context;
            CPlusPlus::Symbol *symbol = symbolAt(filePath, line, column, &context);
            if (!symbol) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ symbol found at %1:%2:%3. Is the file in an open project "
                            "and the position on an identifier?")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }

            QJsonArray references;
            const QList<CPlusPlus::Usage> usages = symbolUsages(symbol, context);
            for (const CPlusPlus::Usage &u : usages) {
                QJsonObject obj{
                    {"file", u.path.toUserOutput()},
                    {"line", u.line},
                    {"column", u.col},
                    {"length", u.len},
                    {"kind", usageKind(u.tags)}};
                const QString lineText = u.lineText.trimmed();
                if (!lineText.isEmpty())
                    obj.insert("line_text", lineText);
                if (!u.containingFunction.isEmpty())
                    obj.insert("function", u.containingFunction);
                references.append(obj);
            }

            int total = 0;
            bool truncated = false;
            const QJsonArray capped = capResults(references, resultLimit(args), &total, &truncated);
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                {"references", capped}, {"total", total}, {"truncated", truncated}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_get_symbol_info")
            .title("Get C++ symbol info and definition")
            .description(
                "Resolves the C++ symbol at a position and returns its name, fully qualified "
                "name, kind and type, plus its declaration and (for functions) definition "
                "locations - i.e. go-to-definition. Give the file and a 1-based line and "
                "column pointing at an identifier. The file must belong to an open project.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Absolute path to the C++ file."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line of the identifier."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the identifier."}})
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("name", QJsonObject{{"type", "string"}})
                    .addProperty("kind", QJsonObject{{"type", "string"}})
                    .addProperty("qualified_name", QJsonObject{{"type", "string"}})
                    .addProperty("type", QJsonObject{{"type", "string"}})
                    .addProperty("declaration", locationSchema())
                    .addProperty("definition", locationSchema())
                    .addRequired("name")
                    .addRequired("kind")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            if (file.isEmpty() || line <= 0 || column <= 0) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\" and 1-based \"line\" and \"column\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            CPlusPlus::LookupContext context;
            CPlusPlus::Symbol *symbol = symbolAt(filePath, line, column, &context);
            if (!symbol) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ symbol found at %1:%2:%3.")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }

            CPlusPlus::Overview overview;
            QJsonObject result{
                {"name", overview.prettyName(symbol->name())},
                {"kind", symbolKind(symbol)}};
            const QString qualified
                = overview.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol));
            if (!qualified.isEmpty())
                result.insert("qualified_name", qualified);
            const QString type = overview.prettyType(symbol->type());
            if (!type.isEmpty())
                result.insert("type", type);

            const auto location = [](const CPlusPlus::Symbol *s) {
                return QJsonObject{{"file", s->filePath().toUserOutput()},
                                   {"line", s->line()},
                                   {"column", s->column()}};
            };
            if (!symbol->filePath().isEmpty())
                result.insert("declaration", location(symbol));

            SymbolFinder finder;
            if (CPlusPlus::Function *def
                    = finder.findMatchingDefinition(symbol, context.snapshot(), false)) {
                result.insert("definition", location(def));
            }

            return CallToolResult{}.isError(false).structuredContent(result);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_get_type_hierarchy")
            .title("Get C++ type hierarchy")
            .description(
                "Returns the base and derived class hierarchy of the C++ class or struct at "
                "a position. Give the file and a 1-based line and column pointing at a class "
                "name; returns the class with nested \"bases\" (up) and \"derived\" (down), "
                "each with name and location. The file must belong to an open project.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Absolute path to the C++ file."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line of the class name."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the class name."}})
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("name", QJsonObject{{"type", "string"}})
                    .addProperty("qualified_name", QJsonObject{{"type", "string"}})
                    .addProperty("file", QJsonObject{{"type", "string"}})
                    .addProperty("line", QJsonObject{{"type", "integer"}})
                    .addProperty("column", QJsonObject{{"type", "integer"}})
                    // Each entry is a node of this same shape, nested recursively.
                    .addProperty(
                        "bases",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "object"}}}})
                    .addProperty(
                        "derived",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "object"}}}})
                    .addRequired("name")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            if (file.isEmpty() || line <= 0 || column <= 0) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\" and 1-based \"line\" and \"column\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            CPlusPlus::LookupContext context;
            CPlusPlus::Symbol *symbol = symbolAt(filePath, line, column, &context);
            if (!symbol) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ symbol found at %1:%2:%3.")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }

            // A live (non-canceled) future is required; the lookups bail out on
            // future.isCanceled(), and a default-constructed QFuture reads as
            // canceled.
            QFutureInterface<void> futureInterface;
            futureInterface.reportStarted();
            const QFuture<void> future = futureInterface.future();
            CppClass cppClass(symbol);
            cppClass.lookupBases(future, symbol, context);
            cppClass.lookupDerived(future, symbol, context.snapshot());
            futureInterface.reportFinished();

            return CallToolResult{}.isError(false).structuredContent(hierarchyToJson(cppClass));
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_find_callers")
            .title("Find C++ callers")
            .description(
                "Finds the callers of the C++ function at a position - the incoming call "
                "hierarchy - using the C++ code model. Give the file and a 1-based line and "
                "column pointing at a function name; returns each call site with its file, "
                "1-based line and column, the source line text, and the enclosing function "
                "(with its own location) that makes the call. The file must belong to an "
                "open project.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the C++ file containing the function."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line of the function name to resolve."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the function name to resolve."}})
                    .addProperty("limit", limitProperty())
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "callers",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description", "Call sites of the function."}})
                    .addProperty("total", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addRequired("callers")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            if (file.isEmpty() || line <= 0 || column <= 0) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\" and 1-based \"line\" and \"column\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            CPlusPlus::LookupContext context;
            CPlusPlus::Symbol *symbol = symbolAt(filePath, line, column, &context);
            if (!symbol) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ symbol found at %1:%2:%3. Is the file in an open project "
                            "and the position on a function name?")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }
            if (symbolKind(symbol) != QLatin1String("function")) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("The symbol at %1:%2:%3 is not a function.")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }

            // Group the call sites by the function that contains them. Only the
            // containingFunction name is safe to use here: containingFunctionSymbol
            // points into a document that symbolUsages() has already destroyed.
            QStringList callerOrder;
            QHash<QString, QJsonArray> sitesByCaller;
            const QList<CPlusPlus::Usage> usages = symbolUsages(symbol, context);
            for (const CPlusPlus::Usage &u : usages) {
                if (u.tags.testFlag(CPlusPlus::Usage::Tag::Declaration))
                    continue; // The function's own declaration/definition is not a call.

                const QString caller = u.containingFunction; // Empty at file scope.
                if (!sitesByCaller.contains(caller))
                    callerOrder.append(caller);
                QJsonObject site{
                    {"file", u.path.toUserOutput()},
                    {"line", u.line},
                    {"column", u.col}};
                const QString lineText = u.lineText.trimmed();
                if (!lineText.isEmpty())
                    site.insert("line_text", lineText);
                sitesByCaller[caller].append(site);
            }

            QJsonArray callers;
            for (const QString &caller : callerOrder) {
                QJsonObject node{{"call_sites", sitesByCaller.value(caller)}};
                if (!caller.isEmpty())
                    node.insert("caller", caller);
                callers.append(node);
            }

            int total = 0;
            bool truncated = false;
            const QJsonArray capped = capResults(callers, resultLimit(args), &total, &truncated);
            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"callers", capped}, {"total", total}, {"truncated", truncated}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_find_callees")
            .title("Find C++ callees")
            .description(
                "Finds the functions called by the C++ function at a position - the "
                "outgoing call hierarchy - using the C++ code model. Give the file and a "
                "1-based line and column pointing at a function name; returns each called "
                "function grouped with its call sites (file, 1-based line and column, and "
                "source line text). It resolves the function's definition, so the body "
                "must be available; the file must belong to an open project.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the C++ file containing the function."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line of the function name to resolve."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the function name to resolve."}})
                    .addProperty("limit", limitProperty())
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "callees",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description", "Functions called by the function."}})
                    .addProperty("total", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addRequired("callees")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            if (file.isEmpty() || line <= 0 || column <= 0) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\" and 1-based \"line\" and \"column\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            CPlusPlus::LookupContext context;
            CPlusPlus::Symbol *symbol = symbolAt(filePath, line, column, &context);
            if (!symbol) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ symbol found at %1:%2:%3. Is the file in an open project "
                            "and the position on a function name?")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }
            if (symbolKind(symbol) != QLatin1String("function")) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("The symbol at %1:%2:%3 is not a function.")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }

            // Prefer the definition (which has a body); fall back to the symbol
            // itself when no separate definition is found.
            const CPlusPlus::Snapshot snapshot = context.snapshot();
            SymbolFinder finder;
            const CPlusPlus::Symbol *defSymbol = symbol;
            if (CPlusPlus::Function *def = finder.findMatchingDefinition(symbol, snapshot, false))
                defSymbol = def;
            const FilePath defFile = defSymbol->filePath();

            QJsonArray callees;
            if (!defFile.isEmpty()) {
                const QByteArray source = fileSource(defFile, CppModelManager::workingCopy());
                CPlusPlus::Document::Ptr doc = snapshot.preprocessedDocument(source, defFile);
                doc->tokenize();
                doc->check();
                CPlusPlus::TranslationUnit *unit = doc->translationUnit();
                if (unit && unit->ast()) {
                    CalleeCollector collector(unit, defSymbol->line(), defSymbol->column());
                    unit->ast()->accept(&collector);
                    const QStringList lines = QString::fromUtf8(source).split('\n');

                    CPlusPlus::Overview overview;
                    QStringList order;
                    QHash<QString, QJsonObject> nodes;
                    QHash<QString, QJsonArray> sites;
                    const QList<QPair<int, int>> positions = collector.callPositions();
                    for (const QPair<int, int> &pos : positions) {
                        CPlusPlus::LookupContext calleeContext;
                        CPlusPlus::Symbol *callee
                            = symbolAt(defFile, pos.first, pos.second, &calleeContext);
                        if (!callee || symbolKind(callee) != QLatin1String("function"))
                            continue;
                        const QString name = overview.prettyName(callee->name());
                        const QString qualified = overview.prettyName(
                            CPlusPlus::LookupContext::fullyQualifiedName(callee));
                        const QString key = (qualified.isEmpty() ? name : qualified)
                                            + '@' + callee->filePath().toUserOutput() + ':'
                                            + QString::number(callee->line());
                        if (!nodes.contains(key)) {
                            order.append(key);
                            QJsonObject node{{"callee", name}};
                            if (!qualified.isEmpty() && qualified != name)
                                node.insert("qualified_name", qualified);
                            if (!callee->filePath().isEmpty()) {
                                node.insert(
                                    "declaration",
                                    QJsonObject{{"file", callee->filePath().toUserOutput()},
                                                {"line", callee->line()},
                                                {"column", callee->column()}});
                            }
                            nodes.insert(key, node);
                        }
                        QJsonObject site{{"file", defFile.toUserOutput()},
                                         {"line", pos.first},
                                         {"column", pos.second}};
                        const QString lineText = lines.value(pos.first - 1).trimmed();
                        if (!lineText.isEmpty())
                            site.insert("line_text", lineText);
                        sites[key].append(site);
                    }
                    for (const QString &key : order) {
                        QJsonObject node = nodes.value(key);
                        node.insert("call_sites", sites.value(key));
                        callees.append(node);
                    }
                }
            }

            int total = 0;
            bool truncated = false;
            const QJsonArray capped = capResults(callees, resultLimit(args), &total, &truncated);
            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"callees", capped}, {"total", total}, {"truncated", truncated}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_find_symbols")
            .title("Search C++ symbols in the project")
            .description(
                "Searches the project-wide C++ code model index by name - a fast "
                "\"go to symbol\". The index holds classes, enums, function DEFINITIONS, "
                "signals and type aliases; it does NOT contain plain declarations (data "
                "members, globals, or member functions that are only declared), so an "
                "empty result does NOT prove a name is unused - use cpp_get_file_symbols for "
                "the complete symbol list of a known file. Give a case-insensitive name "
                "substring; optionally restrict by \"kind\" and cap the count with "
                "\"limit\". Results are ranked (exact, then prefix, then substring) so the "
                "most relevant survive the cap; \"total_matches\" and \"truncated\" report "
                "when the cap dropped matches. Each match has its name, kind, fully "
                "qualified scope, type/signature, and file with 1-based line and column.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "query",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Case-insensitive substring to match against symbol names "
                             "(matched against the fully qualified name)."}})
                    .addProperty(
                        "kind",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"class", "function", "enum", "declaration"}},
                            {"description",
                             "Optional: restrict to one kind. \"function\" is function "
                             "definitions; \"declaration\" is type aliases and signals "
                             "(the index holds no plain declarations)."}})
                    .addProperty(
                        "limit",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Optional: maximum number of results (default 200)."}})
                    .addRequired("query"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "symbols",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description",
                             "Matching symbols across the project, most relevant first."}})
                    .addProperty(
                        "total_matches",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Total matches before the limit was applied."}})
                    .addProperty(
                        "truncated",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "True if more matches existed than the limit returned."}})
                    .addRequired("symbols")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString query = args.value("query").toString();
            if (query.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(
                    TextContent{}.text("Missing required argument \"query\"."));
            }
            const QString kind = args.value("kind").toString();
            int limit = args.value("limit").toInt();
            if (limit <= 0)
                limit = 200;

            IndexItem::ItemType typeFilter = IndexItem::All;
            if (kind == QLatin1String("class"))
                typeFilter = IndexItem::Class;
            else if (kind == QLatin1String("function"))
                typeFilter = IndexItem::Function;
            else if (kind == QLatin1String("enum"))
                typeFilter = IndexItem::Enum;
            else if (kind == QLatin1String("declaration"))
                typeFilter = IndexItem::Declaration;
            else if (!kind.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("Unknown kind \"%1\". Use class, function, enum or "
                            "declaration.").arg(kind)));
            }

            // locatorData() is the address of a by-value member, never null.
            CppLocatorData *locatorData = CppModelManager::locatorData();

            QList<IndexItem::Ptr> matches;
            locatorData->filterAllFiles([&](const IndexItem::Ptr &item) {
                if ((item->type() & typeFilter)
                        && item->scopedSymbolName().contains(query, Qt::CaseInsensitive))
                    matches.append(item);
                return IndexItem::Recurse;
            });

            // filterAllFiles traverses a QHash in unspecified order, so rank the
            // matches before truncating: exact name match first, then prefix, then
            // substring, then scope-only; ties broken deterministically so the same
            // query yields the same result and the limit never silently drops a
            // better match. Mirrors the ordering of cpplocatorfilter's matchesFor().
            const auto rankOf = [&query](const IndexItem::Ptr &item) {
                const QString name = item->symbolName();
                if (name.compare(query, Qt::CaseInsensitive) == 0)
                    return 0;
                if (name.startsWith(query, Qt::CaseInsensitive))
                    return 1;
                if (name.contains(query, Qt::CaseInsensitive))
                    return 2;
                return 3; // Matched only via the scope qualifier.
            };
            // Only the first `limit` are returned, so partial_sort is enough - a
            // broad query can match ~10^5 symbols and fully sorting them all is
            // pure waste (and this runs on the GUI thread).
            std::partial_sort(matches.begin(),
                              matches.begin() + qMin(limit, int(matches.size())),
                              matches.end(),
                      [&rankOf](const IndexItem::Ptr &a, const IndexItem::Ptr &b) {
                          const int ra = rankOf(a);
                          const int rb = rankOf(b);
                          if (ra != rb)
                              return ra < rb;
                          int c = a->scopedSymbolName().compare(b->scopedSymbolName(),
                                                                Qt::CaseInsensitive);
                          if (c != 0)
                              return c < 0;
                          c = a->filePath().toUserOutput().compare(b->filePath().toUserOutput());
                          if (c != 0)
                              return c < 0;
                          if (a->line() != b->line())
                              return a->line() < b->line();
                          return a->column() < b->column();
                      });

            const int total = int(matches.size());
            QJsonArray symbols;
            for (int i = 0; i < total && i < limit; ++i) {
                const IndexItem::Ptr &item = matches.at(i);
                QJsonObject obj{
                    {"name", item->symbolName()},
                    {"kind", itemKind(item->type())},
                    {"file", item->filePath().toUserOutput()},
                    {"line", item->line()},
                    {"column", item->column() + 1}}; // IndexItem::column() is 0-based.
                if (!item->symbolScope().isEmpty())
                    obj.insert("scope", item->symbolScope());
                if (!item->symbolType().isEmpty())
                    obj.insert("type", item->symbolType());
                if (item->type() == IndexItem::Function)
                    obj.insert("is_definition", item->isFunctionDefinition());
                symbols.append(obj);
            }

            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"symbols", symbols},
                            {"total_matches", total},
                            {"truncated", total > limit}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_rename_symbol")
            .title("Rename a C++ symbol")
            .description(
                "Renames the C++ symbol at a position across the whole project, using the "
                "code model - the rename refactoring. Give the file and a 1-based line and "
                "column on an identifier, and the \"new_name\". By default this is a DRY "
                "RUN: it returns the edits it would make (each with file, 1-based line and "
                "column, length, and the old and new text) and changes nothing. Set "
                "\"apply\" to true to write the edits. Only files belonging to the open "
                "projects are edited, never Qt or system headers; \"skipped_edits\" and "
                "\"skipped_files\" report the usages left untouched by that filter, so a "
                "partial rename is visible rather than silent. Before anything is written "
                "the new name is checked for clashes: a declaration of that name in the "
                "same scope, a base-class member it would hide or start to override, an "
                "outer declaration it would shadow, or a declaration that would capture one "
                "of the renamed usages and make it mean something else. \"conflicts\" lists "
                "them with a severity; \"apply\" is refused while a hard conflict exists "
                "unless \"force\" is true. \"other_declarations_with_name\" lists unrelated "
                "indexed symbols that already have the new name, for information.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).destructiveHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the C++ file containing the symbol."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line of the identifier to rename."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the identifier to rename."}})
                    .addProperty(
                        "new_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "The new identifier name."}})
                    .addProperty(
                        "apply",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Write the edits to disk. Default false (dry run)."}})
                    .addProperty(
                        "force",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Apply even though the new name has a hard conflict. Default "
                             "false."}})
                    .addProperty("limit", limitProperty())
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column")
                    .addRequired("new_name"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("applied", QJsonObject{{"type", "boolean"}})
                    .addProperty("symbol", QJsonObject{{"type", "string"}})
                    .addProperty("new_name", QJsonObject{{"type", "string"}})
                    .addProperty("total_edits", QJsonObject{{"type", "integer"}})
                    .addProperty("files", QJsonObject{{"type", "integer"}})
                    .addProperty(
                        "edits",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addProperty(
                        "files_changed",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addProperty("skipped_edits", QJsonObject{{"type", "integer"}})
                    .addProperty(
                        "skipped_files",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addProperty("has_conflicts", QJsonObject{{"type", "boolean"}})
                    .addProperty(
                        "conflicts",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description",
                             "Declarations the new name clashes with: kind (same_scope, "
                             "overload, hides_base_member, becomes_override, shadows, "
                             "rebinds_usage), severity (hard or soft), name, symbol_kind "
                             "and location."}})
                    .addProperty(
                        "other_declarations_with_name",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description",
                             "Indexed symbols elsewhere in the project that already carry "
                             "the new name; informational."}})
                    .addRequired("applied")
                    .addRequired("total_edits")
                    .addRequired("skipped_edits")
                    .addRequired("has_conflicts")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            const QString newName = args.value("new_name").toString();
            const bool apply = args.value("apply").toBool();
            const bool force = args.value("force").toBool();
            if (file.isEmpty() || line <= 0 || column <= 0 || newName.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\", 1-based \"line\"/\"column\" and \"new_name\"."));
            }

            static const QRegularExpression identifier(
                QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
            if (!identifier.match(newName).hasMatch()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("\"%1\" is not a valid C++ identifier.").arg(newName)));
            }
            // The lexer classifies keywords, Qt's included, as it reads them.
            CPlusPlus::SimpleLexer lexer;
            lexer.setLanguageFeatures(CPlusPlus::LanguageFeatures::defaultFeatures());
            const CPlusPlus::Tokens tokens = lexer(newName);
            if (tokens.size() != 1 || tokens.first().kind() != CPlusPlus::T_IDENTIFIER) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("\"%1\" is a C++ or Qt keyword.").arg(newName)));
            }
            const QByteArray newNameUtf8 = newName.toUtf8();

            const FilePath filePath = FilePath::fromUserInput(file);
            CPlusPlus::LookupContext context;
            CPlusPlus::Symbol *symbol = symbolAt(filePath, line, column, &context);
            if (!symbol) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ symbol found at %1:%2:%3. Is the file in an open "
                            "project and the position on an identifier?")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }
            const CPlusPlus::Identifier *id = symbol->identifier();
            if (!id) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("The symbol at %1:%2:%3 has no renamable name.")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }
            const QString oldName = QString::fromUtf8(id->chars(), id->size());
            if (newName == oldName) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("New name equals the current name \"%1\".").arg(oldName)));
            }

            // Would the new name clash? Look it up from where the symbol is
            // declared: a hit in the same scope is a redeclaration, one in a base
            // class hides that member (or, for a virtual with the same signature,
            // starts to override it), one further out gets shadowed. Below, every
            // usage is looked up from its own scope as well: a hit declared in a
            // scope that does not enclose the symbol would capture the renamed
            // reference and make it mean something else.
            // Copies what is reported about a clashing declaration, because some
            // are found in documents that symbolUsages() destroys again before the
            // result is built.
            struct Conflict
            {
                QString kind;
                bool hard = false;
                QString name;
                QString symbolKind;
                FilePath file;
                int line = 0;
                int column = 0;
            };
            CPlusPlus::Overview overview;
            QList<Conflict> conflicts;
            const auto addConflict = [&conflicts, &overview](const QString &kind, bool hard,
                                                             const CPlusPlus::Symbol *other) {
                Conflict conflict{kind, hard, overview.prettyName(other->name()),
                                  symbolKind(other), other->filePath(), other->line(),
                                  other->column()};
                for (const Conflict &known : std::as_const(conflicts)) {
                    if (known.file == conflict.file && known.line == conflict.line
                            && known.column == conflict.column) {
                        return;
                    }
                }
                conflicts.append(conflict);
            };
            const auto isEnclosedBy = [](const CPlusPlus::Scope *inner,
                                         const CPlusPlus::Scope *outer) {
                for (const CPlusPlus::Scope *scope = inner; scope; scope = scope->enclosingScope()) {
                    if (scope == outer)
                        return true;
                }
                return false;
            };
            CPlusPlus::Scope *symbolScope = symbol->enclosingScope();
            const CPlusPlus::Function *renamedFunction = symbol->type()->asFunctionType();
            if (symbolScope) {
                const CPlusPlus::Name *newNameId = context.thisDocument()->control()->identifier(
                    newNameUtf8.constData(), newNameUtf8.size());
                const QList<CPlusPlus::LookupItem> visible = context.lookup(newNameId, symbolScope);
                for (const CPlusPlus::LookupItem &item : visible) {
                    CPlusPlus::Symbol *other = item.declaration();
                    if (!other || other == symbol)
                        continue;
                    const CPlusPlus::Scope *otherScope = other->enclosingScope();
                    const CPlusPlus::Function *otherFunction = other->type()->asFunctionType();
                    const bool overload = renamedFunction && otherFunction
                                          && !sameParameters(renamedFunction, otherFunction);
                    if (otherScope == symbolScope) {
                        addConflict(overload ? QStringLiteral("overload")
                                             : QStringLiteral("same_scope"), !overload, other);
                    } else if (isEnclosedBy(symbolScope, otherScope)) {
                        addConflict(QStringLiteral("shadows"), false, other);
                    } else if (otherScope && otherScope->asClass()) {
                        // Visible from inside the class yet declared in another
                        // one: a base class member.
                        const bool overrides = renamedFunction && otherFunction && !overload
                                               && renamedFunction->isVirtual()
                                               && otherFunction->isVirtual();
                        addConflict(overrides ? QStringLiteral("becomes_override")
                                              : QStringLiteral("hides_base_member"),
                                    overrides, other);
                    } else {
                        addConflict(QStringLiteral("shadows"), false, other);
                    }
                }
            }
            const CPlusPlus::Snapshot snapshot = context.snapshot();
            const auto checkRebinding = [&](const CPlusPlus::Document::Ptr &doc,
                                            const QList<CPlusPlus::Usage> &fileUsages) {
                if (!symbolScope)
                    return;
                const CPlusPlus::Name *nameInDoc = doc->control()->identifier(
                    newNameUtf8.constData(), newNameUtf8.size());
                const CPlusPlus::LookupContext usageContext(doc, snapshot);
                for (const CPlusPlus::Usage &u : fileUsages) {
                    if (u.tags.testFlag(CPlusPlus::Usage::Tag::Declaration))
                        continue;
                    CPlusPlus::Scope *usageScope = doc->scopeAt(u.line, u.col + 1);
                    if (!usageScope)
                        continue;
                    const QList<CPlusPlus::LookupItem> items
                        = usageContext.lookup(nameInDoc, usageScope);
                    for (const CPlusPlus::LookupItem &item : items) {
                        CPlusPlus::Symbol *other = item.declaration();
                        if (!other || other == symbol)
                            continue;
                        // Declared in a scope enclosing the symbol: reported above.
                        const CPlusPlus::Scope *otherScope = other->enclosingScope();
                        if (otherScope && !isEnclosedBy(symbolScope, otherScope))
                            addConflict(QStringLiteral("rebinds_usage"), true, other);
                    }
                }
            };

            // Restrict edits to files inside an open project, so a rename never
            // rewrites Qt or system headers that a usage may point into. When no
            // project is open there is no such boundary (only explicitly parsed
            // files are in the snapshot), so the filter falls back to all usages.
            QList<FilePath> projectDirs;
            for (const ProjectExplorer::Project *project : ProjectExplorer::ProjectManager::projects())
                projectDirs.append(project->projectDirectory());
            const auto inProject = [&projectDirs](const FilePath &path) {
                if (projectDirs.isEmpty())
                    return true;
                for (const FilePath &dir : projectDirs) {
                    if (path.isChildOf(dir))
                        return true;
                }
                return false;
            };

            // The occurrences to rewrite are exactly the symbol's usages; group
            // them by file (preserving first-seen order) so each file is edited
            // in one pass. Also drives the dry-run preview so it matches what
            // apply would do.
            QList<FilePath> fileOrder;
            QHash<FilePath, QList<CPlusPlus::Usage>> byFile;
            QList<CPlusPlus::Usage> usages;
            // What the filter dropped, reported alongside the edits: a rename that
            // silently covers only part of the usages leaves the code inconsistent,
            // so the caller has to be able to see that it was partial.
            int skippedEdits = 0;
            QStringList skippedFiles;
            for (const CPlusPlus::Usage &u : symbolUsages(symbol, context, checkRebinding)) {
                if (!inProject(u.path)) {
                    ++skippedEdits;
                    const QString skipped = u.path.toUserOutput();
                    if (!skippedFiles.contains(skipped))
                        skippedFiles.append(skipped);
                    continue;
                }
                usages.append(u);
                if (!byFile.contains(u.path))
                    fileOrder.append(u.path);
                byFile[u.path].append(u);
            }
            if (usages.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No occurrences of \"%1\" in the open projects' files.")
                        .arg(oldName)));
            }

            QJsonArray conflictsJson;
            const Conflict *firstHard = nullptr;
            for (const Conflict &conflict : std::as_const(conflicts)) {
                if (conflict.hard && !firstHard)
                    firstHard = &conflict;
                QJsonObject obj{{"kind", conflict.kind},
                                {"severity", conflict.hard ? QStringLiteral("hard")
                                                           : QStringLiteral("soft")},
                                {"name", conflict.name},
                                {"symbol_kind", conflict.symbolKind}};
                if (!conflict.file.isEmpty()) {
                    obj.insert("file", conflict.file.toUserOutput());
                    obj.insert("line", conflict.line);
                    obj.insert("column", conflict.column);
                }
                conflictsJson.append(obj);
            }
            QJsonArray sameNamed;
            const QList<IndexItem::Ptr> indexed
                = CppModelManager::locatorData()->findSymbols(IndexItem::All, newName);
            for (const IndexItem::Ptr &item : indexed) {
                if (sameNamed.size() >= resultLimit(args))
                    break;
                sameNamed.append(QJsonObject{{"name", item->scopedSymbolName()},
                                             {"kind", itemKind(item->type())},
                                             {"file", item->filePath().toUserOutput()},
                                             {"line", item->line()},
                                             {"column", item->column() + 1}});
            }
            const auto withConflicts = [&](QJsonObject result) {
                result.insert("has_conflicts", !conflicts.isEmpty());
                result.insert("conflicts", conflictsJson);
                result.insert("other_declarations_with_name", sameNamed);
                return result;
            };

            if (apply && firstHard && !force) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("Cannot apply: \"%1\" clashes with the %2 %3 at %4:%5 (%6). "
                            "Nothing was changed; pass \"force\" to rename anyway.")
                        .arg(newName, firstHard->symbolKind, firstHard->name,
                             firstHard->file.toUserOutput(), QString::number(firstHard->line),
                             firstHard->kind)));
            }

            if (!apply) {
                QJsonArray edits;
                for (const CPlusPlus::Usage &u : usages) {
                    QJsonObject edit{
                        {"file", u.path.toUserOutput()},
                        {"line", u.line},
                        {"column", u.col + 1}, // Usage::col is 0-based.
                        {"length", u.len},
                        {"old_text", oldName},
                        {"new_text", newName}};
                    const QString lineText = u.lineText.trimmed();
                    if (!lineText.isEmpty())
                        edit.insert("line_text", lineText);
                    edits.append(edit);
                }
                int total = 0;
                bool truncated = false;
                const QJsonArray capped = capResults(edits, resultLimit(args), &total, &truncated);
                return CallToolResult{}.isError(false).structuredContent(withConflicts(QJsonObject{
                    {"applied", false},
                    {"symbol", oldName},
                    {"new_name", newName},
                    {"total_edits", total},
                    {"files", int(fileOrder.size())},
                    {"edits", capped},
                    {"skipped_edits", skippedEdits},
                    {"skipped_files", QJsonArray::fromStringList(skippedFiles)},
                    {"truncated", truncated}}));
            }

            // Refuse before touching anything if any target is read-only, so we
            // never trip the modal read-only-files dialog in a headless server.
            QStringList notWritable;
            for (const FilePath &fp : fileOrder) {
                if (!fp.isWritableFile())
                    notWritable.append(fp.toUserOutput());
            }
            if (!notWritable.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("Cannot apply: not writable: %1").arg(notWritable.join(", "))));
            }

            // Build and check every change set before applying the first one.
            // The usages come from the snapshot, the edits go to the editor
            // document, which may be newer, so verify that each offset still
            // holds the old name rather than rewrite an arbitrary span.
            CppRefactoringChanges changes(snapshot);
            QList<TextEditor::RefactoringFilePtr> refFiles;
            QList<Utils::ChangeSet> changeSets;
            for (const FilePath &fp : fileOrder) {
                const TextEditor::RefactoringFilePtr refFile = changes.file(fp);
                Utils::ChangeSet changeSet;
                for (const CPlusPlus::Usage &u : byFile.value(fp)) {
                    const int start = refFile->position(u.line, u.col + 1);
                    if (start < 0 || refFile->textOf(start, start + u.len) != oldName) {
                        return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                            QString("Cannot apply: %1:%2:%3 no longer holds \"%4\". The code "
                                    "model is out of date, nothing was changed.")
                                .arg(fp.toUserOutput()).arg(u.line).arg(u.col + 1).arg(oldName)));
                    }
                    changeSet.replace(start, start + u.len, newName);
                }
                refFiles.append(refFile);
                changeSets.append(changeSet);
            }

            int applied = 0;
            QJsonArray filesChanged;
            QStringList failed;
            for (int i = 0; i < fileOrder.size(); ++i) {
                const FilePath &fp = fileOrder.at(i);
                if (refFiles.at(i)->apply(changeSets.at(i))) {
                    applied += int(byFile.value(fp).size());
                    filesChanged.append(fp.toUserOutput());
                } else {
                    failed.append(fp.toUserOutput());
                }
            }
            if (!failed.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("Renamed %1 of %2 occurrences, writing failed for: %3")
                        .arg(applied).arg(usages.size()).arg(failed.join(", "))));
            }

            return CallToolResult{}.isError(false).structuredContent(withConflicts(QJsonObject{
                {"applied", true},
                {"symbol", oldName},
                {"new_name", newName},
                {"total_edits", applied},
                {"files_changed", filesChanged},
                {"skipped_edits", skippedEdits},
                {"skipped_files", QJsonArray::fromStringList(skippedFiles)}}));
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_get_quick_fixes")
            .title("Get C++ quick-fixes")
            .description(
                "Lists the C++ quick-fixes and refactoring actions the editor offers at a "
                "position - what \"Alt+Enter\" would show - each with its description. Give "
                "the file and a 1-based line and column. This only lists the available "
                "actions; it does not apply them. The file is opened in an editor if it is "
                "not already, and must belong to an open project and be parsed (the actions "
                "depend on the editor's semantic info being up to date).")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Absolute path to the C++ file."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line of the position."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the position."}})
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "quick_fixes",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addRequired("quick_fixes")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            if (file.isEmpty() || line <= 0 || column <= 0) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\" and 1-based \"line\" and \"column\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            if (!CppModelManager::document(filePath)) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ code model document for \"%1\". Is it a C++ file that "
                            "belongs to an open project?").arg(filePath.toUserOutput())));
            }

            Core::IEditor *editor = Core::EditorManager::openEditor(
                filePath, {},
                Core::EditorManager::DoNotChangeCurrentEditor
                    | Core::EditorManager::DoNotMakeVisible);
            auto cppWidget = qobject_cast<CppEditorWidget *>(
                TextEditor::TextEditorWidget::fromEditor(editor));
            if (!cppWidget) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("Could not open \"%1\" in a C++ editor.")
                        .arg(filePath.toUserOutput())));
            }

            // Semantic info arrives asynchronously, so a file this call just
            // opened has none yet. Say so instead of reporting no quick-fixes.
            if (!cppWidget->isSemanticInfoValid()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("\"%1\" is not parsed yet. Try again once the code model "
                            "has caught up.").arg(filePath.toUserOutput())));
            }

            QTextDocument *doc = cppWidget->document();
            const QTextBlock block = doc->findBlockByNumber(line - 1);
            if (!block.isValid()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("Line %1 is out of range in \"%2\".")
                        .arg(line).arg(filePath.toUserOutput())));
            }
            const QTextCursor savedCursor = cppWidget->textCursor();
            const QScopeGuard restoreCursor([cppWidget, savedCursor] {
                cppWidget->setTextCursor(savedCursor);
            });
            QTextCursor cursor(doc);
            cursor.setPosition(block.position() + qMin(column - 1, block.length() - 1));
            cppWidget->setTextCursor(cursor);

            const CppQuickFixInterface interface(cppWidget, TextEditor::ExplicitlyInvoked);
            if (interface.path().isEmpty()) {
                return CallToolResult{}.isError(false).structuredContent(
                    QJsonObject{{"quick_fixes", QJsonArray()}});
            }

            QJsonArray quickFixes;
            for (CppQuickFixFactory *factory : CppQuickFixFactory::cppQuickFixFactories()) {
                CppQuickFixFactory::QuickFixOperations operations;
                factory->match(interface, operations);
                for (const TextEditor::QuickFixOperation::Ptr &op : std::as_const(operations)) {
                    const QString description = op->description();
                    if (!description.isEmpty())
                        quickFixes.append(QJsonObject{{"description", description}});
                }
            }

            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"quick_fixes", quickFixes}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_find_overrides")
            .title("Find C++ virtual function overrides")
            .description(
                "Finds the overriding implementations of the virtual C++ member function "
                "at a position - go to implementation(s) - across the class hierarchy, "
                "plus the base declaration(s) it overrides. Give the file and a 1-based "
                "line and column on a function name; returns \"overrides\" (each with its "
                "fully qualified name, signature and location), \"base_declarations\", and "
                "whether the function is virtual. The file must belong to an open project.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the C++ file containing the function."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line of the function name."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the function name."}})
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("is_virtual", QJsonObject{{"type", "boolean"}})
                    .addProperty(
                        "overrides",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addProperty(
                        "base_declarations",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addRequired("overrides")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            if (file.isEmpty() || line <= 0 || column <= 0) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\" and 1-based \"line\" and \"column\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            CPlusPlus::LookupContext context;
            CPlusPlus::Symbol *symbol = symbolAt(filePath, line, column, &context);
            // A member function declaration resolves to a Declaration whose type
            // is the function; unwrap it (this also passes a Function through).
            CPlusPlus::Function *function
                = symbol ? symbol->type()->asFunctionType() : nullptr;
            if (!function) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ function found at %1:%2:%3. Is the file in an open "
                            "project and the position on a function name?")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }

            const CPlusPlus::Snapshot snapshot = context.snapshot();
            SymbolFinder finder;
            CPlusPlus::Class *functionsClass
                = finder.findMatchingClassDeclaration(function, snapshot);
            if (!functionsClass) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Could not find the class that declares the function."));
            }

            CPlusPlus::Overview overview;
            const auto functionToJson = [&overview](const CPlusPlus::Function *f) {
                return QJsonObject{
                    {"name", overview.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(
                                 const_cast<CPlusPlus::Function *>(f)))},
                    {"signature", overview.prettyType(f->type(), f->name())},
                    {"file", f->filePath().toUserOutput()},
                    {"line", f->line()},
                    {"column", f->column()}};
            };

            QList<const CPlusPlus::Function *> firstVirtuals;
            const bool isVirtual
                = FunctionUtils::isVirtualFunction(function, context, &firstVirtuals);

            QJsonArray overrides;
            if (isVirtual) {
                const QList<CPlusPlus::Function *> functions = FunctionUtils::overrides(
                    function, functionsClass, functionsClass, snapshot);
                for (const CPlusPlus::Function *f : functions)
                    overrides.append(functionToJson(f));
            }
            QJsonArray baseDeclarations;
            for (const CPlusPlus::Function *f : std::as_const(firstVirtuals))
                baseDeclarations.append(functionToJson(f));

            return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                {"is_virtual", isVirtual},
                {"overrides", overrides},
                {"base_declarations", baseDeclarations}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_get_include_hierarchy")
            .title("Get C++ include hierarchy")
            .description(
                "Returns the include hierarchy of a C++ file from the code model: the files "
                "it includes directly (each with the 1-based line of the #include, the name "
                "as written, and whether it was a <global> or a \"local\" include), the "
                "files that include it directly (each with the line of their #include), and "
                "the #includes that could not be resolved to a file. Set \"transitive\" to "
                "also get the flattened closures: every file it pulls in, and every file "
                "that depends on it. The file must be known to the code model, i.e. a C++ "
                "source or header that belongs to an open project or is included by one.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Absolute path to the C++ source or header file."}})
                    .addProperty(
                        "transitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Also return \"transitive_includes\" and "
                             "\"transitive_included_by\". Default false."}})
                    .addProperty("limit", limitProperty())
                    .addRequired("file"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("file", QJsonObject{{"type", "string"}})
                    .addProperty(
                        "includes",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description", "Files included directly, in #include order."}})
                    .addProperty(
                        "included_by",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description", "Files whose #include resolves to this file."}})
                    .addProperty(
                        "unresolved_includes",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description",
                             "#includes the code model could not resolve to a file."}})
                    .addProperty(
                        "transitive_includes",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addProperty(
                        "transitive_included_by",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addProperty(
                        "totals",
                        QJsonObject{
                            {"type", "object"},
                            {"description", "Size of each list before the limit was applied."}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addRequired("file")
                    .addRequired("includes")
                    .addRequired("included_by")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            if (file.isEmpty()) {
                return CallToolResult{}.isError(true).addContent(
                    TextContent{}.text("Missing required argument \"file\"."));
            }
            const bool transitive = args.value("transitive").toBool();
            const int limit = resultLimit(args);

            const FilePath filePath = FilePath::fromUserInput(file);
            const CPlusPlus::Snapshot snapshot = CppModelManager::snapshot();
            const CPlusPlus::Document::Ptr doc = snapshot.document(filePath);
            if (!doc) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ code model document for \"%1\". Is it a C++ file that "
                            "belongs to an open project?")
                        .arg(filePath.toUserOutput())));
            }

            const auto isGlobal = [](const CPlusPlus::Document::Include &include) {
                return include.type() == CPlusPlus::Client::IncludeGlobal;
            };
            QJsonArray includes;
            for (const CPlusPlus::Document::Include &include : doc->resolvedIncludes()) {
                includes.append(QJsonObject{
                    {"file", include.resolvedFileName().toUserOutput()},
                    {"line", include.line()},
                    {"name", include.unresolvedFileName()},
                    {"global", isGlobal(include)}});
            }
            QJsonArray unresolved;
            for (const CPlusPlus::Document::Include &include : doc->unresolvedIncludes()) {
                unresolved.append(QJsonObject{
                    {"name", include.unresolvedFileName()},
                    {"line", include.line()},
                    {"global", isGlobal(include)}});
            }

            // The snapshot is a hash, so its includers come back in no particular
            // order; sort them so the same question gets the same answer.
            QList<QPair<QString, int>> includers;
            const QList<CPlusPlus::Snapshot::IncludeLocation> locations
                = snapshot.includeLocationsOfDocument(filePath);
            for (const CPlusPlus::Snapshot::IncludeLocation &location : locations)
                includers.append({location.first->filePath().toUserOutput(), location.second});
            std::sort(includers.begin(), includers.end());
            QJsonArray includedBy;
            for (const QPair<QString, int> &includer : std::as_const(includers))
                includedBy.append(QJsonObject{{"file", includer.first}, {"line", includer.second}});

            QJsonObject totals;
            bool truncated = false;
            const auto capped = [&](const QString &key, const QJsonArray &array) {
                int total = 0;
                bool listTruncated = false;
                const QJsonArray result = capResults(array, limit, &total, &listTruncated);
                totals.insert(key, total);
                truncated = truncated || listTruncated;
                return result;
            };
            QJsonObject result{{"file", filePath.toUserOutput()}};
            result.insert("includes", capped("includes", includes));
            result.insert("included_by", capped("included_by", includedBy));
            result.insert("unresolved_includes", capped("unresolved_includes", unresolved));

            if (transitive) {
                const auto sortedPaths = [&filePath](const QSet<FilePath> &paths) {
                    QStringList names;
                    for (const FilePath &path : paths) {
                        if (path != filePath) // A cycle would list the file itself.
                            names.append(path.toUserOutput());
                    }
                    names.sort();
                    return QJsonArray::fromStringList(names);
                };
                result.insert("transitive_includes",
                              capped("transitive_includes",
                                     sortedPaths(snapshot.allIncludesForDocument(filePath))));
                const FilePaths dependents = snapshot.filesDependingOn(filePath);
                result.insert("transitive_included_by",
                              capped("transitive_included_by",
                                     sortedPaths(QSet<FilePath>(dependents.begin(),
                                                                dependents.end()))));
            }

            result.insert("totals", totals);
            result.insert("truncated", truncated);
            return CallToolResult{}.isError(false).structuredContent(result);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("cpp_find_signal_connections")
            .title("Find Qt signal/slot connections")
            .description(
                "Finds the signal/slot connections involving the C++ function at a "
                "position, by scanning the connect() and disconnect() calls the code model "
                "can see. Give the file and a 1-based line and column on a signal, slot or "
                "other member function. Ask with a signal to learn which slots it is "
                "connected to, with a slot to learn which signals trigger it. Each "
                "connection reports its location, whether it is a connect or disconnect, "
                "the \"role\" the function plays in it (signal or slot), the sender, "
                "signal, receiver and slot arguments as written, how the slot is given "
                "(\"slot_kind\": qt4_macro for SIGNAL()/SLOT(), member_pointer for "
                "&Class::member, lambda, or another expression), an optional "
                "connection_type, and the position of the \"counterpart\" argument, ready "
                "for cpp_get_symbol_info. Limits: only textual connect/disconnect calls in "
                "files the code model has parsed are seen - not connections made in .ui "
                "files, by connectSlotsByName, from QML, or through wrapper functions. A "
                "SIGNAL()/SLOT() macro the code model cannot resolve is matched by name "
                "alone and reported with \"resolved\" false.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the C++ file containing the function."}})
                    .addProperty(
                        "line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based line of the function name."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the function name."}})
                    .addProperty("limit", limitProperty())
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "symbol",
                        QJsonObject{
                            {"type", "object"},
                            {"description",
                             "The function asked about: name, qualified_name, is_signal, "
                             "is_slot."}})
                    .addProperty(
                        "connections",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}},
                            {"description",
                             "connect()/disconnect() calls involving the function, in "
                             "file and line order."}})
                    .addProperty("total", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addRequired("symbol")
                    .addRequired("connections")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            if (file.isEmpty() || line <= 0 || column <= 0) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\" and 1-based \"line\" and \"column\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            CPlusPlus::LookupContext context;
            CPlusPlus::Symbol *symbol = symbolAt(filePath, line, column, &context);
            if (!symbol) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ symbol found at %1:%2:%3. Is the file in an open project "
                            "and the position on a function name?")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }
            const CPlusPlus::Function *function = symbol->type()->asFunctionType();
            const CPlusPlus::Identifier *id = symbol->identifier();
            if (!function || !id) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("The symbol at %1:%2:%3 is not a function. Point at a signal, "
                            "a slot or another member function.")
                        .arg(filePath.toUserOutput()).arg(line).arg(column)));
            }
            const QByteArray symbolName(id->chars(), id->size());

            CPlusPlus::Overview overview;
            QJsonObject symbolJson{{"name", overview.prettyName(symbol->name())},
                                   {"is_signal", function->isSignal()},
                                   {"is_slot", function->isSlot()}};
            const QString qualified
                = overview.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol));
            if (!qualified.isEmpty())
                symbolJson.insert("qualified_name", qualified);

            // Is the 1-based position inside the token range [first, last)?
            const auto contains = [](CPlusPlus::TranslationUnit *unit, const CPlusPlus::AST *ast,
                                     int posLine, int posColumn) {
                int startLine = 0;
                int startColumn = 0;
                int endLine = 0;
                int endColumn = 0;
                unit->getTokenPosition(ast->firstToken(), &startLine, &startColumn);
                unit->getTokenEndPosition(ast->lastToken() - 1, &endLine, &endColumn);
                const QPair<int, int> pos(posLine, posColumn);
                return pos >= QPair<int, int>(startLine, startColumn)
                       && pos < QPair<int, int>(endLine, endColumn);
            };

            // Like symbolUsages(): parse each file that mentions the name once, and
            // take both the usages of the symbol and the connect() calls from that
            // parse. A call counts if an argument holds a resolved usage, or, for a
            // SIGNAL()/SLOT() macro, names the function - the built-in model cannot
            // resolve a method written inside a macro from outside its class.
            struct Connection
            {
                QString file;
                int line = 0;
                int column = 0;
                QJsonObject json;
            };
            QList<Connection> connections;
            const CPlusPlus::Snapshot snapshot = context.snapshot();
            const WorkingCopy workingCopy = CppModelManager::workingCopy();
            for (auto it = snapshot.begin(), end = snapshot.end(); it != end; ++it) {
                const FilePath path = it.key();
                if (!it.value()->control()->findIdentifier(id->chars(), id->size()))
                    continue;
                const QByteArray source = fileSource(path, workingCopy);
                CPlusPlus::Document::Ptr doc = snapshot.preprocessedDocument(source, path);
                doc->tokenize();
                if (!doc->control()->findIdentifier(id->chars(), id->size()))
                    continue;
                doc->check();
                CPlusPlus::TranslationUnit *unit = doc->translationUnit();
                if (!unit || !unit->ast())
                    continue;

                ConnectCallCollector collector(unit);
                unit->ast()->accept(&collector);
                if (collector.calls.isEmpty())
                    continue;

                CPlusPlus::FindUsages findUsages(source, doc, snapshot, /*categorize=*/false);
                findUsages(symbol);
                QList<QPair<int, int>> usagePositions;
                const QList<CPlusPlus::Usage> usages = findUsages.usages();
                for (const CPlusPlus::Usage &u : usages)
                    usagePositions.append({u.line, u.col + 1}); // Usage::col is 0-based.

                const QStringList lines = QString::fromUtf8(source).split('\n');
                for (CPlusPlus::CallAST *call : std::as_const(collector.calls)) {
                    QList<CPlusPlus::ExpressionAST *> arguments;
                    for (CPlusPlus::ExpressionListAST *arg = call->expression_list; arg;
                         arg = arg->next) {
                        if (arg->value)
                            arguments.append(arg->value);
                    }
                    // connect(sender, signal, slot) is the shortest form that
                    // names both ends.
                    if (arguments.size() < 3)
                        continue;

                    int symbolArgument = -1;
                    bool resolved = false;
                    for (int i = 0; i < arguments.size() && symbolArgument < 0; ++i) {
                        for (const QPair<int, int> &pos : std::as_const(usagePositions)) {
                            if (contains(unit, arguments.at(i), pos.first, pos.second)) {
                                symbolArgument = i;
                                resolved = true;
                                break;
                            }
                        }
                    }
                    for (int i = 1; i < arguments.size() && symbolArgument < 0; ++i) {
                        if (qtMethodName(arguments.at(i)) == symbolName)
                            symbolArgument = i;
                    }
                    if (symbolArgument < 0)
                        continue;

                    // Three arguments: connect(sender, signal, slot-or-functor);
                    // otherwise the receiver comes third and the slot fourth.
                    const int slotIndex = arguments.size() == 3 ? 2 : 3;
                    QString role = QStringLiteral("other");
                    if (symbolArgument == 1)
                        role = QStringLiteral("signal");
                    else if (symbolArgument == slotIndex)
                        role = QStringLiteral("slot");
                    else if (symbolArgument == 0)
                        role = QStringLiteral("sender");
                    else if (symbolArgument == 2)
                        role = QStringLiteral("receiver");

                    CPlusPlus::ExpressionAST *slotArgument = arguments.at(slotIndex);
                    const QString slotKind = connectArgumentKind(slotArgument, unit);
                    int callLine = 0;
                    int callColumn = 0;
                    unit->getTokenPosition(call->firstToken(), &callLine, &callColumn);
                    QJsonObject json{
                        {"file", path.toUserOutput()},
                        {"line", callLine},
                        {"column", callColumn},
                        {"kind", QString::fromUtf8(calleeName(call))},
                        {"role", role},
                        {"resolved", resolved},
                        {"sender", astText(lines, unit, arguments.at(0))},
                        {"signal", astText(lines, unit, arguments.at(1))},
                        {"slot", astText(lines, unit, slotArgument)},
                        {"slot_kind", slotKind}};
                    if (slotIndex == 3)
                        json.insert("receiver", astText(lines, unit, arguments.at(2)));
                    else if (slotKind == QLatin1String("qt4_macro"))
                        json.insert("receiver", QStringLiteral("this")); // Qt 4 three-argument form.
                    if (arguments.size() >= 5)
                        json.insert("connection_type", astText(lines, unit, arguments.at(4)));
                    if (symbolArgument == slotIndex && slotKind == QLatin1String("lambda"))
                        json.insert("via_lambda", true); // The function is used inside the lambda.
                    const QString lineText = lines.value(callLine - 1).trimmed();
                    if (!lineText.isEmpty())
                        json.insert("line_text", lineText);
                    if (symbolArgument == 1 || symbolArgument == slotIndex) {
                        CPlusPlus::ExpressionAST *counterpart
                            = arguments.at(symbolArgument == 1 ? slotIndex : 1);
                        int counterpartLine = 0;
                        int counterpartColumn = 0;
                        unit->getTokenPosition(argumentNameToken(counterpart), &counterpartLine,
                                               &counterpartColumn);
                        json.insert("counterpart", QJsonObject{{"file", path.toUserOutput()},
                                                               {"line", counterpartLine},
                                                               {"column", counterpartColumn}});
                    }
                    connections.append({path.toUserOutput(), callLine, callColumn, json});
                }
            }

            // The snapshot is a hash; sort so the same question gets the same answer.
            std::sort(connections.begin(), connections.end(),
                      [](const Connection &a, const Connection &b) {
                          if (a.file != b.file)
                              return a.file < b.file;
                          if (a.line != b.line)
                              return a.line < b.line;
                          return a.column < b.column;
                      });
            QJsonArray array;
            for (const Connection &connection : std::as_const(connections))
                array.append(connection.json);

            int total = 0;
            bool truncated = false;
            const QJsonArray capped = capResults(array, resultLimit(args), &total, &truncated);
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                {"symbol", symbolJson},
                {"connections", capped},
                {"total", total},
                {"truncated", truncated}});
        });
}

} // namespace CppEditor::Internal
