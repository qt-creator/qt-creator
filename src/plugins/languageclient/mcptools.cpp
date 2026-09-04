// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcptools.h"

#include "client.h"
#include "languageclientmanager.h"

#include <mcp/server/toolregistry.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <languageserverprotocol/callhierarchy.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/lsptypes.h>
#include <languageserverprotocol/servercapabilities.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QDeadlineTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>

#include <algorithm>
#include <memory>
#include <optional>
#include <variant>

using namespace LanguageServerProtocol;
using namespace TextEditor;
using namespace Utils;

namespace LanguageClient {

// How long a document a tool opened stays open after the last question about
// it, and how long to wait for a server that is still starting.
constexpr int idleCloseMs = 2 * 60 * 1000;
constexpr int serverStartTimeoutMs = 15 * 1000;

// ---------------------------------------------------------------- arguments

struct LocationArgs
{
    FilePath file;
    int line = 0;
    int column = 0;
};

static Result<LocationArgs> parseLocationArgs(const QJsonObject &args)
{
    const QString file = args.value("file").toString();
    const int line = args.value("line").toInt();
    const int column = args.value("column").toInt();
    if (file.isEmpty() || line <= 0 || column <= 0)
        return ResultError(QString("Requires \"file\" and 1-based \"line\" and \"column\"."));
    return LocationArgs{FilePath::fromUserInput(file), line, column};
}

constexpr int defaultResultLimit = 200;
constexpr int maxResultLimit = 1000;

// The optional "limit" argument, clamped so a client cannot ask for an
// unbounded answer.
static int resultLimit(const QJsonObject &args)
{
    const int limit = args.value("limit").toInt();
    return std::clamp(limit > 0 ? limit : defaultResultLimit, 1, maxResultLimit);
}

static QJsonObject limitProperty()
{
    return QJsonObject{{"type", "integer"},
                       {"description", QString("Maximum number of results (default %1, "
                                               "capped at %2).")
                                           .arg(defaultResultLimit).arg(maxResultLimit)}};
}

// --------------------------------------------- documents the tools opened

// A server only knows about open documents, so a tool asked about a file that
// is not open opens it in a hidden editor. Such documents stay open for a
// while, so that an agent asking several questions about one file does not
// make the server parse it again each time, and are closed once idle. One the
// user has since edited or brought into view is no longer ours to close.
struct OpenedDocument
{
    QPointer<TextDocument> document;
    QDeadlineTimer idle;
};

static QList<OpenedDocument> &openedDocuments()
{
    static QList<OpenedDocument> documents;
    return documents;
}

static bool sweepScheduled = false;

static void closeOpenedDocuments(bool onlyIdle);

static void scheduleSweep()
{
    if (sweepScheduled)
        return;
    sweepScheduled = true;
    QTimer::singleShot(idleCloseMs / 4, Core::ICore::instance(), [] {
        sweepScheduled = false;
        closeOpenedDocuments(/*onlyIdle=*/true);
    });
}

static bool isVisible(TextDocument *document)
{
    const QList<Core::IEditor *> editors = Core::EditorManager::visibleEditors();
    return Utils::anyOf(editors, [document](Core::IEditor *editor) {
        return editor->document() == document;
    });
}

static void closeOpenedDocuments(bool onlyIdle)
{
    QList<Core::IDocument *> toClose;
    QList<OpenedDocument> kept;
    for (const OpenedDocument &opened : std::as_const(openedDocuments())) {
        if (!opened.document)
            continue; // Closed by somebody else meanwhile.
        if (onlyIdle && !opened.idle.hasExpired()) {
            kept.append(opened);
            continue;
        }
        if (opened.document->isModified() || isVisible(opened.document))
            continue;
        toClose.append(opened.document);
    }
    openedDocuments() = kept;
    if (!toClose.isEmpty())
        Core::EditorManager::closeDocuments(toClose, /*askAboutModifiedEditors=*/false);
    if (!kept.isEmpty())
        scheduleSweep();
}

static void trackOpenedDocument(TextDocument *document)
{
    for (OpenedDocument &opened : openedDocuments()) {
        if (opened.document == document) {
            opened.idle.setRemainingTime(idleCloseMs);
            return;
        }
    }
    openedDocuments().append({document, QDeadlineTimer(idleCloseMs)});
    scheduleSweep();
}

void closeDocumentsOpenedByMcpTools()
{
    closeOpenedDocuments(/*onlyIdle=*/false);
}

// ------------------------------------------------------- client resolution

struct Resolved
{
    QPointer<TextDocument> document;
    QPointer<Client> client; // A server can go away while a question is in flight.
};
using ResolveHandler = std::function<void(const Result<Resolved> &)>;

// Calls back once the client has finished its handshake with the server, or
// with false when it goes away or does not get there in time.
static void whenReachable(Client *client, const std::function<void(bool)> &then)
{
    auto guard = new QObject;
    auto done = std::make_shared<bool>(false);
    const auto finish = [guard, done, then](bool reachable) {
        if (*done)
            return;
        *done = true;
        guard->deleteLater();
        then(reachable);
    };
    QObject::connect(client, &Client::initialized, guard, [finish] { finish(true); });
    QObject::connect(LanguageClientManager::instance(), &LanguageClientManager::clientRemoved,
                     guard, [finish, client](Client *removed) {
        if (removed == client)
            finish(false);
    });
    QTimer::singleShot(serverStartTimeoutMs, guard, [finish] { finish(false); });
}

// Finds the document and the language client for a file, opening the file in a
// hidden editor when it is not open, and waits for a client that is still
// starting.
static void resolveClient(const FilePath &filePath, const ResolveHandler &handler)
{
    TextDocument *document = TextDocument::textDocumentForFilePath(filePath);
    bool openedHere = false;
    if (!document) {
        if (!filePath.isReadableFile()) {
            handler(ResultError(QString("\"%1\" does not exist or is not readable.")
                                    .arg(filePath.toUserOutput())));
            return;
        }
        Core::IEditor *editor = Core::EditorManager::openEditor(
            filePath, {},
            Core::EditorManager::DoNotChangeCurrentEditor | Core::EditorManager::DoNotMakeVisible);
        document = editor ? qobject_cast<TextDocument *>(editor->document()) : nullptr;
        if (!document) {
            handler(ResultError(QString("Could not open \"%1\" in a text editor.")
                                    .arg(filePath.toUserOutput())));
            return;
        }
        openedHere = true;
    }

    Client *client = LanguageClientManager::clientForDocument(document);
    if (!client) {
        const QList<Client *> candidates
            = LanguageClientManager::clientsSupportingDocument(document, /*onlyReachable=*/false);
        if (!candidates.isEmpty()) {
            client = candidates.first();
            LanguageClientManager::openDocumentWithClient(document, client);
        }
    }
    if (!client) {
        if (openedHere)
            Core::EditorManager::closeDocuments({document}, /*askAboutModifiedEditors=*/false);
        handler(ResultError(QString("No language server is configured for \"%1\". If its "
                                    "project was just opened, the server may still be "
                                    "starting; retry in a moment.")
                                .arg(filePath.toUserOutput())));
        return;
    }
    if (openedHere)
        trackOpenedDocument(document);

    if (client->reachable()) {
        handler(Resolved{document, client});
        return;
    }
    const QPointer<TextDocument> documentGuard(document);
    const QPointer<Client> clientGuard(client);
    whenReachable(client, [handler, documentGuard, clientGuard, filePath](bool reachable) {
        if (!reachable || !documentGuard || !clientGuard) {
            handler(ResultError(QString("The language server for \"%1\" is not ready. Try "
                                        "again in a moment.")
                                    .arg(filePath.toUserOutput())));
            return;
        }
        handler(Resolved{documentGuard, clientGuard});
    });
}

// ----------------------------------------------- positions, results, errors

static Result<Position> positionFor(TextDocument *document, int line, int column)
{
    QTextDocument *doc = document->document();
    const QTextBlock block = doc->findBlockByNumber(line - 1);
    if (!block.isValid()) {
        return ResultError(QString("Line %1 is out of range in \"%2\".")
                               .arg(line).arg(document->filePath().toUserOutput()));
    }
    QTextCursor cursor(doc);
    cursor.setPosition(block.position() + qBound(0, column - 1, qMax(0, block.length() - 1)));
    return Position(cursor);
}

static TextDocumentPositionParams positionParams(const Resolved &resolved, const Position &position)
{
    return TextDocumentPositionParams(
        TextDocumentIdentifier(resolved.client->hostPathToServerUri(resolved.document->filePath())),
        position);
}

// A range as 1-based lines and columns. Columns count UTF-16 units, as the
// protocol does and as the editor shows them.
static QJsonObject rangeJson(const Range &range)
{
    return QJsonObject{{"line", range.start().line() + 1},
                       {"column", range.start().character() + 1},
                       {"end_line", range.end().line() + 1},
                       {"end_column", range.end().character() + 1}};
}

static bool provides(const std::optional<std::variant<bool, WorkDoneProgressOptions>> &provider)
{
    if (!provider)
        return false;
    if (const bool *enabled = std::get_if<bool>(&*provider))
        return *enabled;
    return true;
}

static QString unsupported(const Resolved &resolved, const QString &what)
{
    return QString("The language server for \"%1\" (%2) does not support %3.")
        .arg(resolved.document->filePath().toUserOutput(), resolved.client->name(), what);
}

struct ResponseFailure
{
    int code = 0;
    QString message;

