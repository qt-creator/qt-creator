// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "cppindexingsupport.h"
#include "cppmodelmanager.h"
#include "indexitem.h"
#include "searchsymbols.h"

#include <mcp/server/toolregistry.h>

#include <cplusplus/CppDocument.h>

#include <utils/filepath.h>
#include <utils/result.h>

#include <QJsonArray>
#include <QJsonObject>

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
}

} // namespace CppEditor::Internal
