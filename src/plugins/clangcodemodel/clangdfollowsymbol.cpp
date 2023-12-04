// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdfollowsymbol.h"

#include "clangcodemodeltr.h"
#include "clangdast.h"
#include "clangdclient.h"

#include <cppeditor/cppeditorwidget.h>
#include <cppeditor/cppvirtualfunctionassistprovider.h>
#include <cppeditor/cppvirtualfunctionproposalitem.h>

#include <languageclient/languageclientsymbolsupport.h>
#include <languageserverprotocol/lsptypes.h>
#include <languageserverprotocol/jsonrpcmessages.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/iassistprovider.h>
#include <texteditor/textdocument.h>

#include <QApplication>
#include <QPointer>

using namespace CppEditor;
using namespace LanguageServerProtocol;
using namespace TextEditor;
using namespace Utils;

namespace ClangCodeModel::Internal {

using SymbolData = QPair<QString, Link>;
using SymbolDataList = QList<SymbolData>;

class ClangdFollowSymbol::VirtualFunctionAssistProcessor : public IAssistProcessor
{
public:
    VirtualFunctionAssistProcessor(ClangdFollowSymbol *followSymbol)
        : m_followSymbol(followSymbol) {}

    void cancel() override { resetData(true); }
    bool running() override { return m_followSymbol && m_running; }
    void update();
    void finalize();
    void resetData(bool resetFollowSymbolData);

private:
    IAssistProposal *perform() override
    {
        return createProposal(false);
    }

    IAssistProposal *createProposal(bool final);
    VirtualFunctionProposalItem *createEntry(const QString &name, const Link &link) const;

    QPointer<ClangdFollowSymbol> m_followSymbol;
    bool m_running = false;
};

class ClangdFollowSymbol::VirtualFunctionAssistProvider : public IAssistProvider
{
public:
    VirtualFunctionAssistProvider(ClangdFollowSymbol *followSymbol)
        : m_followSymbol(followSymbol) {}

private:
    IAssistProcessor *createProcessor(const AssistInterface *interface) const override;

    const QPointer<ClangdFollowSymbol> m_followSymbol;
};

class ClangdFollowSymbol::Private
{
public:
    Private(ClangdFollowSymbol *q, ClangdClient *client, const QTextCursor &cursor,
            CppEditorWidget *editorWidget, const FilePath &filePath, const LinkHandler &callback,
            bool openInSplit)
        : q(q), client(client), cursor(cursor), editorWidget(editorWidget),
          uri(client->hostPathToServerUri(filePath)), callback(callback),
          virtualFuncAssistProvider(q),
          docRevision(editorWidget ? editorWidget->textDocument()->document()->revision() : -1),
          openInSplit(openInSplit) {}

    void goToTypeDefinition();
    void handleGotoDefinitionResult();
    void sendGotoImplementationRequest(const Utils::Link &link);
    void handleGotoImplementationResult(const GotoImplementationRequest::Response &response);
    void handleDocumentInfoResults();
    void closeTempDocuments();
    bool addOpenFile(const FilePath &filePath);
    bool defLinkIsAmbiguous() const;
    void cancel();

    ClangdFollowSymbol * const q;
    ClangdClient * const client;
    const QTextCursor cursor;
    const QPointer<CppEditor::CppEditorWidget> editorWidget;
    const DocumentUri uri;
    const LinkHandler callback;
    VirtualFunctionAssistProvider virtualFuncAssistProvider;
    QList<MessageId> pendingSymbolInfoRequests;
    QList<MessageId> pendingGotoImplRequests;
    QList<MessageId> pendingGotoDefRequests;
    const int docRevision;
    const bool openInSplit;

