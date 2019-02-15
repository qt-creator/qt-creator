/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "languageclientcompletionassist.h"

#include "client.h"
#include "languageclientutils.h"

#include <languageserverprotocol/completion.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <utils/algorithm.h>
#include <utils/textutils.h>
#include <utils/utilsicons.h>

#include <QDebug>
#include <QLoggingCategory>
#include <QRegExp>
#include <QTextBlock>
#include <QTextDocument>
#include <QTime>

static Q_LOGGING_CATEGORY(LOGLSPCOMPLETION, "qtc.languageclient.completion", QtWarningMsg);

using namespace LanguageServerProtocol;
using namespace TextEditor;

namespace LanguageClient {

class LanguageClientCompletionItem : public AssistProposalItemInterface
{
public:
    LanguageClientCompletionItem(CompletionItem item);

    // AssistProposalItemInterface interface
    QString text() const override;
    bool implicitlyApplies() const override;
    bool prematurelyApplies(const QChar &typedCharacter) const override;
    void apply(TextDocumentManipulatorInterface &manipulator, int basePosition) const override;
    QIcon icon() const override;
    QString detail() const override;
    bool isSnippet() const override;
    bool isValid() const override;
    quint64 hash() const override;

    const QString &sortText() const;

    bool operator <(const LanguageClientCompletionItem &other) const;

    bool isPerfectMatch(int pos, QTextDocument *doc) const;

private:
    CompletionItem m_item;
    mutable QString m_sortText;
};

LanguageClientCompletionItem::LanguageClientCompletionItem(CompletionItem item)
    : m_item(std::move(item))
{ }

QString LanguageClientCompletionItem::text() const
{ return m_item.label(); }

bool LanguageClientCompletionItem::implicitlyApplies() const
{ return false; }

bool LanguageClientCompletionItem::prematurelyApplies(const QChar &/*typedCharacter*/) const
{ return false; }

void LanguageClientCompletionItem::apply(TextDocumentManipulatorInterface &manipulator,
                                         int /*basePosition*/) const
{
    const int pos = manipulator.currentPosition();
    if (auto edit = m_item.textEdit()) {
        applyTextEdit(manipulator, *edit);
    } else {
        const QString textToInsert(m_item.insertText().value_or(text()));
        int length = 0;
        for (auto it = textToInsert.crbegin(); it != textToInsert.crend(); ++it) {
            auto ch = *it;
            if (ch == manipulator.characterAt(pos - length - 1))
                ++length;
            else if (length != 0)
                length = 0;
        }
        manipulator.replace(pos - length, length, textToInsert);
    }

    if (auto additionalEdits = m_item.additionalTextEdits()) {
        for (const auto &edit : *additionalEdits)
            applyTextEdit(manipulator, edit);
    }
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
    case CompletionItemKind::Field: icon = iconForType(VarPublic); break;
    case CompletionItemKind::Variable: icon = iconForType(VarPublic); break;
    case CompletionItemKind::Class: icon = iconForType(Class); break;
    case CompletionItemKind::Module: icon = iconForType(Namespace); break;
    case CompletionItemKind::Property: icon = iconForType(Property); break;
    case CompletionItemKind::Enum: icon = iconForType(Enum); break;
    case CompletionItemKind::Keyword: icon = iconForType(Keyword); break;
    case CompletionItemKind::Snippet: icon = QIcon(":/texteditor/images/snippet.png"); break;
    case CompletionItemKind::EnumMember: icon = iconForType(Enumerator); break;
    case CompletionItemKind::Struct: icon = iconForType(Struct); break;
    default:
        break;
    }
    return icon;
}

QString LanguageClientCompletionItem::detail() const
{
    if (auto _doc = m_item.documentation()) {
        auto doc = *_doc;
        QString detailDocText;
        if (Utils::holds_alternative<QString>(doc)) {
            detailDocText = Utils::get<QString>(doc);
        } else if (Utils::holds_alternative<MarkupContent>(doc)) {
            // TODO markdown parser?
            detailDocText = Utils::get<MarkupContent>(doc).content();
        }
        if (!detailDocText.isEmpty())
            return detailDocText;
    }
    return m_item.detail().value_or(text());
}