    QString text() const { return QString("%1 (error %2)").arg(message).arg(code); }
};

template<typename Response>
static std::optional<ResponseFailure> failureOf(const Response &response)
{
    if (const auto error = response.error())
        return ResponseFailure{error->code(), error->message()};
    return std::nullopt;
}

constexpr int methodNotFoundCode = -32601;

static QString symbolKindName(SymbolKind kind)
{
    switch (kind) {
    case SymbolKind::File: return QStringLiteral("file");
    case SymbolKind::Module: return QStringLiteral("module");
    case SymbolKind::Namespace: return QStringLiteral("namespace");
    case SymbolKind::Package: return QStringLiteral("package");
    case SymbolKind::Class: return QStringLiteral("class");
    case SymbolKind::Method: return QStringLiteral("method");
    case SymbolKind::Property: return QStringLiteral("property");
    case SymbolKind::Field: return QStringLiteral("field");
    case SymbolKind::Constructor: return QStringLiteral("constructor");
    case SymbolKind::Enum: return QStringLiteral("enum");
    case SymbolKind::Interface: return QStringLiteral("interface");
    case SymbolKind::Function: return QStringLiteral("function");
    case SymbolKind::Variable: return QStringLiteral("variable");
    case SymbolKind::Constant: return QStringLiteral("constant");
    case SymbolKind::String: return QStringLiteral("string");
    case SymbolKind::Number: return QStringLiteral("number");
    case SymbolKind::Boolean: return QStringLiteral("boolean");
    case SymbolKind::Array: return QStringLiteral("array");
    case SymbolKind::Object: return QStringLiteral("object");
    case SymbolKind::Key: return QStringLiteral("key");
    case SymbolKind::Null: return QStringLiteral("null");
    case SymbolKind::EnumMember: return QStringLiteral("enum_member");
    case SymbolKind::Struct: return QStringLiteral("struct");
    case SymbolKind::Event: return QStringLiteral("event");
    case SymbolKind::Operator: return QStringLiteral("operator");
    case SymbolKind::TypeParameter: return QStringLiteral("type_parameter");
    }
    return QStringLiteral("symbol");
}

// A call or type hierarchy item: what it is and where its name is.
template<typename Item>
static QJsonObject hierarchyItemJson(const Client *client, const Item &item)
{
    const Range selection = item.selectionRange();
    QJsonObject json{{"name", item.name()},
                     {"kind", symbolKindName(item.symbolKind())},
                     {"file", client->serverUriToHostPath(item.uri()).toUserOutput()},
                     {"line", selection.start().line() + 1},
                     {"column", selection.start().character() + 1}};
    if (const std::optional<QString> detail = item.detail(); detail && !detail->isEmpty())
        json.insert("detail", *detail);
    return json;
}

// ------------------------------------------------------------------- hover

static QString markedStringText(const MarkedString &marked)
{
    if (const QString *plain = std::get_if<QString>(&marked))
        return *plain;
    if (const MarkedLanguageString *code = std::get_if<MarkedLanguageString>(&marked))
        return "```" + code->language() + '\n' + code->value() + "\n```";
    return {};
}

void lspHover(const QJsonObject &args, const ToolResultHandler &handler)
{
    const Result<LocationArgs> location = parseLocationArgs(args);
    if (!location) {
        handler(ResultError(location.error()));
        return;
    }
    resolveClient(location->file, [location = *location, handler](const Result<Resolved> &resolved) {
        if (!resolved) {
            handler(ResultError(resolved.error()));
            return;
        }
        if (!provides(resolved->client->capabilities().hoverProvider())) {
            handler(ResultError(unsupported(*resolved, "hover")));
            return;
        }
        const Result<Position> position = positionFor(resolved->document, location.line,
                                                      location.column);
        if (!position) {
            handler(ResultError(position.error()));
            return;
        }

        HoverRequest request(positionParams(*resolved, *position));
        request.setResponseCallback([handler](const HoverRequest::Response &response) {
            if (const std::optional<ResponseFailure> failure = failureOf(response)) {
                handler(ResultError(failure->text()));
                return;
            }
            const std::optional<HoverResult> hoverResult = response.result();
            const Hover *hover = hoverResult ? std::get_if<Hover>(&*hoverResult) : nullptr;
            if (!hover) {
                handler(QJsonObject{{"contents", QString()},
                                    {"format", QStringLiteral("plaintext")},
                                    {"note", "The server has no information about this "
                                             "position."}});
                return;
            }
            QString text;
            QString format = QStringLiteral("plaintext");
            const HoverContent content = hover->content();
            if (const MarkupContent *markup = std::get_if<MarkupContent>(&content)) {
                text = markup->content();
                if (markup->kind() == MarkupKind::markdown)
                    format = QStringLiteral("markdown");
            } else if (const MarkedString *marked = std::get_if<MarkedString>(&content)) {
                text = markedStringText(*marked);
                format = QStringLiteral("markdown");
            } else if (const auto *list = std::get_if<QList<MarkedString>>(&content)) {
                QStringList parts;
                for (const MarkedString &marked : *list)
                    parts.append(markedStringText(marked));
                text = parts.join("\n\n");
                format = QStringLiteral("markdown");
            }
            QJsonObject result{{"contents", text}, {"format", format}};
            if (const std::optional<Range> range = hover->range())
                result.insert("range", rangeJson(*range));
            handler(result);
        });
        resolved->client->sendMessage(request);
    });
}

// ---------------------------------------------------------- call hierarchy

// Fetches the callers or callees of a function level by level, up to a depth
// and a total count, and reports the tree once every request has answered.
class CallHierarchyWalk : public std::enable_shared_from_this<CallHierarchyWalk>
{
public:
    CallHierarchyWalk(const Resolved &resolved, bool incoming, int maxDepth, int limit,
                      const ToolResultHandler &handler)
        : m_resolved(resolved), m_incoming(incoming), m_maxDepth(maxDepth), m_limit(limit),
          m_handler(handler)
    {}

