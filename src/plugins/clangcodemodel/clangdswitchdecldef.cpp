// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdswitchdecldef.h"

#include "clangdast.h"
#include "clangdclient.h"

#include <cppeditor/cppeditorwidget.h>
#include <languageclient/documentsymbolcache.h>
#include <languageserverprotocol/lsptypes.h>
#include <texteditor/textdocument.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QMetaObject>
#include <QTextCursor>

#include <optional>

using namespace CppEditor;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace TextEditor;
using namespace Utils;

namespace ClangCodeModel::Internal {

class ClangdSwitchDeclDef::Private
{
public:
    Private(ClangdSwitchDeclDef * q, ClangdClient *client, TextDocument *doc,
            const QTextCursor &cursor, CppEditorWidget *editorWidget, const LinkHandler &callback)
        : q(q), client(client), document(doc), uri(client->hostPathToServerUri(doc->filePath())),
          cursor(cursor), editorWidget(editorWidget), callback(callback)
    {}

    std::optional<ClangdAstNode> getFunctionNode() const;
    QTextCursor cursorForFunctionName(const ClangdAstNode &functionNode) const;
    void handleDeclDefSwitchReplies();

    ClangdSwitchDeclDef * const q;
    ClangdClient * const client;
    const QPointer<TextDocument> document;
    const DocumentUri uri;
    const QTextCursor cursor;
    const QPointer<CppEditorWidget> editorWidget;
    const LinkHandler callback;
    std::optional<ClangdAstNode> ast;
    std::optional<DocumentSymbolsResult> docSymbols;
    bool done = false;
};

ClangdSwitchDeclDef::ClangdSwitchDeclDef(ClangdClient *client, TextDocument *doc,
        const QTextCursor &cursor, CppEditorWidget *editorWidget, const LinkHandler &callback)
    : QObject(client), d(new Private(this, client, doc, cursor, editorWidget, callback))
{
    // Abort if the user does something else with the document in the meantime.
    connect(doc, &TextDocument::contentsChanged, this, &ClangdSwitchDeclDef::emitDone,
            Qt::QueuedConnection);
    if (editorWidget) {
        connect(editorWidget, &CppEditorWidget::cursorPositionChanged,
                this, &ClangdSwitchDeclDef::emitDone, Qt::QueuedConnection);
    }
    connect(qApp, &QApplication::focusChanged,
            this, &ClangdSwitchDeclDef::emitDone, Qt::QueuedConnection);

    connect(client->documentSymbolCache(), &DocumentSymbolCache::gotSymbols, this,
            [this](const DocumentUri &uri, const DocumentSymbolsResult &symbols) {
        if (uri != d->uri)
            return;
        d->client->documentSymbolCache()->disconnect(this);
        d->docSymbols = symbols;
        if (d->ast)
            d->handleDeclDefSwitchReplies();
    });


    // Retrieve AST and document symbols.
    const auto astHandler = [this, self = QPointer(this)]
            (const ClangdAstNode &ast, const MessageId &) {
        qCDebug(clangdLog) << "received ast for decl/def switch";
        if (!self)
            return;
         if (!d->document) {
             emitDone();
             return;
         }
        if (!ast.isValid()) {
            emitDone();
            return;
        }
        d->ast = ast;
        if (d->docSymbols)
            d->handleDeclDefSwitchReplies();

    };
    client->getAndHandleAst(doc, astHandler, ClangdClient::AstCallbackMode::SyncIfPossible, {});
    client->documentSymbolCache()->requestSymbols(d->uri, Schedule::Now);
}

ClangdSwitchDeclDef::~ClangdSwitchDeclDef()
{
    delete d;
}

void ClangdSwitchDeclDef::emitDone()
{
    if (d->done)
        return;

    d->done = true;
    QMetaObject::invokeMethod(this, &ClangdSwitchDeclDef::done, Qt::QueuedConnection);
}

std::optional<ClangdAstNode> ClangdSwitchDeclDef::Private::getFunctionNode() const
{
    QTC_ASSERT(ast, return {});

    const ClangdAstPath path = getAstPath(*ast, Range(cursor));
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if (it->role() == "declaration"
                && (it->kind() == "CXXMethod" || it->kind() == "CXXConversion"
                    || it->kind() == "CXXConstructor" || it->kind() == "CXXDestructor"
                    || it->kind() == "Function")) {
            return *it;
        }
    }
    return {};
}

QTextCursor ClangdSwitchDeclDef::Private::cursorForFunctionName(const ClangdAstNode &functionNode) const
{
    QTC_ASSERT(docSymbols, return {});

    const auto symbolList = std::get_if<QList<DocumentSymbol>>(&*docSymbols);
    if (!symbolList)
        return {};
    const Range &astRange = functionNode.range();
    QList symbolsToCheck = *symbolList;
    while (!symbolsToCheck.isEmpty()) {
        const DocumentSymbol symbol = symbolsToCheck.takeFirst();
        if (symbol.range() == astRange)
            return symbol.selectionRange().start().toTextCursor(document->document());
        if (symbol.range().contains(astRange))
            symbolsToCheck << symbol.children().value_or(QList<DocumentSymbol>());
    }
    return {};
}

void ClangdSwitchDeclDef::Private::handleDeclDefSwitchReplies()
{
    if (!document) {
        q->emitDone();
        return;
    }

    // Find the function declaration or definition associated with the cursor.
    // For instance, the cursor could be somwehere inside a function body or
    // on a function return type, or ...
    if (clangdLogAst().isDebugEnabled())
        ast->print(0);
    const std::optional<ClangdAstNode> functionNode = getFunctionNode();
    if (!functionNode) {
        q->emitDone();
        return;
    }

    // Unfortunately, the AST does not contain the location of the actual function name symbol,
    // so we have to look for it in the document symbols.
    const QTextCursor funcNameCursor = cursorForFunctionName(*functionNode);
    if (!funcNameCursor.isNull()) {
        client->followSymbol(document.data(), funcNameCursor, editorWidget, callback,
                             true, FollowTo::SymbolDef, false);
    }
    q->emitDone();
}

} // namespace ClangCodeModel::Internal
