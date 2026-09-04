// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcptools.h"

#include "client.h"
#include "languageclientmanager.h"
#include "languageclientutils.h"

#include <mcp/server/toolregistry.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <languageserverprotocol/callhierarchy.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/lsptypes.h>
#include <languageserverprotocol/servercapabilities.h>
#include <languageserverprotocol/typehierarchy.h>
#include <languageserverprotocol/workspace.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QDeadlineTimer>
#include <QHash>
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

// The host path a server URI stands for, spelled canonically: a server may
// report the same file both through a symlink and directly.
static QString hostPathString(const Client *client, const DocumentUri &uri)
{
    const FilePath path = client->serverUriToHostPath(uri);
    const FilePath canonical = path.canonicalPath();
    return (canonical.isEmpty() ? path : canonical).toUserOutput();
}

static QJsonObject locationJson(const Client *client, const Location &location)
{
    QJsonObject json = rangeJson(location.range());
    json.insert("file", hostPathString(client, location.uri()));
    return json;
}

// Sorts locations by file, line and column, so that an answer does not depend
// on the order a server happens to produce, and drops the duplicates a server
// may report for one place.
static void sortAndDedupeLocations(QList<QJsonObject> &locations)
{
    std::sort(locations.begin(), locations.end(),
              [](const QJsonObject &a, const QJsonObject &b) {
                  const int file = a.value("file").toString().compare(b.value("file").toString());
                  if (file != 0)
                      return file < 0;
                  if (a.value("line").toInt() != b.value("line").toInt())
                      return a.value("line").toInt() < b.value("line").toInt();
                  return a.value("column").toInt() < b.value("column").toInt();
              });
    const auto samePlace = [](const QJsonObject &a, const QJsonObject &b) {
        for (const char *key : {"file", "line", "column", "end_line", "end_column"}) {
            if (a.value(QLatin1String(key)) != b.value(QLatin1String(key)))
                return false;
        }
        return true;
    };
    locations.erase(std::unique(locations.begin(), locations.end(), samePlace), locations.end());
}

