// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "cppcanonicalsymbol.h"
#include "cppelementevaluator.h"
#include "cppindexingsupport.h"
#include "cppmodelmanager.h"
#include "cppworkingcopy.h"
#include "indexitem.h"
#include "searchsymbols.h"
#include "symbolfinder.h"

#include <mcp/server/toolregistry.h>

#include <cplusplus/AST.h>
#include <cplusplus/ASTVisitor.h>
#include <cplusplus/Control.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/FindUsages.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TranslationUnit.h>

#include <utils/filepath.h>
#include <utils/result.h>

#include <QFuture>
#include <QFutureInterface>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>

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

            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"symbols", symbols}});
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

            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"references", references}});
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

            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"callers", callers}});
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

            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"callees", callees}});
        });
}

} // namespace CppEditor::Internal