    void start(const CallHierarchyItem &root)
    {
        m_root = std::make_shared<Node>();
        m_root->json = hierarchyItemJson(m_resolved.client, root);
        fetch(m_root, root, 0);
    }

private:
    struct Node
    {
        QJsonObject json;
        QList<std::shared_ptr<Node>> children;
    };
    using Calls = QList<QPair<CallHierarchyItem, QList<Range>>>;

    void fetch(const std::shared_ptr<Node> &node, const CallHierarchyItem &item, int level)
    {
        if (m_incoming) {
            send<CallHierarchyIncomingCallsRequest, CallHierarchyIncomingCall>(
                node, item, level, &CallHierarchyIncomingCall::from);
        } else {
            send<CallHierarchyOutgoingCallsRequest, CallHierarchyOutgoingCall>(
                node, item, level, &CallHierarchyOutgoingCall::to);
        }
    }

    template<typename Request, typename Call>
    void send(const std::shared_ptr<Node> &node, const CallHierarchyItem &item, int level,
              CallHierarchyItem (Call::*otherEnd)() const)
    {
        if (!m_resolved.client) {
            m_error = QString("The language server went away.");
            if (m_pending == 0)
                finish();
            return;
        }
        ++m_pending;
        CallHierarchyCallsParams params;
        params.setItem(item);
        Request request(params);
        auto self = shared_from_this();
        request.setResponseCallback(
            [self, node, level, otherEnd](const typename Request::Response &response) {
                Calls calls;
                const std::optional<ResponseFailure> failure = failureOf(response);
                if (!failure && response.result()) {
                    for (const Call &call : response.result()->toListOrEmpty())
                        calls.append({(call.*otherEnd)(), call.fromRanges()});
                }
                self->handleCalls(node, level, failure, calls);
            });
        m_resolved.client->sendMessage(request);
    }