// Caps a list to the limit, reporting the size before the cap and whether it
// was applied, so a query on a large project cannot return an unbounded answer.
static QJsonArray cappedArray(const QList<QJsonObject> &objects, int limit, int *total,
                              bool *truncated)
{
    *total = int(objects.size());
    *truncated = *total > limit;
    QJsonArray array;
    for (int i = 0; i < objects.size() && i < limit; ++i)
        array.append(objects.at(i));
    return array;
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
                     {"file", hostPathString(client, item.uri())},
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

// ------------------------------------------------------------ hierarchies

// A walk fetches a hierarchy level by level, up to a depth and a total count,
// and reports the tree once every request has answered. The fetcher says how
// one level is asked for: the request to send for an item, and how to read the
// items, with any extra data about them, out of the answer.
template<typename Fetcher>
class HierarchyWalk : public std::enable_shared_from_this<HierarchyWalk<Fetcher>>
{
public:
    using Item = typename Fetcher::Item;
    using Children = QList<QPair<Item, QJsonObject>>; // Each item with extra data for its node.

    HierarchyWalk(const Resolved &resolved, const Fetcher &fetcher, int maxDepth, int limit,
                  const ToolResultHandler &handler)
        : m_resolved(resolved), m_fetcher(fetcher), m_maxDepth(maxDepth), m_limit(limit),
          m_handler(handler)
    {}

    void start(const Item &root)
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

    void fetch(const std::shared_ptr<Node> &node, const Item &item, int level)
    {
        if (!m_resolved.client) {
            m_error = QString("The language server went away.");
            if (m_pending == 0)
                finish();
            return;
        }
        ++m_pending;
        auto self = this->shared_from_this();
        m_fetcher.send(m_resolved.client, item,
                       [self, node, level](const std::optional<ResponseFailure> &failure,
                                           const Children &children) {
            self->handleChildren(node, level, failure, children);
        });
    }

    void handleChildren(const std::shared_ptr<Node> &node, int level,
                        const std::optional<ResponseFailure> &failure, const Children &children)
    {
        --m_pending;
        if (failure && m_error.isEmpty())
            m_error = m_fetcher.describe(*failure);
        if (m_error.isEmpty()) {
            for (const auto &[item, extra] : children) {
                ++m_total;
                if (m_reported >= m_limit) {
                    m_truncated = true;
                    continue;
                }
                ++m_reported;
                auto child = std::make_shared<Node>();
                child->json = hierarchyItemJson(m_resolved.client, item);
                for (auto it = extra.begin(); it != extra.end(); ++it)
                    child->json.insert(it.key(), it.value());
                node->children.append(child);
                if (level + 1 < m_maxDepth)
                    fetch(child, item, level + 1);
            }
        }
        if (m_pending == 0)
            finish();
    }

    QJsonArray childrenJson(const std::shared_ptr<Node> &node) const
    {
        QJsonArray array;
        for (const std::shared_ptr<Node> &child : node->children) {
            QJsonObject json = child->json;
            if (!child->children.isEmpty())
                json.insert(m_fetcher.childrenKey(), childrenJson(child));
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
                           {m_fetcher.childrenKey(), childrenJson(m_root)},
                           {"total", m_total},
                           {"truncated", m_truncated}};
        m_fetcher.decorate(result, m_total);
        m_handler(result);
    }

    const Resolved m_resolved;
    const Fetcher m_fetcher;
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

// The callers or the callees of a call hierarchy item, each with the ranges
// of the calls.
class CallsFetcher
{
public:
    using Item = CallHierarchyItem;

    explicit CallsFetcher(bool incoming) : m_incoming(incoming) {}

    QString childrenKey() const { return QStringLiteral("calls"); }

    template<typename Handler>
    void send(Client *client, const Item &item, const Handler &handler) const
    {
        CallHierarchyCallsParams params;
        params.setItem(item);
        if (m_incoming) {
            sendRequest<CallHierarchyIncomingCallsRequest, CallHierarchyIncomingCall>(
                client, params, &CallHierarchyIncomingCall::from, handler);
        } else {
            sendRequest<CallHierarchyOutgoingCallsRequest, CallHierarchyOutgoingCall>(
                client, params, &CallHierarchyOutgoingCall::to, handler);
        }
    }

    QString describe(const ResponseFailure &failure) const
    {
        // The capability does not say which directions a server implements;
        // clangd before 20.1 answers outgoing calls with "method not found".
        if (failure.code == methodNotFoundCode && !m_incoming) {
            return QString("The language server does not implement outgoing calls (clangd "
                           "needs version 20.1 or newer).");
        }
        return failure.text();
    }

    void decorate(QJsonObject &result, int total) const
    {
        result.insert("direction", m_incoming ? QStringLiteral("incoming")
                                              : QStringLiteral("outgoing"));
        if (total == 0) {
            result.insert("note", m_incoming
                ? QString("No callers found. The server's index may not cover every file yet.")
                : QString("No calls found. The function's body may not be available to the "
                          "server, or it may not have indexed it yet."));
        }
    }

private:
    template<typename Request, typename Call, typename Handler>
    static void sendRequest(Client *client, const CallHierarchyCallsParams &params,
                            CallHierarchyItem (Call::*otherEnd)() const, const Handler &handler)
    {
        Request request(params);
        request.setResponseCallback([handler, otherEnd](const typename Request::Response &response) {
            QList<QPair<CallHierarchyItem, QJsonObject>> children;
            const std::optional<ResponseFailure> failure = failureOf(response);
            if (!failure && response.result()) {
                for (const Call &call : response.result()->toListOrEmpty()) {
                    QJsonArray fromRanges;
                    for (const Range &range : call.fromRanges())
                        fromRanges.append(rangeJson(range));
                    children.append({(call.*otherEnd)(), QJsonObject{{"from_ranges", fromRanges}}});
                }
            }
            handler(failure, children);
        });
        client->sendMessage(request);
    }

    const bool m_incoming;
};

// The supertypes or the subtypes of a type hierarchy item.
class TypesFetcher
{
public:
    using Item = TypeHierarchyItem;

    explicit TypesFetcher(bool supertypes) : m_supertypes(supertypes) {}

    QString childrenKey() const
    {
        return m_supertypes ? QStringLiteral("supertypes") : QStringLiteral("subtypes");
    }

    template<typename Handler>
    void send(Client *client, const Item &item, const Handler &handler) const
    {
        TypeHierarchyParams params;
        params.setItem(item);
        if (m_supertypes)
            sendRequest<TypeHierarchySupertypesRequest>(client, params, handler);
        else
            sendRequest<TypeHierarchySubtypesRequest>(client, params, handler);
    }

    QString describe(const ResponseFailure &failure) const { return failure.text(); }

    void decorate(QJsonObject &result, int total) const
    {
        result.insert("direction", childrenKey());
        if (total == 0) {
            result.insert("note", m_supertypes
                ? QString("No supertypes found.")
                : QString("No subtypes found. The server's index may not cover every file yet."));
        }
    }

private:
    template<typename Request, typename Handler>
    static void sendRequest(Client *client, const TypeHierarchyParams &params,
                            const Handler &handler)
    {
        Request request(params);
        request.setResponseCallback([handler](const typename Request::Response &response) {
            QList<QPair<TypeHierarchyItem, QJsonObject>> children;
            const std::optional<ResponseFailure> failure = failureOf(response);
            if (!failure && response.result()) {
                for (const TypeHierarchyItem &item : response.result()->toListOrEmpty())
                    children.append({item, QJsonObject()});
            }
            handler(failure, children);
        });
        client->sendMessage(request);
    }

    const bool m_supertypes;
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
            auto walk = std::make_shared<HierarchyWalk<CallsFetcher>>(
                resolved, CallsFetcher(incoming), depth, limit, handler);
            walk->start(items.first());
        });
        resolved->client->sendMessage(request);
    });
}

