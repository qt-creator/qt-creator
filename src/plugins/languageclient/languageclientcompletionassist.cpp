// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientcompletionassist.h"

#include "client.h"
#include "languageclientutils.h"
#include "snippet.h"

#include <languageserverprotocol/completion.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/genericproposalwidget.h>
#include <texteditor/snippets/snippet.h>
#include <texteditor/snippets/snippetassistcollector.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorsettings.h>
#include <utils/algorithm.h>
#include <utils/textutils.h>
#include <utils/utilsicons.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>
#include <QTime>

static Q_LOGGING_CATEGORY(LOGLSPCOMPLETION, "qtc.languageclient.completion", QtWarningMsg);

using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace LanguageClient {

LanguageClientCompletionItem::LanguageClientCompletionItem(CompletionItem item)
    : m_item(std::move(item))
{ }

QString LanguageClientCompletionItem::text() const
{ return m_item.label(); }

bool LanguageClientCompletionItem::implicitlyApplies() const
{ return false; }

bool LanguageClientCompletionItem::prematurelyApplies(const QChar &typedCharacter) const
{
    if (m_item.commitCharacters() && m_item.commitCharacters()->contains(typedCharacter)) {
        m_triggeredCommitCharacter = typedCharacter;
        return true;
    }
    return false;
}

void LanguageClientCompletionItem::apply(TextDocumentManipulatorInterface &manipulator,
                                         int /*basePosition*/) const
{
    if (auto edit = m_item.textEdit()) {
        applyTextEdit(manipulator, *edit, isSnippet());
    } else {
        const int pos = manipulator.currentPosition();
        const QString textToInsert(m_item.insertText().value_or(text()));
        int length = 0;
        for (auto it = textToInsert.crbegin(), end = textToInsert.crend(); it != end; ++it) {
            if (it->toLower() != manipulator.characterAt(pos - length - 1).toLower()) {
                length = 0;
                break;
            }
            ++length;
        }
        QTextCursor cursor = manipulator.textCursorAt(pos);
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        const QString blockTextUntilPosition = cursor.selectedText();
        static QRegularExpression identifier("[a-zA-Z_][a-zA-Z0-9_]*$");
        QRegularExpressionMatch match = identifier.match(blockTextUntilPosition);
        int matchLength = match.hasMatch() ? match.capturedLength(0) : 0;
        length = qMax(length, matchLength);
        if (isSnippet()) {
            manipulator.replace(pos - length, length, {});
            manipulator.insertCodeSnippet(pos - length, textToInsert, &parseSnippet);
        } else {
            manipulator.replace(pos - length, length, textToInsert);
        }
    }

    if (auto additionalEdits = m_item.additionalTextEdits()) {
        for (const auto &edit : *additionalEdits)
            applyTextEdit(manipulator, edit);
    }
    if (!m_triggeredCommitCharacter.isNull())
        manipulator.insertCodeSnippet(manipulator.currentPosition(),
                                      m_triggeredCommitCharacter,
                                      &Snippet::parse);
}

QIcon LanguageClientCompletionItem::icon() const
{
    QIcon icon;
    using namespace Utils::CodeModelIcon;
    const int kind = m_item.kind().value_or(CompletionItemKind::Text);
    switch (kind) {
    case CompletionItemKind::Method:
    case CompletionItemKind::Function:
    case CompletionItemKind::Constructor: icon = iconForType(FuncPublic); break;
    case CompletionItemKind::Field:
    case CompletionItemKind::Variable: icon = iconForType(VarPublic); break;
    case CompletionItemKind::Class: icon = iconForType(Class); break;
    case CompletionItemKind::Module: icon = iconForType(Namespace); break;
    case CompletionItemKind::Property: icon = iconForType(Property); break;
    case CompletionItemKind::Enum: icon = iconForType(Enum); break;
    case CompletionItemKind::Keyword: icon = iconForType(Keyword); break;
    case CompletionItemKind::Snippet: icon = QIcon(":/texteditor/images/snippet.png"); break;
    case CompletionItemKind::EnumMember: icon = iconForType(Enumerator); break;
    case CompletionItemKind::Struct: icon = iconForType(Struct); break;
    default: icon = iconForType(Unknown); break;
    }
    return icon;
}

QString LanguageClientCompletionItem::detail() const
{
    if (auto _doc = m_item.documentation()) {
        auto doc = *_doc;
        QString detailDocText;
        if (std::holds_alternative<QString>(doc)) {
            detailDocText = std::get<QString>(doc);
        } else if (std::holds_alternative<MarkupContent>(doc)) {
            // TODO markdown parser?
            detailDocText = std::get<MarkupContent>(doc).content();
        }
        if (!detailDocText.isEmpty())
            return detailDocText;
    }
    return m_item.detail().value_or(text());
}

bool LanguageClientCompletionItem::isSnippet() const
{
    return m_item.insertTextFormat() == CompletionItem::Snippet;
}

bool LanguageClientCompletionItem::isValid() const
{
    return m_item.isValid();
}

quint64 LanguageClientCompletionItem::hash() const
{
    return qHash(m_item.label()); // TODO: naaaa
}

CompletionItem LanguageClientCompletionItem::item() const
{
    return m_item;
}

QChar LanguageClientCompletionItem::triggeredCommitCharacter() const
{
    return m_triggeredCommitCharacter;
}

const QString &LanguageClientCompletionItem::sortText() const
{
    if (m_sortText.isEmpty())
        m_sortText = m_item.sortText().has_value() ? *m_item.sortText() : m_item.label();
    return m_sortText;
}

bool LanguageClientCompletionItem::hasSortText() const
{
    return m_item.sortText().has_value();
}

QString LanguageClientCompletionItem::filterText() const
{
    if (m_filterText.isEmpty())
        m_filterText = m_item.filterText().value_or(m_item.label());
    return m_filterText;
}

bool LanguageClientCompletionItem::operator <(const LanguageClientCompletionItem &other) const
{
    return sortText() < other.sortText();
}

bool LanguageClientCompletionItem::isPerfectMatch(int pos, QTextDocument *doc) const
{
    QTC_ASSERT(doc, return false);
    using namespace Utils::Text;
    if (auto additionalEdits = m_item.additionalTextEdits()) {
        if (!additionalEdits->isEmpty())
            return false;
    }
    if (isSnippet())
        return false;
    if (auto edit = m_item.textEdit()) {
        auto range = edit->range();
        const int start = positionInText(doc, range.start().line() + 1, range.start().character() + 1);
        const int end = positionInText(doc, range.end().line() + 1, range.end().character() + 1);
        auto text = textAt(QTextCursor(doc), start, end - start);
        return text == edit->newText();
    }
    const QString textToInsert(m_item.insertText().value_or(text()));
    const int length = textToInsert.length();
    return textToInsert == textAt(QTextCursor(doc), pos - length, length);
}

bool LanguageClientCompletionItem::isDeprecated() const
{
    if (const auto tags = m_item.tags(); tags && tags->contains(CompletionItem::Deprecated))
        return true;
    if (const auto deprecated = m_item.deprecated())
        return *deprecated;
    return false;
}

class LanguageClientCompletionModel : public GenericProposalModel
{
public:
    // GenericProposalModel interface
    bool containsDuplicates() const override { return false; }
    bool isSortable(const QString &/*prefix*/) const override;
    void sort(const QString &prefix) override;
    bool supportsPrefixExpansion() const override { return false; }

    QList<AssistProposalItemInterface *> items() const { return m_currentItems; }
};

bool LanguageClientCompletionModel::isSortable(const QString &) const
{
    return Utils::anyOf(items(), [](AssistProposalItemInterface *i) {
        auto item = dynamic_cast<LanguageClientCompletionItem *>(i);
        return !item || item->hasSortText();
    });
}

void LanguageClientCompletionModel::sort(const QString &prefix)
{
    std::sort(m_currentItems.begin(), m_currentItems.end(),
              [&prefix] (AssistProposalItemInterface *a, AssistProposalItemInterface *b){
        const auto lca = dynamic_cast<LanguageClientCompletionItem *>(a);
        const auto lcb = dynamic_cast<LanguageClientCompletionItem *>(b);
        if (!lca && !lcb)
            return a->text() < b->text();
        if (lca && lcb)
            return *lca < *lcb;
        if (prefix.isEmpty())
            return lca && !lcb;
        if (!lca)
            return a->text().toLower().startsWith(prefix.toLower());
        return !b->text().toLower().startsWith(prefix.toLower());
    });
}

class LanguageClientCompletionWidget : public GenericProposalWidget
{
public:
    LanguageClientCompletionWidget(const IAssistProvider *provider)
        : m_provider(provider)
    {}

    ~LanguageClientCompletionWidget() { deleteCurrentProcessor(); }

    void deleteCurrentProcessor()
    {
        if (m_processor) {
            m_processor->cancel();
            delete m_processor;
            m_processor = nullptr;
        }
    }

    void setProposal(IAssistProposal *proposal, const QString &prefix)
    {
        if (!proposal) {
            // Close the proposal if we have no running processor otherwise ignore the empty
            // proposal and wait for the processor to finish
            if (!m_processor || !m_processor->running())
                closeProposal();
            return;
        }
        if (proposal->id() != TextEditor::Constants::GENERIC_PROPOSAL_ID) {
            // We received something else than a generic proposal so we cannot update the model
            closeProposal();
            return;
        }
        updateModel(proposal->model().staticCast<GenericProposalModel>(), prefix);
        delete proposal;
    }

    void updateProposal(std::unique_ptr<AssistInterface> &&interface) override
    {
        deleteCurrentProcessor();
        if (!m_provider) {
            GenericProposalWidget::updateProposal(std::move(interface));
            return;
        }
        m_processor = m_provider->createProcessor(interface.get());
        QTC_ASSERT(m_processor, return);

        const QString prefix = interface->textAt(m_basePosition,
                                                 interface->position() - m_basePosition);

        m_processor->setAsyncCompletionAvailableHandler([this, processor = m_processor, prefix](
                                                            IAssistProposal *proposal) {
            QTC_ASSERT(processor == m_processor, return);
            if (!processor->running()) {
                // do not delete this processor directly since this function is called from within the processor
                QMetaObject::invokeMethod(
                    QCoreApplication::instance(),
                    [processor] { delete processor; },
                    Qt::QueuedConnection);
                m_processor = nullptr;
            }
            setProposal(proposal, prefix);
        });

        setProposal(m_processor->start(std::move(interface)), prefix);
        if (m_processor && !m_processor->running()) {
            delete m_processor;
            m_processor = nullptr;
        }
    }

private:
    QPointer<const IAssistProvider> m_provider;
    std::optional<MessageId> m_currentRequestId;
    IAssistProcessor *m_processor = nullptr;
};

class LanguageClientCompletionProposal : public GenericProposal
{
public:
    LanguageClientCompletionProposal(const IAssistProvider *provider,
                                     int cursorPos,
                                     LanguageClientCompletionModel *model)
        : GenericProposal(cursorPos, GenericProposalModelPtr(model))
        , m_model(model)
        , m_provider(provider)
    {}

    // IAssistProposal interface
    bool hasItemsToPropose(const QString &/*text*/, AssistReason reason) const override
    {
        if (m_model->size() <= 0 || m_document.isNull())
            return false;

        return m_model->keepPerfectMatch(reason)
                || !Utils::anyOf(m_model->items(), [this](AssistProposalItemInterface *item){
            if (const auto lcItem = dynamic_cast<LanguageClientCompletionItem *>(item))
                return lcItem->isPerfectMatch(m_pos, m_document);
            return false;
        });
    }

    IAssistProposalWidget *createWidget() const override
    {
        return new LanguageClientCompletionWidget(m_provider);
    }

    LanguageClientCompletionModel *m_model;
    QPointer<QTextDocument> m_document;
    QPointer<const IAssistProvider> m_provider;
    int m_pos = -1;
};

LanguageClientCompletionAssistProcessor::LanguageClientCompletionAssistProcessor(
    Client *client, const IAssistProvider *provider, const QString &snippetsGroup)
    : m_client(client)
    , m_provider(provider)
    , m_snippetsGroup(snippetsGroup)
{}

LanguageClientCompletionAssistProcessor::~LanguageClientCompletionAssistProcessor()
{
    QTC_ASSERT(!running(), cancel());
}

QTextDocument *LanguageClientCompletionAssistProcessor::document() const
{
    return interface()->textDocument();
}

QList<AssistProposalItemInterface *> LanguageClientCompletionAssistProcessor::generateCompletionItems(
    const QList<LanguageServerProtocol::CompletionItem> &items) const
{
    return Utils::transform<QList<AssistProposalItemInterface *>>(
        items, [](const CompletionItem &item) { return new LanguageClientCompletionItem(item); });
}

static QString assistReasonString(AssistReason reason)
{
    switch (reason) {
    case IdleEditor: return QString("idle editor");
    case ActivationCharacter: return QString("activation character");
    case ExplicitlyInvoked: return QString("explicitly invoking");
    }
    return QString("unknown reason");
}

IAssistProposal *LanguageClientCompletionAssistProcessor::perform()
{
    QTC_ASSERT(m_client, return nullptr);
    m_pos = interface()->position();
    m_basePos = m_pos;
    auto isIdentifierChar = [](const QChar &c) { return c.isLetterOrNumber() || c == '_'; };
    while (m_basePos > 0 && isIdentifierChar(interface()->characterAt(m_basePos - 1)))
        --m_basePos;
    if (interface()->reason() == IdleEditor) {
        // Trigger an automatic completion request only when we are on a word with at least n "identifier" characters
        if (m_pos - m_basePos < TextEditorSettings::completionSettings().m_characterThreshold)
            return nullptr;
        if (m_client->documentUpdatePostponed(interface()->filePath())) {
            m_postponedUpdateConnection
                = QObject::connect(m_client,
                                   &Client::documentUpdated,
                                   [this](TextEditor::TextDocument *document) {
                                       if (document->filePath() == interface()->filePath())
                                           perform();
                                   });
            return nullptr;
        }
    }
    if (m_postponedUpdateConnection)
        QObject::disconnect(m_postponedUpdateConnection);
    CompletionParams::CompletionContext context;
    if (interface()->reason() == ActivationCharacter) {
        context.setTriggerKind(CompletionParams::TriggerCharacter);
        QChar triggerCharacter = interface()->characterAt(interface()->position() - 1);
        if (!triggerCharacter.isNull())
            context.setTriggerCharacter(triggerCharacter);
    } else {
        context.setTriggerKind(CompletionParams::Invoked);
    }
    CompletionParams params;
    int line;
    int column;
    if (!Utils::Text::convertPosition(interface()->textDocument(), m_pos, &line, &column))
        return nullptr;
    --line; // line is 0 based in the protocol
    params.setPosition({line, column});
    params.setContext(context);
    params.setTextDocument(
        TextDocumentIdentifier(m_client->hostPathToServerUri(interface()->filePath())));
    if (const int limit = m_client->completionResultsLimit(); limit >= 0)
        params.setLimit(limit);
    CompletionRequest completionRequest(params);
    completionRequest.setResponseCallback([this](auto response) {
        this->handleCompletionResponse(response);
    });
    m_client->sendMessage(completionRequest);
    m_client->addAssistProcessor(this);
    m_currentRequest = completionRequest.id();
    m_filePath = interface()->filePath();
    qCDebug(LOGLSPCOMPLETION) << QTime::currentTime()
                              << " : request completions at " << m_pos
                              << " by " << assistReasonString(interface()->reason());
    return nullptr;
}

bool LanguageClientCompletionAssistProcessor::running()
{
    return m_currentRequest.has_value() || m_postponedUpdateConnection;
}

void LanguageClientCompletionAssistProcessor::cancel()
{
    if (m_currentRequest.has_value()) {
        if (m_client) {
            m_client->cancelRequest(*m_currentRequest);
            m_client->removeAssistProcessor(this);
        }
        m_currentRequest.reset();
    } else if (m_postponedUpdateConnection) {
        QObject::disconnect(m_postponedUpdateConnection);
    }
}

void LanguageClientCompletionAssistProcessor::handleCompletionResponse(
    const CompletionRequest::Response &response)
{
    // We must report back to the code assistant under all circumstances
    qCDebug(LOGLSPCOMPLETION) << QTime::currentTime() << " : got completions";
    m_currentRequest.reset();
    QTC_ASSERT(m_client, setAsyncProposalAvailable(nullptr); return);
    if (auto error = response.error())
        m_client->log(*error);

    const std::optional<CompletionResult> &result = response.result();
    if (!result || std::holds_alternative<std::nullptr_t>(*result)) {
        setAsyncProposalAvailable(nullptr);
        m_client->removeAssistProcessor(this);
        return;
    }

    const QString prefix = Utils::Text::textAt(QTextCursor(document()),
                                               m_basePos,
                                               m_pos - m_basePos);

    QList<CompletionItem> items;
    if (std::holds_alternative<CompletionList>(*result)) {
        const auto &list = std::get<CompletionList>(*result);
        items = list.items().value_or(QList<CompletionItem>());
    } else if (std::holds_alternative<QList<CompletionItem>>(*result)) {
        items = std::get<QList<CompletionItem>>(*result);
    }
    auto proposalItems = generateCompletionItems(items);
    if (!m_snippetsGroup.isEmpty()) {
        proposalItems << TextEditor::SnippetAssistCollector(m_snippetsGroup,
                                                            QIcon(
                                                                ":/texteditor/images/snippet.png"))
                             .collect();
    }
    auto model = new LanguageClientCompletionModel();
    model->loadContent(proposalItems);
    LanguageClientCompletionProposal *proposal = new LanguageClientCompletionProposal(m_provider,
                                                                                      m_basePos,
                                                                                      model);
    proposal->m_document = document();
    proposal->m_pos = m_pos;
    setAsyncProposalAvailable(proposal);
    m_client->removeAssistProcessor(this);
    qCDebug(LOGLSPCOMPLETION) << QTime::currentTime() << " : "
                              << items.count() << " completions handled";
}

LanguageClientCompletionAssistProvider::LanguageClientCompletionAssistProvider(Client *client)
    : CompletionAssistProvider(client)
    , m_client(client)
{ }

IAssistProcessor *LanguageClientCompletionAssistProvider::createProcessor(
    const AssistInterface *) const
{
    return new LanguageClientCompletionAssistProcessor(m_client, this, m_snippetsGroup);
}

int LanguageClientCompletionAssistProvider::activationCharSequenceLength() const
{
    return m_activationCharSequenceLength;
}

bool LanguageClientCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return Utils::anyOf(m_triggerChars, [sequence](const QString &trigger) {
        return trigger.endsWith(sequence);
    });
}

void LanguageClientCompletionAssistProvider::setTriggerCharacters(
    const std::optional<QList<QString>> triggerChars)
{
    m_activationCharSequenceLength = 0;
    m_triggerChars = triggerChars.value_or(QList<QString>());
    for (const QString &trigger : std::as_const(m_triggerChars)) {
        if (trigger.length() > m_activationCharSequenceLength)
            m_activationCharSequenceLength = trigger.length();
    }
}

} // namespace LanguageClient
