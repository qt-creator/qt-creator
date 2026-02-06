// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "foldingrangesupport.h"

#include "client.h"
#include "languageclientmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

#include <QSet>

using namespace Core;
using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace LanguageClient::Internal {

static Q_LOGGING_CATEGORY(logFolding, "qtc.languageclient.folding", QtWarningMsg);

class FoldingRangeSupportImpl : public QObject
{
    Q_OBJECT
public:
    FoldingRangeSupportImpl(Client * client) : m_client(client)
    {
        QObject::connect(EditorManager::instance(), &EditorManager::currentEditorChanged, this,
            [this](IEditor *editor) {
                if (auto textEditor = qobject_cast<BaseTextEditor *>(editor))
                    requestFoldingRanges(textEditor->textDocument());
            });
    }

    void requestFoldingRanges(TextDocument *doc)
    {
        if (m_client->reachable()) {
            doRequestRanges(doc);
            return;
        }
        if (!Utils::insert(m_queued, doc))
            return;
        connect(m_client, &Client::initialized, this,
            [this, doc = QPointer<TextDocument>(doc)]() { if (doc) doRequestRanges(doc); },
            Qt::QueuedConnection);
    }

private:
    void doRequestRanges(TextDocument *doc)
    {
        QTC_ASSERT(m_client->reachable(), return);
        m_queued.remove(doc);

        if (!m_client->documentOpen(doc))
            return;

        const auto filterApplies = [doc] (const TextDocumentRegistrationOptions *options) {
            return options->filterApplies(doc->filePath(), Utils::mimeTypeForName(doc->mimeType()));
        };
        TextDocumentRegistrationOptions options(
            m_client->dynamicCapabilities().option(FoldingRangeRequest::methodName).toObject());
        if (options.isValid() && !filterApplies(&options)) {
            return;
        } else if (auto provider = m_client->capabilities().foldingRangeProvider()) {
            using Options = ServerCapabilities::FoldingRangeRegistrationOptions;
            if (auto options = std::get_if<Options>(&*provider)) {
                if (!filterApplies(options))
                    return;
            }
        } else {
            return;
        }

        doc->setFoldingIndentExternallyProvided(true);

        const Utils::FilePath filePath = doc->filePath();
        const TextDocumentIdentifier docId(m_client->hostPathToServerUri(filePath));
        auto responseCallback = [this,
                                 filePath,
                                 documentVersion = m_client->documentVersion(filePath)](
                                    const FoldingRangeRequest::Response &response) {
            m_runningRequests.remove(filePath);
            if (const auto error = response.error()) {
                qCDebug(logFolding);
            } else {
                handleFoldingRanges(filePath, response.result().value_or(nullptr), documentVersion);
            }
        };
        TextDocumentParams params;
        params.setTextDocument(docId);
        FoldingRangeRequest request(params);
        request.setResponseCallback(responseCallback);
        qCDebug(logFolding) << "Requesting folding ranges for" << filePath << "with version"
                            << m_client->documentVersion(filePath);
        MessageId &id = m_runningRequests[filePath];
        if (id.isValid())
            m_client->cancelRequest(id);
        id = request.id();
        m_client->sendMessage(request);
    }

    void handleFoldingRanges(
        const Utils::FilePath &filePath, const FoldingRangeResult &result, int documentVersion)
    {
        TextDocument * const doc = TextDocument::textDocumentForFilePath(filePath);
        if (!doc || LanguageClientManager::clientForDocument(doc) != m_client)
            return;
        if (m_client->documentVersion(filePath) != documentVersion)
            return;

        const auto ranges = std::get_if<QList<FoldingRange>>(&result);
        if (!ranges)
            return;

        QTextDocument * const docdoc = doc->document();
        for (QTextBlock b = docdoc->begin(); b != docdoc->end(); b = b.next()) {
            TextBlockUserData::setFoldingIndent(b, 0);
            TextBlockUserData::setFoldingStartIncluded(b, false);
            TextBlockUserData::setFoldingEndIncluded(b, false);
        }
        for (const FoldingRange &range : *ranges) {
            const QTextBlock start = docdoc->findBlockByNumber(range.startLine() + 1);
            const QTextBlock end = docdoc->findBlockByNumber(range.endLine() + 1);
            for (QTextBlock b = start; b != end; b = b.next())
                TextBlockUserData::changeFoldingIndent(b, 1);
        }
    }

    Client * const m_client;
    QSet<TextDocument *> m_queued;
    QHash<Utils::FilePath, LanguageServerProtocol::MessageId> m_runningRequests;
};

FoldingRangeSupport::FoldingRangeSupport(Client *client) : d(new FoldingRangeSupportImpl(client))
{

}

FoldingRangeSupport::~FoldingRangeSupport()
{
    delete d;
}

void FoldingRangeSupport::requestFoldingRanges(TextEditor::TextDocument *doc)
{
    d->requestFoldingRanges(doc);
}

void FoldingRangeSupport::deactivate(TextEditor::TextDocument *doc)
{
    doc->setFoldingIndentExternallyProvided(false);
}

void FoldingRangeSupport::refresh()
{
    for (IEditor *editor : EditorManager::visibleEditors()) {
        if (auto textEditor = qobject_cast<BaseTextEditor *>(editor))
            requestFoldingRanges(textEditor->textDocument());
    }
}

} // namespace LanguageClient::Internal

#include <foldingrangesupport.moc>