void lspTypeHierarchy(const QJsonObject &args, const ToolResultHandler &handler)
{
    const Result<LocationArgs> location = parseLocationArgs(args);
    if (!location) {
        handler(ResultError(location.error()));
        return;
    }
    const QString direction = args.value("direction").toString(QStringLiteral("both"));
    const QStringList directions{"supertypes", "subtypes", "both"};
    if (!directions.contains(direction)) {
        handler(ResultError(QString("\"direction\" must be \"supertypes\", \"subtypes\" or "
                                    "\"both\", not \"%1\".").arg(direction)));
        return;
    }
    const int depth = std::clamp(args.value("depth").toInt(1), 1, 3);
    const int limit = resultLimit(args);

    resolveClient(location->file, [location = *location, direction, depth, limit, handler](
                                      const Result<Resolved> &resolved) {
        if (!resolved) {
            handler(ResultError(resolved.error()));
            return;
        }
        if (!provides(resolved->client->capabilities().typeHierarchyProvider())) {
            handler(ResultError(unsupported(*resolved, "type hierarchy")));
            return;
        }
        const Result<Position> position = positionFor(resolved->document, location.line,
                                                      location.column);
        if (!position) {
            handler(ResultError(position.error()));
            return;
        }

        PrepareTypeHierarchyRequest request(positionParams(*resolved, *position));
        request.setResponseCallback([resolved = *resolved, location, direction, depth, limit,
                                     handler](const PrepareTypeHierarchyRequest::Response &response) {
            if (const std::optional<ResponseFailure> failure = failureOf(response)) {
                handler(ResultError(failure->text()));
                return;
            }
            const QList<TypeHierarchyItem> items
                = response.result() ? response.result()->toListOrEmpty()
                                    : QList<TypeHierarchyItem>();
            if (items.isEmpty() || !resolved.client) {
                handler(ResultError(QString("Nothing at %1:%2:%3 to build a type hierarchy "
                                            "for. Point at a class name.")
                                        .arg(location.file.toUserOutput())
                                        .arg(location.line).arg(location.column)));
                return;
            }
            const TypeHierarchyItem root = items.first();
            if (direction != QLatin1String("both")) {
                auto walk = std::make_shared<HierarchyWalk<TypesFetcher>>(
                    resolved, TypesFetcher(direction == QLatin1String("supertypes")), depth,
                    limit, handler);
                walk->start(root);
                return;
            }
            // Both directions: one walk up, then one down, merged into one answer.
            auto up = std::make_shared<HierarchyWalk<TypesFetcher>>(
                resolved, TypesFetcher(true), depth, limit,
                [resolved, root, depth, limit, handler](const Result<QJsonObject> &upResult) {
                    if (!upResult) {
                        handler(upResult);
                        return;
                    }
                    auto down = std::make_shared<HierarchyWalk<TypesFetcher>>(
                        resolved, TypesFetcher(false), depth, limit,
                        [upResult = *upResult, handler](const Result<QJsonObject> &downResult) {
                            if (!downResult) {
                                handler(downResult);
                                return;
                            }
                            QJsonObject merged = upResult;
                            merged.insert("direction", QStringLiteral("both"));
                            merged.insert("subtypes", downResult->value("subtypes"));
                            const int total = upResult.value("total").toInt()
                                              + downResult->value("total").toInt();
                            merged.insert("total", total);
                            merged.insert("truncated", upResult.value("truncated").toBool()
                                                           || downResult->value("truncated").toBool());
                            merged.remove("note");
                            if (total == 0) {
                                merged.insert("note", "Neither supertypes nor subtypes found. "
                                                      "The server's index may not cover every "
                                                      "file yet.");
                            }
                            handler(merged);
                        });
                    down->start(root);
                });
            up->start(root);
        });
        resolved->client->sendMessage(request);
    });
}

