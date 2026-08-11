// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "cppcanonicalsymbol.h"
#include "cppindexingsupport.h"
#include "cppmodelmanager.h"
#include "cppworkingcopy.h"
#include "indexitem.h"
#include "searchsymbols.h"

#include <mcp/server/toolregistry.h>

#include <cplusplus/Control.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/FindUsages.h>
#include <cplusplus/Literals.h>

#include <utils/filepath.h>
#include <utils/result.h>

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
}

} // namespace CppEditor::Internal