    Link defLink;
    Links allLinks;
    QHash<Link, Link> declDefMap;
    std::optional<ClangdAstNode> cursorNode;
    ClangdAstNode defLinkNode;
    SymbolDataList symbolsToDisplay;
    std::set<FilePath> openedFiles;
    VirtualFunctionAssistProcessor *virtualFuncAssistProcessor = nullptr;
    QMetaObject::Connection focusChangedConnection;
    bool done = false;
};

ClangdFollowSymbol::ClangdFollowSymbol(ClangdClient *client, const QTextCursor &cursor,
        CppEditorWidget *editorWidget, TextDocument *document, const LinkHandler &callback,
        FollowTo followTo, bool openInSplit)
    : QObject(client),
      d(new Private(this, client, cursor, editorWidget, document->filePath(), callback,
                    openInSplit))
{
    // Abort if the user does something else with the document in the meantime.
    connect(document, &TextDocument::contentsChanged, this, [this] { emitDone(); },
            Qt::QueuedConnection);
    if (editorWidget) {
        connect(editorWidget, &CppEditorWidget::cursorPositionChanged,
                this, [this] { emitDone(); }, Qt::QueuedConnection);
    }
    d->focusChangedConnection = connect(qApp, &QApplication::focusChanged,
                                        this, [this] { emitDone(); }, Qt::QueuedConnection);

    if (followTo == FollowTo::SymbolType) {
        d->goToTypeDefinition();
        return;
    }

    // Step 1: Follow the symbol via "Go to Definition". At the same time, request the
    //         AST node corresponding to the cursor position, so we can find out whether
    //         we have to look for overrides.
    const auto gotoDefCallback = [self = QPointer(this)](const Utils::Link &link) {
        qCDebug(clangdLog) << "received go to definition response";
        if (!self)
            return;
        if (!link.hasValidTarget()) {
            self->emitDone();
            return;
        }
        self->d->defLink = link;
        if (self->d->cursorNode)
            self->d->handleGotoDefinitionResult();
    };
    client->symbolSupport().findLinkAt(document,
                                       cursor,
                                       std::move(gotoDefCallback),
                                       true,
                                       LanguageClient::LinkTarget::SymbolDef);

    const auto astHandler = [self = QPointer(this)](const ClangdAstNode &ast, const MessageId &) {
        qCDebug(clangdLog) << "received ast response for cursor";
        if (!self)
            return;
        self->d->cursorNode = ast;
        if (self->d->defLink.hasValidTarget())
            self->d->handleGotoDefinitionResult();
    };
    client->getAndHandleAst(document, astHandler, ClangdClient::AstCallbackMode::AlwaysAsync,
                            Range(cursor));
}

ClangdFollowSymbol::~ClangdFollowSymbol()
{
    d->cancel();
    delete d;
}

void ClangdFollowSymbol::cancel()
{
    d->cancel();
    clear();
    emitDone();
}

void ClangdFollowSymbol::clear()
{
    d->openedFiles.clear();
    d->pendingSymbolInfoRequests.clear();
    d->pendingGotoImplRequests.clear();
    d->pendingGotoDefRequests.clear();
}

void ClangdFollowSymbol::emitDone(const Link &link)
{
    if (d->done)
        return;

    d->done = true;
    if (link.hasValidTarget())
        d->callback(link);
    emit done();
}

bool ClangdFollowSymbol::Private::defLinkIsAmbiguous() const
{
    // Even if the call is to a virtual function, it might not be ambiguous:
    // class A { virtual void f(); }; class B : public A { void f() override { A::f(); } };
    if (!cursorNode->mightBeAmbiguousVirtualCall() && !cursorNode->isPureVirtualDeclaration())
        return false;

    // If we have up-to-date highlighting info, we know whether we are dealing with
    // a virtual call.
    if (editorWidget) {
        const auto result = client->hasVirtualFunctionAt(editorWidget->textDocument(),
                                                         docRevision, cursorNode->range());
        if (result.has_value())
            return *result;
    }

    // Otherwise, we accept potentially doing more work than needed rather than not catching
    // possible overrides.
    return true;
}

void ClangdFollowSymbol::Private::cancel()
{
    closeTempDocuments();
    if (virtualFuncAssistProcessor)
        virtualFuncAssistProcessor->resetData(false);
    for (const MessageId &id : std::as_const(pendingSymbolInfoRequests))
        client->cancelRequest(id);
    for (const MessageId &id : std::as_const(pendingGotoImplRequests))
        client->cancelRequest(id);
    for (const MessageId &id : std::as_const(pendingGotoDefRequests))
        client->cancelRequest(id);
}

bool ClangdFollowSymbol::Private::addOpenFile(const FilePath &filePath)
{
    return openedFiles.insert(filePath).second;
}

void ClangdFollowSymbol::Private::handleDocumentInfoResults()
{
    closeTempDocuments();

    // If something went wrong, we just follow the original link.
    if (symbolsToDisplay.isEmpty()) {
        q->emitDone(defLink);
        return;
    }

    if (symbolsToDisplay.size() == 1) {
        q->emitDone(symbolsToDisplay.first().second);
        return;
    }

    QTC_ASSERT(virtualFuncAssistProcessor && virtualFuncAssistProcessor->running(),
               q->emitDone(); return);
    virtualFuncAssistProcessor->finalize();
}

void ClangdFollowSymbol::Private::sendGotoImplementationRequest(const Link &link)
{
    if (!client->documentForFilePath(link.targetFilePath) && addOpenFile(link.targetFilePath))
        client->openExtraFile(link.targetFilePath);
    const Position position(link.targetLine - 1, link.targetColumn);
    const TextDocumentIdentifier documentId(client->hostPathToServerUri(link.targetFilePath));
    GotoImplementationRequest req(TextDocumentPositionParams(documentId, position));
    req.setResponseCallback([sentinel = QPointer(q), this, reqId = req.id()]
                            (const GotoImplementationRequest::Response &response) {
        qCDebug(clangdLog) << "received go to implementation reply";
        if (!sentinel)
            return;
        pendingGotoImplRequests.removeOne(reqId);
        handleGotoImplementationResult(response);
    });
    client->sendMessage(req, ClangdClient::SendDocUpdates::Ignore);
    pendingGotoImplRequests << req.id();
    qCDebug(clangdLog) << "sending go to implementation request" << link.targetLine;
}

void ClangdFollowSymbol::VirtualFunctionAssistProcessor::update()
{
    if (!m_followSymbol->d->editorWidget)
        return;
    setAsyncProposalAvailable(createProposal(false));
}

void ClangdFollowSymbol::VirtualFunctionAssistProcessor::finalize()
{
    if (!m_followSymbol->d->editorWidget)
        return;
    const auto proposal = createProposal(true);
    if (m_followSymbol->d->editorWidget->isInTestMode()) {
        m_followSymbol->d->symbolsToDisplay.clear();
        const auto immediateProposal = createProposal(false);
        m_followSymbol->d->editorWidget->setProposals(immediateProposal, proposal);
    } else {
        setAsyncProposalAvailable(proposal);
    }
    resetData(true);
}

void ClangdFollowSymbol::VirtualFunctionAssistProcessor::resetData(bool resetFollowSymbolData)
{
    if (!m_followSymbol)
        return;
    m_followSymbol->d->virtualFuncAssistProcessor = nullptr;
    ClangdFollowSymbol * const followSymbol = m_followSymbol;
    m_followSymbol = nullptr;
    if (resetFollowSymbolData)
        followSymbol->emitDone();
}

IAssistProposal *ClangdFollowSymbol::VirtualFunctionAssistProcessor::createProposal(bool final)
{
    QTC_ASSERT(m_followSymbol, return nullptr);
    m_running = !final;

    QList<AssistProposalItemInterface *> items;
    bool needsBaseDeclEntry = !m_followSymbol->d->defLinkNode.range()
            .contains(Position(m_followSymbol->d->cursor));
    for (const SymbolData &symbol : std::as_const(m_followSymbol->d->symbolsToDisplay)) {
        Link link = symbol.second;
        if (m_followSymbol->d->defLink == link) {
            if (!needsBaseDeclEntry)
                continue;
            needsBaseDeclEntry = false;
        } else {
            const Link defLink = m_followSymbol->d->declDefMap.value(symbol.second);
            if (defLink.hasValidTarget())
                link = defLink;
        }
        items << createEntry(symbol.first, link);
    }
    if (needsBaseDeclEntry)
        items << createEntry({}, m_followSymbol->d->defLink);
    if (!final) {
        const auto infoItem = new VirtualFunctionProposalItem({}, false);
        infoItem->setText(Tr::tr("collecting overrides..."));
        infoItem->setOrder(-1);
        items << infoItem;
    }

    return new VirtualFunctionProposal(m_followSymbol->d->cursor.position(), items,
                                       m_followSymbol->d->openInSplit);
}

CppEditor::VirtualFunctionProposalItem *
ClangdFollowSymbol::VirtualFunctionAssistProcessor::createEntry(const QString &name,
                                                                const Link &link) const
{
    const auto item = new VirtualFunctionProposalItem(link, m_followSymbol->d->openInSplit);
    QString text = name;
    if (link == m_followSymbol->d->defLink) {
        item->setOrder(1000); // Ensure base declaration is on top.
        if (text.isEmpty()) {
            text = Tr::tr("<base declaration>");
        } else if (m_followSymbol->d->defLinkNode.isPureVirtualDeclaration()
                   || m_followSymbol->d->defLinkNode.isPureVirtualDefinition()) {
            text += " = 0";
        }
    }
    item->setText(text);
    return item;
}

IAssistProcessor *
ClangdFollowSymbol::VirtualFunctionAssistProvider::createProcessor(const AssistInterface *) const
{
    return m_followSymbol->d->virtualFuncAssistProcessor
            = new VirtualFunctionAssistProcessor(m_followSymbol);
}

void ClangdFollowSymbol::Private::goToTypeDefinition()
{
    GotoTypeDefinitionRequest req(TextDocumentPositionParams(TextDocumentIdentifier{uri},
                                                             Position(cursor)));
    req.setResponseCallback([sentinel = QPointer(q), this, reqId = req.id()]
                            (const GotoTypeDefinitionRequest::Response &response) {
        qCDebug(clangdLog) << "received go to type definition reply";
        if (!sentinel)
            return;
        Link link;

        if (const std::optional<GotoResult> &result = response.result()) {
            if (const auto ploc = std::get_if<Location>(&*result)) {
                link = {ploc->toLink(client->hostPathMapper())};
            } else if (const auto plloc = std::get_if<QList<Location>>(&*result)) {
                if (!plloc->empty())
                    link = plloc->first().toLink(client->hostPathMapper());
            }
        }
        q->emitDone(link);
    });
    client->sendMessage(req, ClangdClient::SendDocUpdates::Ignore);
    qCDebug(clangdLog) << "sending go to type definition request";
}

void ClangdFollowSymbol::Private::handleGotoDefinitionResult()
{
    QTC_ASSERT(defLink.hasValidTarget(), return);

    qCDebug(clangdLog) << "handling go to definition result";

    // No dis-ambiguation necessary. Call back with the link and finish.
    if (!defLinkIsAmbiguous()) {
        q->emitDone(defLink);
        return;
    }

    // Step 2: Get all possible overrides via "Go to Implementation".
    // Note that we have to do this for all member function calls, because
    // we cannot tell here whether the member function is virtual.
    allLinks << defLink;
    sendGotoImplementationRequest(defLink);
}

void ClangdFollowSymbol::Private::handleGotoImplementationResult(
        const GotoImplementationRequest::Response &response)
{
    auto transformLink = [mapper = client->hostPathMapper()](const Location &loc) {
        return loc.toLink(mapper);
    };
    if (const std::optional<GotoResult> &result = response.result()) {
        QList<Link> newLinks;
        if (const auto ploc = std::get_if<Location>(&*result))
            newLinks = {transformLink(*ploc)};
        if (const auto plloc = std::get_if<QList<Location>>(&*result))
            newLinks = transform(*plloc, transformLink);
        for (const Link &link : std::as_const(newLinks)) {
            if (!allLinks.contains(link)) {
                allLinks << link;

                // We must do this recursively, because clangd reports only the first
                // level of overrides.
                sendGotoImplementationRequest(link);
            }
        }
    }

    // We didn't find any further candidates, so jump to the original definition link.
    if (allLinks.size() == 1 && pendingGotoImplRequests.isEmpty()) {
        q->emitDone(allLinks.first());
        return;
    }

    // As soon as we know that there is more than one candidate, we start the code assist
    // procedure, to let the user know that things are happening.
    if (allLinks.size() > 1 && !virtualFuncAssistProcessor && editorWidget) {
        QObject::disconnect(focusChangedConnection);
        editorWidget->invokeTextEditorWidgetAssist(FollowSymbol, &virtualFuncAssistProvider);
    }

    if (!pendingGotoImplRequests.isEmpty())
        return;

    // Step 3: We are done looking for overrides, and we found at least one.
    //         Make a symbol info request for each link to get the class names.
    //         Also get the AST for the base declaration, so we can find out whether it's
    //         pure virtual and mark it accordingly.
    //         In addition, we need to follow all override links, because for these, clangd
    //         gives us the declaration instead of the definition (until clangd 16).
    for (const Link &link : std::as_const(allLinks)) {
        if (!client->documentForFilePath(link.targetFilePath) && addOpenFile(link.targetFilePath))
            client->openExtraFile(link.targetFilePath);
        const auto symbolInfoHandler = [sentinel = QPointer(q), this, link](
                const QString &name, const QString &prefix, const MessageId &reqId) {
            qCDebug(clangdLog) << "handling symbol info reply"
                               << link.targetFilePath.toUserOutput() << link.targetLine;
            if (!sentinel || !virtualFuncAssistProcessor)
                return;
            if (!name.isEmpty())
                symbolsToDisplay.push_back({prefix + name, link});
            pendingSymbolInfoRequests.removeOne(reqId);
            virtualFuncAssistProcessor->update();
            if (pendingSymbolInfoRequests.isEmpty() && pendingGotoDefRequests.isEmpty()
                    && defLinkNode.isValid()) {
                handleDocumentInfoResults();
            }
        };
        const Position pos(link.targetLine - 1, link.targetColumn);
        const MessageId reqId = client->requestSymbolInfo(link.targetFilePath, pos,
                                                          symbolInfoHandler);
        pendingSymbolInfoRequests << reqId;
        qCDebug(clangdLog) << "sending symbol info request";

        if (link == defLink)
            continue;

        if (client->versionNumber().majorVersion() >= 17)
            continue;

        const TextDocumentIdentifier doc(client->hostPathToServerUri(link.targetFilePath));
        const TextDocumentPositionParams params(doc, pos);
        GotoDefinitionRequest defReq(params);
        defReq.setResponseCallback(
            [this, link, transformLink, sentinel = QPointer(q), reqId = defReq.id()](
                const GotoDefinitionRequest::Response &response) {
                qCDebug(clangdLog) << "handling additional go to definition reply for"
                                   << link.targetFilePath << link.targetLine;
                if (!sentinel)
                    return;
                Link newLink;
                if (std::optional<GotoResult> _result = response.result()) {
                    const GotoResult result = _result.value();
                    if (const auto ploc = std::get_if<Location>(&result)) {
                        newLink = transformLink(*ploc);
                    } else if (const auto plloc = std::get_if<QList<Location>>(&result)) {
                        if (!plloc->isEmpty())
                            newLink = transformLink(plloc->value(0));
                    }
                }
                qCDebug(clangdLog) << "def link is" << newLink.targetFilePath << newLink.targetLine;
                declDefMap.insert(link, newLink);
                pendingGotoDefRequests.removeOne(reqId);
                if (pendingSymbolInfoRequests.isEmpty() && pendingGotoDefRequests.isEmpty()
                    && defLinkNode.isValid()) {
                    handleDocumentInfoResults();
                }
            });
        pendingGotoDefRequests << defReq.id();
        qCDebug(clangdLog) << "sending additional go to definition request"
                           << link.targetFilePath << link.targetLine;
        client->sendMessage(defReq, ClangdClient::SendDocUpdates::Ignore);
    }

    const FilePath defLinkFilePath = defLink.targetFilePath;
    const TextDocument * const defLinkDoc = client->documentForFilePath(defLinkFilePath);
    const auto defLinkDocVariant = defLinkDoc ? ClangdClient::TextDocOrFile(defLinkDoc)
                                              : ClangdClient::TextDocOrFile(defLinkFilePath);
    const Position defLinkPos(defLink.targetLine - 1, defLink.targetColumn);
    const auto astHandler = [this, sentinel = QPointer(q)]
            (const ClangdAstNode &ast, const MessageId &) {
        qCDebug(clangdLog) << "received ast response for def link";
        if (!sentinel)
            return;
        defLinkNode = ast;
        if (pendingSymbolInfoRequests.isEmpty() && pendingGotoDefRequests.isEmpty())
            handleDocumentInfoResults();
    };
    client->getAndHandleAst(defLinkDocVariant, astHandler,
                            ClangdClient::AstCallbackMode::AlwaysAsync,
                            Range(defLinkPos, defLinkPos));
}

void ClangdFollowSymbol::Private::closeTempDocuments()
{
    for (const FilePath &fp : std::as_const(openedFiles)) {
        if (!client->documentForFilePath(fp))
            client->closeExtraFile(fp);
    }
    openedFiles.clear();
}

} // namespace ClangCodeModel::Internal