// -------------------------------------------------------------- references

void lspReferences(const QJsonObject &args, const ToolResultHandler &handler)
{
    const Result<LocationArgs> location = parseLocationArgs(args);
    if (!location) {
        handler(ResultError(location.error()));
        return;
    }
    const bool includeDeclaration = args.value("include_declaration").toBool(true);
    const int limit = resultLimit(args);

    resolveClient(location->file, [location = *location, includeDeclaration, limit, handler](
                                      const Result<Resolved> &resolved) {
        if (!resolved) {
            handler(ResultError(resolved.error()));
            return;
        }
        if (!provides(resolved->client->capabilities().referencesProvider())) {
            handler(ResultError(unsupported(*resolved, "finding references")));
            return;
        }
        const Result<Position> position = positionFor(resolved->document, location.line,
                                                      location.column);
        if (!position) {
            handler(ResultError(position.error()));
            return;
        }

        ReferenceParams params(positionParams(*resolved, *position));
        params.setContext(ReferenceParams::ReferenceContext(includeDeclaration));
        FindReferencesRequest request(params);
        request.setResponseCallback([resolved = *resolved, includeDeclaration, limit, handler](
                                        const FindReferencesRequest::Response &response) {
            if (const std::optional<ResponseFailure> failure = failureOf(response)) {
                handler(ResultError(failure->text()));
                return;
            }
            if (!resolved.client) {
                handler(ResultError(QString("The language server went away.")));
                return;
            }
            QList<QJsonObject> references;
            if (response.result()) {
                for (const Location &found : response.result()->toListOrEmpty())
                    references.append(locationJson(resolved.client, found));
            }
            sortAndDedupeLocations(references);
            int total = 0;
            bool truncated = false;
            QJsonObject result{{"references", cappedArray(references, limit, &total, &truncated)},
                               {"include_declaration", includeDeclaration},
                               {"total", total},
                               {"truncated", truncated}};
            if (total == 0) {
                result.insert("note", "No references found. Is the position on an identifier, "
                                      "and has the server indexed the project?");
            }
            handler(result);
        });
        resolved->client->sendMessage(request);
    });
}

// ------------------------------------------------------------------ rename

// The open document for a file, under whatever spelling the server uses for
// it: on macOS the temp directory is a symlink, and clangd reports the
// canonical path while the document keeps the one it was opened with.
static TextDocument *openDocumentFor(const FilePath &path)
{
    if (TextDocument *document = TextDocument::textDocumentForFilePath(path))
        return document;
    const FilePath canonical = path.canonicalPath();
    if (canonical.isEmpty())
        return nullptr;
    const QList<Core::IDocument *> documents = Core::DocumentModel::openedDocuments();
    for (Core::IDocument *document : documents) {
        auto textDocument = qobject_cast<TextDocument *>(document);
        if (textDocument && textDocument->filePath().canonicalPath() == canonical)
            return textDocument;
    }
    return nullptr;
}

// The text of files, read once each: open documents from the editor, the rest
// from disk.
class FileTexts
{
public:
    QStringList lines(const FilePath &path)
    {
        auto it = m_lines.find(path);
        if (it == m_lines.end()) {
            QString text;
            if (const TextDocument *document = openDocumentFor(path))
                text = document->plainText();
            else if (const Result<QByteArray> contents = path.fileContents())
                text = QString::fromUtf8(*contents);
            it = m_lines.insert(path, text.split('\n'));
        }
        return *it;
    }

private:
    QHash<FilePath, QStringList> m_lines;
};

// The text between two (0-based) positions of a file.
static QString textBetween(const QStringList &lines, const Position &start, const Position &end)
{
    if (start.line() == end.line()) {
        return lines.value(start.line())
            .mid(start.character(), end.character() - start.character());
    }
    QString text = lines.value(start.line()).mid(start.character());
    for (int line = start.line() + 1; line < end.line(); ++line)
        text += '\n' + lines.value(line);
    return text + '\n' + lines.value(end.line()).left(end.character());
}

// The text edits of a workspace edit, each with its file, position, old and
// new text, in file and position order. A server may also ask for files to
// be created, renamed or deleted; those are only counted.
struct EditList
{
    QList<QJsonObject> edits;
    QStringList files;
    int otherChanges = 0;
};