    void handleCalls(const std::shared_ptr<Node> &node, int level,
                     const std::optional<ResponseFailure> &failure, const Calls &calls)
    {
        --m_pending;
        if (failure && m_error.isEmpty()) {
            // The capability does not say which directions a server implements;
            // clangd before 20.1 answers outgoing calls with "method not found".
            if (failure->code == methodNotFoundCode && !m_incoming) {
                m_error = QString("The language server does not implement outgoing calls "
                                  "(clangd needs version 20.1 or newer).");
            } else {
                m_error = failure->text();
            }
        }
        if (m_error.isEmpty()) {
            for (const auto &[item, ranges] : calls) {
                ++m_total;
                if (m_reported >= m_limit) {
                    m_truncated = true;
                    continue;
                }
                ++m_reported;
                auto child = std::make_shared<Node>();
                child->json = hierarchyItemJson(m_resolved.client, item);
                QJsonArray fromRanges;
                for (const Range &range : ranges)
                    fromRanges.append(rangeJson(range));
                child->json.insert("from_ranges", fromRanges);
                node->children.append(child);
                if (level + 1 < m_maxDepth)
                    fetch(child, item, level + 1);
            }
        }
        if (m_pending == 0)
            finish();
    }

    static QJsonArray childrenJson(const std::shared_ptr<Node> &node)
    {
        QJsonArray array;
        for (const std::shared_ptr<Node> &child : node->children) {
            QJsonObject json = child->json;
            if (!child->children.isEmpty())
                json.insert("calls", childrenJson(child));
            array.append(json);
        }
        return array;
    }

