// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "foldingrangesupport.h"

#include "client.h"
#include "languageclientmanager.h"
#include "languageclientutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>

#include <languageserverprotocol/lspmessages.h>
#include <languageserverprotocol/lsputils.h>

#include <QSet>

#include <utility>

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
                if (auto widget = TextEditorWidget::fromEditor(editor))
                    requestFoldingRanges(widget->textDocument());
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

    void foldOrUnfoldCommentBlocks(TextEditorWidget *widget, bool fold)
    {
        QTC_ASSERT(widget, return);
        if (m_client->documentVersion(widget->textDocument()->filePath()) != m_savedRanges.first)
            return;

        QTextDocument *const doc = widget->textDocument()->document();

        for (const FoldingRange &r : std::as_const(m_savedRanges.second)) {
            if (r.kind() != "comment")
                continue;

            const QTextBlock start = doc->findBlockByNumber(r.startLine() + 1);
            const QTextBlock end = doc->findBlockByNumber(r.endLine() + 1);
            for (QTextBlock b = start; b != end; b = b.next()) {
                if (fold)
                    widget->fold(b);
                else
                    widget->unfold(b);
            }
        }
    }

    void foldOrUnfoldInactiveRegions(TextEditorWidget *widget, bool fold)
    {
        QTC_ASSERT(widget, return);
        if (m_client->documentVersion(widget->textDocument()->filePath()) != m_savedRanges.first)
            return;

        QTextDocument *const doc = widget->textDocument()->document();

        for (const FoldingRange &r : std::as_const(m_savedRanges.second)) {
            if (r.kind() != "region")
                continue;

            // Unfortunately, LSP does not define enough folding range kinds
            // (see https://github.com/microsoft/language-server-protocol/issues/1779),
            // so we have to find out heuristically whether this region corresponds
            // to an inactive one.
            // The logic is:
            //   - If all blocks inside the range are ifdefed out AND
            //   - the block before starts with an "#if" AND
            //   - the block after starts with an "#else", "#elif" or "#endif",
            // then the range corresponds to a foldable inactive region.
            // This will not catch regions where people try to be smart with comments inside
            // the preprocessor directives, but such people don't deserve happiness anyway.

            const QTextBlock start = doc->findBlockByNumber(r.startLine());
            const QTextBlock end = doc->findBlockByNumber(r.endLine() + 1);
            bool allInactive = true;
            for (QTextBlock b = start.next(); b.isValid() && b != end; b = b.next()) {
                if (!TextBlockUserData::ifdefedOut(b)) {
                    allInactive = false;
                    break;
                }
            }
            if (!allInactive)
                continue;
            const QString startText = start.text().simplified();
            if (!startText.startsWith("#if") && !startText.startsWith("# if")
                && !startText.startsWith("#el") && !startText.startsWith("# el")) {
                continue;
            }
            const QString endText = end.text().simplified();
            if (!endText.startsWith("#el") && !endText.startsWith("# el")
                && !endText.startsWith("#en") && !endText.startsWith("# en")) {
                continue;
            }

            for (QTextBlock b = start; b.isValid() && b != end; b = b.next()) {
                if (fold)
                    widget->fold(b);
                else
                    widget->unfold(b);
            }
        }
    }

private:
    void doRequestRanges(TextDocument *doc)
    {
        QTC_ASSERT(m_client->reachable(), return);
        m_queued.remove(doc);

        if (!m_client->documentOpen(doc))
            return;

        const QJsonValue registration = m_client->dynamicCapabilities().option(
            FoldingRangeRequest::method);
        if (!registration.isUndefined() && !registration.isNull()) {
            if (!registrationApplies(registration, doc->filePath(), doc->mimeType()))
                return;
        } else if (const auto &provider = m_client->capabilities().foldingRangeProvider()) {
            if (const auto options = std::get_if<FoldingRangeRegistrationOptions>(&*provider)) {
                if (!applies(
                        options->documentSelector(),
                        doc->filePath(),
                        Utils::mimeTypeForName(doc->mimeType()))) {
                    return;
                }
            }
        } else {
            return;
        }

        doc->setFoldingIndentExternallyProvided(true);

        const Utils::FilePath filePath = doc->filePath();
        FoldingRangeParams params;
        params.textDocument(TextDocumentIdentifier().uri(m_client->uriFor(filePath)));
        auto responseCallback =
            [this, filePath, documentVersion = m_client->documentVersion(filePath)](
                const Utils::Result<FoldingRangeRequestResult> &result) {
                m_runningRequests.remove(filePath);
                if (!result) {
                    qCDebug(logFolding) << result.error();
                    return;
                }
                handleFoldingRanges(filePath, *result, documentVersion);
            };
        qCDebug(logFolding) << "Requesting folding ranges for" << filePath << "with version"
                            << m_client->documentVersion(filePath);
        MessageId &id = m_runningRequests[filePath];
        if (id.isValid())
            m_client->cancelRequest(id);
        id = m_client->sendRequest<FoldingRangeRequest>(params, responseCallback);
    }

    void handleFoldingRanges(
        const Utils::FilePath &filePath,
        const FoldingRangeRequestResult &result,
        int documentVersion)
    {
        TextDocument * const doc = TextDocument::textDocumentForFilePath(filePath);
        if (!doc || LanguageClientManager::clientForDocument(doc) != m_client)
            return;

        if (m_client->documentVersion(filePath) != documentVersion)
            return;

        const auto ranges = std::get_if<QList<FoldingRange>>(&result);
        const auto newRanges = ranges ? *ranges : QList<FoldingRange>();
        m_savedRanges.first = documentVersion;
        if (newRanges == m_savedRanges.second)
            return;
        m_savedRanges.second = newRanges;

        QTextDocument * const docdoc = doc->document();
        for (QTextBlock b = docdoc->begin(); b != docdoc->end(); b = b.next()) {
            TextBlockUserData::setFoldingIndent(b, 0);
            TextBlockUserData::setFoldingStartIncluded(b, false);
            TextBlockUserData::setFoldingEndIncluded(b, false);
        }
        for (const FoldingRange &range : newRanges) {
            const QTextBlock start = docdoc->findBlockByNumber(range.startLine() + 1);
            const QTextBlock end = docdoc->findBlockByNumber(range.endLine() + 1);
            for (QTextBlock b = start; b != end; b = b.next())
                TextBlockUserData::changeFoldingIndent(b, 1);
        }
    }

    Client * const m_client;
    QSet<TextDocument *> m_queued;
    QHash<Utils::FilePath, MessageId> m_runningRequests;
    std::pair<int, QList<FoldingRange>> m_savedRanges;
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

void FoldingRangeSupport::foldOrUnfoldCommentBlocks(TextEditorWidget *widget, bool fold)
{
    d->foldOrUnfoldCommentBlocks(widget, fold);
}

void FoldingRangeSupport::foldOrUnfoldInactiveRegions(TextEditorWidget *widget, bool fold)
{
    d->foldOrUnfoldInactiveRegions(widget, fold);
}

void FoldingRangeSupport::deactivate(TextEditor::TextDocument *doc)
{
    doc->setFoldingIndentExternallyProvided(false);
}

void FoldingRangeSupport::refresh()
{
    for (IEditor *editor : EditorManager::visibleEditors()) {
        if (auto doc = qobject_cast<TextDocument *>(editor->document()))
            requestFoldingRanges(doc);
    }
}

} // namespace LanguageClient::Internal

#include <foldingrangesupport.moc>