static EditList listEdits(const Client *client, const WorkspaceEdit &edit)
{
    EditList list;
    FileTexts texts;
    const auto add = [&](const DocumentUri &uri, const QList<TextEdit> &edits) {
        const QString file = hostPathString(client, uri);
        if (!list.files.contains(file))
            list.files.append(file);
        const QStringList lines = texts.lines(client->serverUriToHostPath(uri));
        for (const TextEdit &textEdit : edits) {
            const Range range = textEdit.range();
            QJsonObject json = rangeJson(range);
            json.insert("file", file);
            json.insert("old_text", textBetween(lines, range.start(), range.end()));
            json.insert("new_text", textEdit.newText());
            const QString lineText = lines.value(range.start().line()).trimmed();
            if (!lineText.isEmpty())
                json.insert("line_text", lineText);
            list.edits.append(json);
        }
    };
    const std::optional<QList<DocumentChange>> documentChanges = edit.documentChanges();
    if (documentChanges && !documentChanges->isEmpty()) {
        for (const DocumentChange &change : *documentChanges) {
            if (const TextDocumentEdit *textDocumentEdit = std::get_if<TextDocumentEdit>(&change))
                add(textDocumentEdit->textDocument().uri(), textDocumentEdit->edits());
            else
                ++list.otherChanges;
        }
    } else if (const std::optional<WorkspaceEdit::Changes> changes = edit.changes()) {
        for (auto it = changes->cbegin(); it != changes->cend(); ++it)
            add(it.key(), it.value());
    }
    sortAndDedupeLocations(list.edits);
    list.files.sort();
    return list;
}

// One rename in progress: prepare, rename, look for clashes, then report or
// apply. Each step is a request; the state lives as long as a step is pending.
class Rename : public std::enable_shared_from_this<Rename>
{
public:
    Rename(const Resolved &resolved, const LocationArgs &location, const Position &position,
           const QString &newName, bool apply, int limit, const ToolResultHandler &handler)
        : m_resolved(resolved), m_location(location), m_position(position), m_newName(newName),
          m_apply(apply), m_limit(limit), m_handler(handler)
    {}

    void start(bool prepareSupported)
    {
        if (prepareSupported)
            prepare();
        else
            rename();
    }

private:
    bool clientGone()
    {
        if (m_resolved.client)
            return false;
        m_handler(ResultError(QString("The language server went away.")));
        return true;
    }

    TextDocumentPositionParams params() const
    {
        return positionParams(m_resolved, m_position);
    }

    // Asks whether the position can be renamed at all, and learns the old name.
    void prepare()
    {
        if (clientGone())
            return;
        PrepareRenameRequest request(params());
        auto self = shared_from_this();
        request.setResponseCallback([self](const PrepareRenameRequest::Response &response) {
            if (const std::optional<ResponseFailure> failure = failureOf(response)) {
                self->m_handler(ResultError(QString("Cannot rename here: %1").arg(failure->text())));
                return;
            }
            const std::optional<PrepareRenameResult> result = response.result();
            if (!result || std::holds_alternative<std::nullptr_t>(*result)) {
                self->m_handler(ResultError(QString("Nothing renamable at %1:%2:%3.")
                                                .arg(self->m_location.file.toUserOutput())
                                                .arg(self->m_location.line)
                                                .arg(self->m_location.column)));
                return;
            }
            if (const PlaceHolderResult *placeHolder = std::get_if<PlaceHolderResult>(&*result))
                self->m_oldName = placeHolder->placeHolder();
            self->rename();
        });
        m_resolved.client->sendMessage(request);
    }

    void rename()
    {
        if (clientGone())
            return;
        RenameParams params;
        params.setTextDocument(this->params().textDocument());
        params.setPosition(m_position);
        params.setNewName(m_newName);
        RenameRequest request(params);
        auto self = shared_from_this();
        request.setResponseCallback([self](const RenameRequest::Response &response) {
            // A clash within the same scope is the server's to detect: clangd
            // refuses it here, with the place of the other declaration.
            if (const std::optional<ResponseFailure> failure = failureOf(response)) {
                self->m_handler(ResultError(QString("The language server refused the rename: %1")
                                                .arg(failure->text())));
                return;
            }
            if (self->clientGone())
                return;
            const std::optional<WorkspaceEdit> edit = response.result();
            if (!edit) {
                self->m_handler(ResultError(QString("The language server returned no edits.")));
                return;
            }
            self->m_edit = *edit;
            self->m_edits = listEdits(self->m_resolved.client, *edit);
            if (self->m_edits.edits.isEmpty() && self->m_edits.otherChanges == 0) {
                self->m_handler(ResultError(QString("The language server returned no edits.")));
                return;
            }
            if (self->m_oldName.isEmpty())
                self->m_oldName = self->m_edits.edits.value(0).value("old_text").toString();
            self->findClashes();
        });
        m_resolved.client->sendMessage(request);
    }