bool LanguageClientCompletionItem::isSnippet() const
{
    // FIXME add lsp > creator snippet converter
    // return m_item.insertTextFormat().value_or(CompletionItem::PlainText);
    return false;
}

bool LanguageClientCompletionItem::isValid() const
{
    return m_item.isValid(nullptr);
}

quint64 LanguageClientCompletionItem::hash() const
{
    return qHash(m_item.label()); // TODO: naaaa
}

const QString &LanguageClientCompletionItem::sortText() const
{
    if (m_sortText.isEmpty())
        m_sortText = m_item.sortText().has_value() ? *m_item.sortText() : m_item.label();
    return m_sortText;
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
        if (!additionalEdits.value().isEmpty())
            return false;
    }
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

class LanguageClientCompletionModel : public GenericProposalModel
{
public:
    // GenericProposalModel interface
    bool isSortable(const QString &/*prefix*/) const override { return true; }
    void sort(const QString &/*prefix*/) override;
    bool supportsPrefixExpansion() const override { return false; }

    QList<LanguageClientCompletionItem *> items() const
    { return Utils::static_container_cast<LanguageClientCompletionItem *>(m_currentItems); }
};

void LanguageClientCompletionModel::sort(const QString &/*prefix*/)
{
    std::sort(m_currentItems.begin(), m_currentItems.end(),
              [] (AssistProposalItemInterface *a, AssistProposalItemInterface *b){
        return *(dynamic_cast<LanguageClientCompletionItem *>(a)) < *(
                    dynamic_cast<LanguageClientCompletionItem *>(b));
    });
}

class LanguageClientCompletionProposal : public GenericProposal
{
public:
    LanguageClientCompletionProposal(int cursorPos, LanguageClientCompletionModel *model)
        : GenericProposal(cursorPos, GenericProposalModelPtr(model))
        , m_model(model)
    { }

    // IAssistProposal interface
    bool hasItemsToPropose(const QString &/*text*/, AssistReason reason) const override
    {
        if (m_model->size() <= 0 || m_document.isNull())
            return false;

        return m_model->keepPerfectMatch(reason)
                || !Utils::anyOf(m_model->items(), [this](LanguageClientCompletionItem *item){
            return item->isPerfectMatch(m_pos, m_document);
        });
    }

    LanguageClientCompletionModel *m_model;
    QPointer<QTextDocument> m_document;
    int m_pos = -1;
};


class LanguageClientCompletionAssistProcessor : public IAssistProcessor
{
public:
    LanguageClientCompletionAssistProcessor(Client *client);
    IAssistProposal *perform(const AssistInterface *interface) override;
    bool running() override;
    bool needsRestart() const override { return true; }

private:
    void handleCompletionResponse(const CompletionRequest::Response &response);

    QPointer<QTextDocument> m_document;
    QPointer<Client> m_client;
    bool m_running = false;
    int m_pos = -1;
};

LanguageClientCompletionAssistProcessor::LanguageClientCompletionAssistProcessor(Client *client)
    : m_client(client)
{ }

static QString assistReasonString(AssistReason reason)
{
    switch (reason) {
    case IdleEditor: return QString("idle editor");
    case ActivationCharacter: return QString("activation character");
    case ExplicitlyInvoked: return QString("explicitly invoking");
    }
    return QString("unknown reason");
}