    void finish()
    {
        if (!m_error.isEmpty()) {
            m_handler(ResultError(m_error));
            return;
        }
        QJsonObject result{{"root", m_root->json},
                           {"direction", m_incoming ? QStringLiteral("incoming")
                                                    : QStringLiteral("outgoing")},
                           {"calls", childrenJson(m_root)},
                           {"total", m_total},
                           {"truncated", m_truncated}};
        if (m_total == 0) {
            result.insert("note", m_incoming
                ? QString("No callers found. The server's index may not cover every file yet.")
                : QString("No calls found. The function's body may not be available to the "
                          "server, or it may not have indexed it yet."));
        }
        m_handler(result);
    }

    const Resolved m_resolved;
    const bool m_incoming;
    const int m_maxDepth;
    const int m_limit;
    const ToolResultHandler m_handler;
    std::shared_ptr<Node> m_root;
    int m_pending = 0;
    int m_total = 0;
    int m_reported = 0;
    bool m_truncated = false;
    QString m_error;
};

void lspCallHierarchy(const QJsonObject &args, const ToolResultHandler &handler)
{
    const Result<LocationArgs> location = parseLocationArgs(args);
    if (!location) {
        handler(ResultError(location.error()));
        return;
    }
    const QString direction = args.value("direction").toString(QStringLiteral("incoming"));
    if (direction != QLatin1String("incoming") && direction != QLatin1String("outgoing")) {
        handler(ResultError(QString("\"direction\" must be \"incoming\" or \"outgoing\", "
                                    "not \"%1\".").arg(direction)));
        return;
    }
    const bool incoming = direction == QLatin1String("incoming");
    const int depth = std::clamp(args.value("depth").toInt(1), 1, 3);
    const int limit = resultLimit(args);

    resolveClient(location->file, [location = *location, incoming, depth, limit, handler](
                                      const Result<Resolved> &resolved) {
        if (!resolved) {
            handler(ResultError(resolved.error()));
            return;
        }
        if (!provides(resolved->client->capabilities().callHierarchyProvider())) {
            handler(ResultError(unsupported(*resolved, "call hierarchy")));
            return;
        }
        const Result<Position> position = positionFor(resolved->document, location.line,
                                                      location.column);
        if (!position) {
            handler(ResultError(position.error()));
            return;
        }

        PrepareCallHierarchyRequest request(positionParams(*resolved, *position));
        request.setResponseCallback([resolved = *resolved, location, incoming, depth, limit,
                                     handler](const PrepareCallHierarchyRequest::Response &response) {
            if (const std::optional<ResponseFailure> failure = failureOf(response)) {
                handler(ResultError(failure->text()));
                return;
            }
            const QList<CallHierarchyItem> items
                = response.result() ? response.result()->toListOrEmpty()
                                    : QList<CallHierarchyItem>();
            if (items.isEmpty() || !resolved.client) {
                handler(ResultError(QString("Nothing at %1:%2:%3 to build a call hierarchy "
                                            "for. Point at a function name.")
                                        .arg(location.file.toUserOutput())
                                        .arg(location.line).arg(location.column)));
                return;
            }
            auto walk = std::make_shared<CallHierarchyWalk>(resolved, incoming, depth, limit,
                                                            handler);
            walk->start(items.first());
        });
        resolved->client->sendMessage(request);
    });
}

// ------------------------------------------------------------ registration

using ToolFunction = void (*)(const QJsonObject &, const ToolResultHandler &);

// Registers an asynchronous tool whose body is one of the functions above.
static void registerAsyncTool(const Mcp::Schema::Tool &tool, ToolFunction function)
{
    Mcp::ToolRegistry::registerTool(
        tool,
        [function](const Mcp::Schema::CallToolRequestParams &params,
                   const Mcp::ToolInterface &toolInterface) -> Result<> {
            // The interface is copied into the handler on purpose: returning
            // from this callback would otherwise finalize the call.
            function(params.argumentsAsObject(), [toolInterface](const Result<QJsonObject> &result) {
                if (result) {
                    toolInterface.finish(
                        Mcp::Schema::CallToolResult{}.isError(false).structuredContent(*result));
                } else {
                    toolInterface.finish(Mcp::Schema::CallToolResult{}.isError(true).addContent(
                        Mcp::Schema::TextContent{}.text(result.error())));
                }
            });
            return ResultOk;
        });
}

// The file, line and column every position-based tool takes.
static Mcp::Schema::Tool::InputSchema positionInputSchema(const QString &subject)
{
    return Mcp::Schema::Tool::InputSchema{}
        .addProperty("file",
                     QJsonObject{{"type", "string"},
                                 {"description", "Absolute path to the source file."}})
        .addProperty("line",
                     QJsonObject{{"type", "integer"},
                                 {"description", QString("1-based line of %1.").arg(subject)}})
        .addProperty("column",
                     QJsonObject{{"type", "integer"},
                                 {"description", QString("1-based column of %1.").arg(subject)}})
        .addRequired("file")
        .addRequired("line")
        .addRequired("column");
}

void registerMcpTools()
{
    using namespace Mcp::Schema;

    registerAsyncTool(
        Tool{}
            .name("lsp_hover")
            .title("Get documentation via the language server")
            .description(
                "Returns what the language server shows on hover at a position: the "
                "declaration, its type, and its documentation comment, as markdown or plain "
                "text, plus the range the information applies to. Answers come from clangd "
                "for C++, qmlls for QML, or whatever server is configured for the file "
                "type, so this is the tool for a symbol's documentation. Give the file and a "
                "1-based line and column on an identifier. The file is opened in a hidden "
                "editor if it is not open; a server must be configured for its file type, "
                "and a server that is still starting asks to be retried.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(positionInputSchema("the identifier"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("contents", QJsonObject{{"type", "string"}})
                    .addProperty("format",
                                 QJsonObject{{"type", "string"},
                                             {"enum", QJsonArray{"markdown", "plaintext"}}})
                    .addProperty("range",
                                 QJsonObject{{"type", "object"},
                                             {"description",
                                              "The 1-based line/column to end_line/end_column "
                                              "the information applies to."}})
                    .addProperty("note", QJsonObject{{"type", "string"}})
                    .addRequired("contents")
                    .addRequired("format")),
        &lspHover);

    registerAsyncTool(
        Tool{}
            .name("lsp_call_hierarchy")
            .title("Get the call hierarchy via the language server")
            .description(
                "Returns the callers (\"incoming\") or the callees (\"outgoing\") of the "
                "function at a position, as the language server (clangd for C++, or whatever "
                "server is configured for the file type) computes them from its index. Give "
                "the file and a 1-based line and column on a function name. Each entry has "
                "the function's name, kind, file and position, the \"from_ranges\" where "
                "the calls happen, and, with a \"depth\" above 1, its own \"calls\" nested "
                "below it. clangd supports outgoing calls from version 20.1 on and reports "
                "an error before that. The file is opened in a hidden editor if it is not "
                "open; a server that is still starting asks to be retried.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                positionInputSchema("the function name")
                    .addProperty(
                        "direction",
                        QJsonObject{{"type", "string"},
                                    {"enum", QJsonArray{"incoming", "outgoing"}},
                                    {"description",
                                     "\"incoming\" for the callers (default), \"outgoing\" "
                                     "for the callees."}})
                    .addProperty(
                        "depth",
                        QJsonObject{{"type", "integer"},
                                    {"description",
                                     "How many levels to follow, 1 to 3 (default 1)."}})
                    .addProperty("limit", limitProperty()))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("root", QJsonObject{{"type", "object"}})
                    .addProperty("direction", QJsonObject{{"type", "string"}})
                    .addProperty(
                        "calls",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}},
                                    {"description",
                                     "The callers or callees, each possibly with its own "
                                     "\"calls\"."}})
                    .addProperty("total", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addProperty("note", QJsonObject{{"type", "string"}})
                    .addRequired("root")
                    .addRequired("calls")),
        &lspCallHierarchy);
}

} // namespace LanguageClient