    // Other symbols already called by the new name, anywhere the server knows
    // of. They are in other scopes, or the rename would have been refused, so
    // they are reported for judgment rather than blocking.
    void findClashes()
    {
        if (clientGone())
            return;
        if (!provides(m_resolved.client->capabilities().workspaceSymbolProvider())) {
            m_note = QString("The server cannot search symbols, so other declarations named "
                             "\"%1\" were not looked for.").arg(m_newName);
            finish();
            return;
        }
        WorkspaceSymbolParams params;
        params.setQuery(m_newName);
        params.setLimit(50);
        WorkspaceSymbolRequest request(params);
        auto self = shared_from_this();
        request.setResponseCallback([self](const WorkspaceSymbolRequest::Response &response) {
            if (self->clientGone())
                return;
            if (const std::optional<ResponseFailure> failure = failureOf(response)) {
                self->m_note = QString("Looking for other declarations named \"%1\" failed: %2")
                                   .arg(self->m_newName, failure->text());
            } else if (response.result()) {
                for (const SymbolInformation &symbol : response.result()->toListOrEmpty()) {
                    if (symbol.name() != self->m_newName)
                        continue; // The query matches fuzzily.
                    QJsonObject conflict = locationJson(self->m_resolved.client, symbol.location());
                    conflict.remove("end_line");
                    conflict.remove("end_column");
                    conflict.insert("name", symbol.name());
                    conflict.insert("kind", symbolKindName(SymbolKind(symbol.kind())));
                    if (const std::optional<QString> container = symbol.containerName();
                            container && !container->isEmpty()) {
                        conflict.insert("container", *container);
                    }
                    self->m_conflicts.append(conflict);
                }
            }
            self->finish();
        });
        m_resolved.client->sendMessage(request);
    }

    void finish()
    {
        if (clientGone())
            return;
        int total = 0;
        bool truncated = false;
        QJsonObject result{{"symbol", m_oldName},
                           {"new_name", m_newName},
                           {"applied", false},
                           {"edits", cappedArray(m_edits.edits, m_limit, &total, &truncated)},
                           {"total_edits", total},
                           {"truncated", truncated},
                           {"files_affected", QJsonArray::fromStringList(m_edits.files)},
                           {"has_conflicts", !m_conflicts.isEmpty()},
                           {"conflicts", m_conflicts}};
        if (m_edits.otherChanges > 0)
            result.insert("other_changes", m_edits.otherChanges);
        if (!m_note.isEmpty())
            result.insert("note", m_note);
        if (!m_apply) {
            m_handler(result);
            return;
        }

        // Refuse before touching anything if a target is read-only: applying
        // would otherwise stop at a modal dialog nobody is there to answer.
        QStringList notWritable;
        for (const QString &file : std::as_const(m_edits.files)) {
            if (!FilePath::fromUserInput(file).isWritableFile())
                notWritable.append(file);
        }
        if (!notWritable.isEmpty()) {
            m_handler(ResultError(QString("Cannot apply: not writable: %1. Nothing was changed.")
                                      .arg(notWritable.join(", "))));
            return;
        }

        // The edits go through the refactoring machinery the editor uses: a
        // file shown in a text editor widget is changed in that editor, any
        // other file is written to disk and an open document reloads. A file
        // is addressed by the path its document was opened with, since the
        // server may spell it differently. A document the edit changed in
        // memory is saved afterwards when it had no changes of its own, so the
        // rename reaches disk either way; one with unsaved changes of its own
        // is left as it is, and named.
        // A server may list one file under two spellings (clangd does, on
        // macOS, for a project in the temp directory), so the edits are
        // gathered per canonical file first, without duplicates, or they
        // would be applied twice.
        struct FileEdits
        {
            FilePath path;
            QList<TextEdit> edits;
        };
        QMap<QString, FileEdits> perFile;
        const auto gather = [this, &perFile](const DocumentUri &uri, const QList<TextEdit> &edits) {
            FileEdits &fileEdits = perFile[hostPathString(m_resolved.client, uri)];
            if (fileEdits.path.isEmpty())
                fileEdits.path = m_resolved.client->serverUriToHostPath(uri);
            for (const TextEdit &edit : edits) {
                const bool known = Utils::anyOf(fileEdits.edits, [&edit](const TextEdit &other) {
                    return other.range().start() == edit.range().start()
                           && other.range().end() == edit.range().end()
                           && other.newText() == edit.newText();
                });
                if (!known)
                    fileEdits.edits.append(edit);
            }
        };
        const std::optional<QList<DocumentChange>> documentChanges = m_edit.documentChanges();
        if (documentChanges && !documentChanges->isEmpty()) {
            for (const DocumentChange &change : *documentChanges) {
                if (const TextDocumentEdit *textEdit = std::get_if<TextDocumentEdit>(&change))
                    gather(textEdit->textDocument().uri(), textEdit->edits());
                else
                    applyDocumentChange(m_resolved.client, change);
            }
        } else if (const std::optional<WorkspaceEdit::Changes> changes = m_edit.changes()) {
            for (auto it = changes->cbegin(); it != changes->cend(); ++it)
                gather(it.key(), it.value());
        }

        QList<Core::IDocument *> toSave;
        QStringList unsaved;
        for (auto it = perFile.cbegin(); it != perFile.cend(); ++it) {
            TextDocument *document = openDocumentFor(it->path);
            if (!document) {
                applyTextEdits(m_resolved.client, it->path, it->edits);
                continue;
            }
            const bool wasModified = document->isModified();
            applyTextEdits(m_resolved.client, document->filePath(), it->edits);
            if (wasModified)
                unsaved.append(it.key());
            else if (document->isModified())
                toSave.append(document);
        }
        for (Core::IDocument *document : std::as_const(toSave)) {
            if (!Core::DocumentManager::saveDocument(document))
                unsaved.append(document->filePath().toUserOutput());
        }
        result.insert("applied", true);
        result.insert("files_changed", QJsonArray::fromStringList(m_edits.files));
        result.insert("unsaved_files", QJsonArray::fromStringList(unsaved));
        m_handler(result);
    }