IAssistProposal *LanguageClientCompletionAssistProcessor::perform(const AssistInterface *interface)
{
    QTC_ASSERT(m_client, return nullptr);
    m_pos = interface->position();
    if (interface->reason() == IdleEditor) {
        // Trigger an automatic completion request only when we are on a word with more than 2 "identifier" character
        const QRegExp regexp("[_a-zA-Z0-9]*");
        int delta = 0;
        while (m_pos - delta > 0 && regexp.exactMatch(interface->textAt(m_pos - delta - 1, delta + 1)))
            ++delta;
        if (delta < 3)
            return nullptr;
    }
    CompletionRequest completionRequest;
    CompletionParams::CompletionContext context;
    context.setTriggerKind(interface->reason() == ActivationCharacter
                           ? CompletionParams::TriggerCharacter
                           : CompletionParams::Invoked);
    auto params = completionRequest.params().value_or(CompletionParams());
    int line;
    int column;
    if (!Utils::Text::convertPosition(interface->textDocument(), m_pos, &line, &column))
        return nullptr;
    --line; // line is 0 based in the protocol
    --column; // column is 0 based in the protocol
    params.setPosition({line, column});
    params.setTextDocument(
                DocumentUri::fromFileName(Utils::FileName::fromString(interface->fileName())));
    completionRequest.setResponseCallback([this](auto response) {
        this->handleCompletionResponse(response);
    });
    completionRequest.setParams(params);
    m_client->sendContent(completionRequest);
    m_running = true;
    m_document = interface->textDocument();
    qCDebug(LOGLSPCOMPLETION) << QTime::currentTime()
                              << " : request completions at " << m_pos
                              << " by " << assistReasonString(interface->reason());
    return nullptr;
}

bool LanguageClientCompletionAssistProcessor::running()
{
    return m_running;
}

void LanguageClientCompletionAssistProcessor::handleCompletionResponse(
    const CompletionRequest::Response &response)
{
    qCDebug(LOGLSPCOMPLETION) << QTime::currentTime() << " : got completions";
    m_running = false;
    QTC_ASSERT(m_client, return);
    if (auto error = response.error()) {
        m_client->log(error.value());
        return;
    }
    const Utils::optional<CompletionResult> &result = response.result();
    if (!result || Utils::holds_alternative<std::nullptr_t>(*result))
        return;

    QList<CompletionItem> items;
    if (Utils::holds_alternative<CompletionList>(*result)) {
        const auto &list = Utils::get<CompletionList>(*result);
        items = list.items().value_or(QList<CompletionItem>());
    } else if (Utils::holds_alternative<QList<CompletionItem>>(*result)) {
        items = Utils::get<QList<CompletionItem>>(*result);
    }
    auto model = new LanguageClientCompletionModel();
    model->loadContent(Utils::transform(items, [](const CompletionItem &item){
        return static_cast<AssistProposalItemInterface *>(new LanguageClientCompletionItem(item));
    }));
    auto proposal = new LanguageClientCompletionProposal(m_pos, model);
    proposal->m_document = m_document;
    proposal->m_pos = m_pos;
    proposal->setFragile(true);
    proposal->setSupportsPrefix(false);
    setAsyncProposalAvailable(proposal);
    qCDebug(LOGLSPCOMPLETION) << QTime::currentTime() << " : "
                              << items.count() << " completions handled";
}

LanguageClientCompletionAssistProvider::LanguageClientCompletionAssistProvider(Client *client)
    : m_client(client)
{ }

IAssistProcessor *LanguageClientCompletionAssistProvider::createProcessor() const
{
    return new LanguageClientCompletionAssistProcessor(m_client);
}

IAssistProvider::RunType LanguageClientCompletionAssistProvider::runType() const
{
    return IAssistProvider::Asynchronous;
}

int LanguageClientCompletionAssistProvider::activationCharSequenceLength() const
{
    return m_activationCharSequenceLength;
}

bool LanguageClientCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return Utils::anyOf(m_triggerChars, [sequence](const QString &trigger){
        return trigger.endsWith(sequence);
    });
}

void LanguageClientCompletionAssistProvider::setTriggerCharacters(QList<QString> triggerChars)
{
    m_triggerChars = triggerChars;
    for (const QString &trigger : triggerChars) {
        if (trigger.length() > m_activationCharSequenceLength)
            m_activationCharSequenceLength = trigger.length();
    }
}

} // namespace LanguageClient
