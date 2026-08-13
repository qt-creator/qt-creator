// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "cppcanonicalsymbol.h"
#include "cppcompletionassist.h"
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

#include <projectexplorer/headerpath.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/texteditor.h>
#include <texteditor/codeassist/assistenums.h>
#include <texteditor/codeassist/assistproposaliteminterface.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/quickfix.h>

#include <cplusplus/AST.h>
#include <cplusplus/ASTVisitor.h>
#include <cplusplus/Control.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/FindUsages.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TranslationUnit.h>

#include <utils/changeset.h>
#include <utils/filepath.h>
#include <utils/result.h>
#include <utils/textutils.h>

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
// (via context) its lookup context, or nullptr if there is none.
static CPlusPlus::Symbol *symbolAt(const Utils::FilePath &filePath, int line, int column,
                                   CPlusPlus::LookupContext *context)
{
    const CPlusPlus::Document::Ptr doc = CppModelManager::document(filePath);
    if (!doc)
        return nullptr;
    const CPlusPlus::Snapshot snapshot = CppModelManager::snapshot();
    const QByteArray source = fileSource(filePath, CppModelManager::workingCopy());
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
// the symbol's identifier are skipped cheaply, mirroring CppFindReferences.
static QList<CPlusPlus::Usage> symbolUsages(CPlusPlus::Symbol *symbol,
                                            const CPlusPlus::LookupContext &context)
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
        CPlusPlus::Document::Ptr doc = snapshot.preprocessedDocument(source, filePath);
        doc->tokenize();
        if (!doc->control()->findIdentifier(id->chars(), id->size()))
            continue;
        doc->check();
        CPlusPlus::FindUsages findUsages(source, doc, snapshot, /*categorize=*/true);
        findUsages(symbol);
        usages += findUsages.usages();
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

void registerMcpTools()
{
    using namespace Mcp::Schema;
    using Mcp::ToolRegistry;

    ToolRegistry::registerTool(
        Tool{}
            .name("get_file_symbols")
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
                        {"column", item->column()}};
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
            .name("get_file_problems")
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
            .name("find_references")
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
            .name("get_symbol_info")
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
            .name("get_type_hierarchy")
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
            .name("find_callers")
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
            .name("find_callees")
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
            .name("find_symbols")
            .title("Search C++ symbols in the project")
            .description(
                "Searches the project-wide C++ code model index by name - a fast "
                "\"go to symbol\". The index holds classes, enums, function DEFINITIONS, "
                "signals and type aliases; it does NOT contain plain declarations (data "
                "members, globals, or member functions that are only declared), so an "
                "empty result does NOT prove a name is unused - use get_file_symbols for "
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
            .name("rename_symbol")
            .title("Rename a C++ symbol")
            .description(
                "Renames the C++ symbol at a position across the whole project, using the "
                "code model - the rename refactoring. Give the file and a 1-based line and "
                "column on an identifier, and the \"new_name\". By default this is a DRY "
                "RUN: it returns the edits it would make (each with file, 1-based line and "
                "column, length, and the old and new text) and changes nothing. Set "
                "\"apply\" to true to write the edits. The file must belong to an open "
                "project.")
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
                    .addRequired("applied")
                    .addRequired("total_edits")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            const QString newName = args.value("new_name").toString();
            const bool apply = args.value("apply").toBool();
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

            // The occurrences to rewrite are exactly the symbol's usages; group
            // them by file (preserving first-seen order) so each file is edited
            // in one pass. Also drives the dry-run preview so it matches what
            // apply would do.
            QList<FilePath> fileOrder;
            QHash<FilePath, QList<CPlusPlus::Usage>> byFile;
            const QList<CPlusPlus::Usage> usages = symbolUsages(symbol, context);
            for (const CPlusPlus::Usage &u : usages) {
                if (!byFile.contains(u.path))
                    fileOrder.append(u.path);
                byFile[u.path].append(u);
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
                return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                    {"applied", false},
                    {"symbol", oldName},
                    {"new_name", newName},
                    {"total_edits", total},
                    {"files", int(fileOrder.size())},
                    {"edits", capped},
                    {"truncated", truncated}});
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
            CppRefactoringChanges changes(context.snapshot());
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

            return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                {"applied", true},
                {"symbol", oldName},
                {"new_name", newName},
                {"total_edits", applied},
                {"files_changed", filesChanged}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("get_completions")
            .title("Get C++ code completions")
            .description(
                "Returns the C++ code-completion proposals at a position, as the editor "
                "would offer them - useful before writing code. Give the file and a "
                "1-based line and column (the cursor point, e.g. just after a \".\" or "
                "\"::\"); returns the candidate completions, each with its text and any "
                "detail (signature/type), filtered and ranked by the prefix already typed. "
                "The file is opened in an editor if it is not already, and must belong to "
                "an open project.")
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
                            {"description", "1-based line of the cursor position."}})
                    .addProperty(
                        "column",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "1-based column of the cursor position."}})
                    .addProperty("limit", limitProperty())
                    .addRequired("file")
                    .addRequired("line")
                    .addRequired("column"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "completions",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addProperty("total", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addRequired("completions")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const int line = args.value("line").toInt();
            const int column = args.value("column").toInt();
            const int limit = resultLimit(args);
            if (file.isEmpty() || line <= 0 || column <= 0) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    "Requires \"file\" and 1-based \"line\" and \"column\"."));
            }

            const FilePath filePath = FilePath::fromUserInput(file);
            const CPlusPlus::Document::Ptr cppDocument = CppModelManager::document(filePath);
            if (!cppDocument) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("No C++ code model document for \"%1\". Is it a C++ file that "
                            "belongs to an open project?").arg(filePath.toUserOutput())));
            }

            Core::IEditor *editor = Core::EditorManager::openEditor(
                filePath, {},
                Core::EditorManager::DoNotChangeCurrentEditor
                    | Core::EditorManager::DoNotMakeVisible);
            TextEditor::TextEditorWidget *widget = TextEditor::TextEditorWidget::fromEditor(editor);
            if (!widget) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("Could not open \"%1\" in a text editor.")
                        .arg(filePath.toUserOutput())));
            }

            QTextDocument *doc = widget->document();
            const QTextBlock block = doc->findBlockByNumber(line - 1);
            if (!block.isValid()) {
                return CallToolResult{}.isError(true).addContent(TextContent{}.text(
                    QString("Line %1 is out of range in \"%2\".")
                        .arg(line).arg(filePath.toUserOutput())));
            }
            // CppCompletionAssistInterface takes its position from the widget's
            // cursor, so the caret has to move there and back again.
            const int cursorPos = block.position() + qMin(column - 1, block.length() - 1);
            const QTextCursor savedCursor = widget->textCursor();
            const QScopeGuard restoreCursor([widget, savedCursor] {
                widget->setTextCursor(savedCursor);
            });
            QTextCursor cursor(doc);
            cursor.setPosition(cursorPos);
            widget->setTextCursor(cursor);

            CPlusPlus::LanguageFeatures features = cppDocument->languageFeatures();
            ProjectExplorer::HeaderPaths headerPaths;
            const QList<ProjectPart::ConstPtr> parts = CppModelManager::projectPart(filePath);
            if (!parts.isEmpty()) {
                features = parts.first()->languageFeatures;
                headerPaths = parts.first()->headerPaths;
            }
            auto interface = std::make_unique<CppCompletionAssistInterface>(
                filePath, widget, TextEditor::ExplicitlyInvoked, CppModelManager::snapshot(),
                headerPaths, features);
            interface->prepareForAsyncUse();
            interface->recreateTextDocument();

            InternalCppCompletionAssistProcessor processor;
            processor.setupAssistInterface(std::move(interface));
            const std::unique_ptr<TextEditor::IAssistProposal> proposal(processor.performAsync());

            QJsonArray completions;
            int total = 0;
            if (proposal) {
                // A position inside a call's argument list yields a function-hint
                // proposal, whose model is not a CppAssistProposalModel.
                if (const CppAssistProposalModelPtr model
                        = proposal->model().dynamicCast<CppAssistProposalModel>()) {
                    const int base = proposal->basePosition();
                    const QString prefix = Utils::Text::textAt(doc, base, cursorPos - base);
                    if (!prefix.isEmpty())
                        model->filter(prefix);
                    if (model->isSortable(prefix))
                        model->sort(prefix);
                    total = model->size();
                    for (int i = 0; i < total && completions.size() < limit; ++i) {
                        QJsonObject item{{"text", model->text(i)}};
                        if (const TextEditor::AssistProposalItemInterface *p = model->proposalItem(i)) {
                            const QString detail = p->detail();
                            if (!detail.isEmpty())
                                item.insert("detail", detail);
                        }
                        completions.append(item);
                    }
                }
            }

            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"completions", completions},
                            {"total", total},
                            {"truncated", total > completions.size()}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("get_quick_fixes")
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
                for (const TextEditor::QuickFixOperation::Ptr &op : operations) {
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
            .name("find_overrides")
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
            for (const CPlusPlus::Function *f : firstVirtuals)
                baseDeclarations.append(functionToJson(f));

            return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                {"is_virtual", isVirtual},
                {"overrides", overrides},
                {"base_declarations", baseDeclarations}});
        });
}

} // namespace CppEditor::Internal