    const Resolved m_resolved;
    const LocationArgs m_location;
    const Position m_position;
    const QString m_newName;
    const bool m_apply;
    const int m_limit;
    const ToolResultHandler m_handler;
    QString m_oldName;
    WorkspaceEdit m_edit;
    EditList m_edits;
    QJsonArray m_conflicts;
    QString m_note;
};

void lspRename(const QJsonObject &args, const ToolResultHandler &handler)
{
    const Result<LocationArgs> location = parseLocationArgs(args);
    if (!location) {
        handler(ResultError(location.error()));
        return;
    }
    const QString newName = args.value("new_name").toString();
    if (newName.isEmpty()) {
        handler(ResultError(QString("Requires \"new_name\".")));
        return;
    }
    const bool apply = args.value("apply").toBool();
    const int limit = resultLimit(args);

    resolveClient(location->file, [location = *location, newName, apply, limit, handler](
                                      const Result<Resolved> &resolved) {
        if (!resolved) {
            handler(ResultError(resolved.error()));
            return;
        }
        const std::optional<std::variant<ServerCapabilities::RenameOptions, bool>> provider
            = resolved->client->capabilities().renameProvider();
        bool supported = provider.has_value();
        bool prepareSupported = false;
        if (provider) {
            if (const bool *enabled = std::get_if<bool>(&*provider))
                supported = *enabled;
            else if (const auto *options = std::get_if<ServerCapabilities::RenameOptions>(&*provider))
                prepareSupported = options->prepareProvider().value_or(false);
        }
        if (!supported) {
            handler(ResultError(unsupported(*resolved, "renaming")));
            return;
        }
        const Result<Position> position = positionFor(resolved->document, location.line,
                                                      location.column);
        if (!position) {
            handler(ResultError(position.error()));
            return;
        }
        auto rename = std::make_shared<Rename>(*resolved, location, *position, newName, apply,
                                               limit, handler);
        rename->start(prepareSupported);
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

    registerAsyncTool(
        Tool{}
            .name("lsp_type_hierarchy")
            .title("Get the type hierarchy via the language server")
            .description(
                "Returns the base classes (\"supertypes\"), the derived classes "
                "(\"subtypes\") or both of the class at a position, as the language server "
                "(clangd for C++, or whatever server is configured for the file type) "
                "computes them from its index. Give the file and a 1-based line and column "
                "on a class name. Each entry has the type's name, kind, file and position, "
                "and, with a \"depth\" above 1, its own \"supertypes\" or \"subtypes\" "
                "nested below it. The file is opened in a hidden editor if it is not open; "
                "a server that is still starting asks to be retried.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                positionInputSchema("the class name")
                    .addProperty(
                        "direction",
                        QJsonObject{{"type", "string"},
                                    {"enum", QJsonArray{"supertypes", "subtypes", "both"}},
                                    {"description",
                                     "\"supertypes\" for the bases, \"subtypes\" for the "
                                     "derived classes, \"both\" (default) for both."}})
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
                        "supertypes",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addProperty(
                        "subtypes",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addProperty("total", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addProperty("note", QJsonObject{{"type", "string"}})
                    .addRequired("root")),
        &lspTypeHierarchy);

    registerAsyncTool(
        Tool{}
            .name("lsp_references")
            .title("Find references via the language server")
            .description(
                "Finds every reference to the symbol at a position, as the language server "
                "(clangd for C++, qmlls for QML, or whatever server is configured for the "
                "file type) knows them from its index - templates, overloads and macros "
                "included, across all files it has indexed. Give the file and a 1-based "
                "line and column on an identifier. Each reference has its file and 1-based "
                "line/column to end_line/end_column; the declaration is included unless "
                "\"include_declaration\" is false. The list is sorted by file and position "
                "and capped by \"limit\", with \"total\" and \"truncated\" saying what "
                "was left out. The file is opened in a hidden editor if it is not open; a "
                "server that is still starting asks to be retried.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                positionInputSchema("the identifier")
                    .addProperty(
                        "include_declaration",
                        QJsonObject{{"type", "boolean"},
                                    {"description",
                                     "Also list the symbol's declaration(s). Default true."}})
                    .addProperty("limit", limitProperty()))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "references",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addProperty("include_declaration", QJsonObject{{"type", "boolean"}})
                    .addProperty("total", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addProperty("note", QJsonObject{{"type", "string"}})
                    .addRequired("references")
                    .addRequired("total")),
        &lspReferences);

    registerAsyncTool(
        Tool{}
            .name("lsp_rename")
            .title("Rename a symbol via the language server")
            .description(
                "Renames the symbol at a position everywhere the language server (clangd "
                "for C++, qmlls for QML, or whatever server is configured for the file type) "
                "knows it - templates, overloads and macros included. Give the file, a "
                "1-based line and column on the identifier, and the \"new_name\". By default "
                "this is a DRY RUN: it returns the edits the server proposes (each with file, "
                "1-based position, old and new text) and changes nothing. Set \"apply\" to "
                "true to make the edits: they reach the files on disk, and a file open in an "
                "editor is updated there too, unless it had unsaved changes of its own, in "
                "which case it is edited but not saved and named in \"unsaved_files\". The "
                "server refuses a name that clashes within the same scope; other "
                "symbols that already carry the new name anywhere in the project are listed "
                "as \"conflicts\" for you to judge, and do not block. The file is opened in a "
                "hidden editor if it is not open; a server that is still starting asks to be "
                "retried.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).destructiveHint(true))
            .inputSchema(
                positionInputSchema("the identifier to rename")
                    .addProperty("new_name",
                                 QJsonObject{{"type", "string"},
                                             {"description", "The new identifier name."}})
                    .addProperty(
                        "apply",
                        QJsonObject{{"type", "boolean"},
                                    {"description",
                                     "Make the edits. Default false (dry run)."}})
                    .addProperty("limit", limitProperty())
                    .addRequired("new_name"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("symbol", QJsonObject{{"type", "string"}})
                    .addProperty("new_name", QJsonObject{{"type", "string"}})
                    .addProperty("applied", QJsonObject{{"type", "boolean"}})
                    .addProperty(
                        "edits",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addProperty("total_edits", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                    .addProperty(
                        "files_affected",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addProperty(
                        "files_changed",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addProperty(
                        "unsaved_files",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addProperty("has_conflicts", QJsonObject{{"type", "boolean"}})
                    .addProperty(
                        "conflicts",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}},
                                    {"description",
                                     "Other symbols already named new_name: name, container, "
                                     "kind and location."}})
                    .addProperty("other_changes", QJsonObject{{"type", "integer"}})
                    .addProperty("note", QJsonObject{{"type", "string"}})
                    .addRequired("applied")
                    .addRequired("edits")
                    .addRequired("total_edits")
                    .addRequired("has_conflicts")),
        &lspRename);
}

} // namespace LanguageClient
