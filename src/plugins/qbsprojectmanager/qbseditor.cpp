// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbseditor.h"

#include "qbslanguageclient.h"
#include "qbsprojectmanagertr.h"

#include <languageclient/languageclientcompletionassist.h>
#include <languageclient/languageclientmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <qmljseditor/qmljscompletionassist.h>
#include <texteditor/codeassist/genericproposal.h>
#include <utils/utilsicons.h>
#include <utils/mimeconstants.h>

#include <QPointer>

#include <memory>
#include <optional>

using namespace LanguageClient;
using namespace QmlJSEditor;
using namespace TextEditor;
using namespace Utils;

namespace QbsProjectManager::Internal {

class QbsEditorWidget : public QmlJSEditorWidget
{
private:
    void findLinkAt(const QTextCursor &cursor,
                    const LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;
};

class QbsCompletionAssistProcessor : public LanguageClientCompletionAssistProcessor
{
public:
    QbsCompletionAssistProcessor(Client *client);

private:
    QList<AssistProposalItemInterface *> generateCompletionItems(
        const QList<LanguageServerProtocol::CompletionItem> &items) const override;
};

class MergedCompletionAssistProcessor : public IAssistProcessor
{
public:
    MergedCompletionAssistProcessor(const AssistInterface *interface) : m_interface(interface) {}
    ~MergedCompletionAssistProcessor();

private:
    IAssistProposal *perform() override;
    bool running() override { return m_started && (!m_qmlProposal || !m_qbsProposal); }
    void checkFinished();

    const AssistInterface * const m_interface;
    std::unique_ptr<IAssistProcessor> m_qmlProcessor;
    std::unique_ptr<IAssistProcessor> m_qbsProcessor;
    std::optional<IAssistProposal *> m_qmlProposal;
    std::optional<IAssistProposal *> m_qbsProposal;
    bool m_started = false;
};

class QbsCompletionAssistProvider : public QmlJSCompletionAssistProvider
{
private:
    IAssistProcessor *createProcessor(const AssistInterface *interface) const override
    {
        return new MergedCompletionAssistProcessor(interface);
    }
};

class QbsCompletionItem : public LanguageClientCompletionItem
{
public:
    using LanguageClientCompletionItem::LanguageClientCompletionItem;

private:
    QIcon icon() const override;
};

class MergedProposalModel : public GenericProposalModel
{
public:
    MergedProposalModel(const QList<GenericProposalModelPtr> &sourceModels);
};

static Client *clientForDocument(const TextDocument *doc)
{
    if (!doc)
        return nullptr;
    const QList<Client *> &candidates = LanguageClientManager::clientsSupportingDocument(doc);
    for (Client * const candidate : candidates) {
        if (const auto qbsClient = qobject_cast<QbsLanguageClient *>(candidate);
            qbsClient && qbsClient->isActive() && qbsClient->documentOpen(doc)) {
            return qbsClient;
        }
    }
    return nullptr;
}

QbsEditorFactory::QbsEditorFactory() : QmlJSEditorFactory("QbsEditor.QbsEditor")
{
    setDisplayName(Tr::tr("Qbs Editor"));
    setMimeTypes({Utils::Constants::QBS_MIMETYPE});
    setEditorWidgetCreator([] { return new QbsEditorWidget; });
    setCompletionAssistProvider(new QbsCompletionAssistProvider);
}

void QbsEditorWidget::findLinkAt(const QTextCursor &cursor, const LinkHandler &processLinkCallback,
                                 bool resolveTarget, bool inNextSplit)
{
    const LinkHandler extendedCallback = [self = QPointer(this), cursor, processLinkCallback,
                                          resolveTarget](const Link &link) {
        if (link.hasValidTarget())
            return processLinkCallback(link);
        if (!self)
            return;
        const auto doc = self->textDocument();
        if (Client * const client = clientForDocument(doc)) {
            client->findLinkAt(doc, cursor, processLinkCallback, resolveTarget,
                               LinkTarget::SymbolDef);
        }
    };
    QmlJSEditorWidget::findLinkAt(cursor, extendedCallback, resolveTarget, inNextSplit);
}

MergedCompletionAssistProcessor::~MergedCompletionAssistProcessor()
{
    if (m_qmlProposal)
        delete *m_qmlProposal;
    if (m_qbsProposal)
        delete *m_qbsProposal;
}

IAssistProposal *MergedCompletionAssistProcessor::perform()
{
    m_started = true;
    if (Client *const qbsClient = clientForDocument(
            TextDocument::textDocumentForFilePath(m_interface->filePath()))) {
        m_qbsProcessor.reset(new QbsCompletionAssistProcessor(qbsClient));
        m_qbsProcessor->setAsyncCompletionAvailableHandler([this](IAssistProposal *proposal) {
            m_qbsProposal = proposal;
            checkFinished();
        });
        m_qbsProcessor->start(std::make_unique<AssistInterface>(m_interface->cursor(),
                                                                m_interface->filePath(),
                                                                m_interface->reason()));
    } else {
        m_qbsProposal = nullptr;
    }
    m_qmlProcessor.reset(QmlJSCompletionAssistProvider().createProcessor(m_interface));
    m_qmlProcessor->setAsyncCompletionAvailableHandler([this](IAssistProposal *proposal) {
        m_qmlProposal = proposal;
        checkFinished();
    });
    const auto qmlJsIface = static_cast<const QmlJSCompletionAssistInterface *>(m_interface);
    return m_qmlProcessor->start(
        std::make_unique<QmlJSCompletionAssistInterface>(qmlJsIface->cursor(),
                                                         qmlJsIface->filePath(),
                                                         m_interface->reason(),
                                                         qmlJsIface->semanticInfo()));
}

void MergedCompletionAssistProcessor::checkFinished()
{
    if (running())
        return;

    QList<GenericProposalModelPtr> sourceModels;
    int basePosition = -1;
    for (const IAssistProposal * const proposal : {*m_qmlProposal, *m_qbsProposal}) {
        if (proposal) {
            if (const auto model = proposal->model().dynamicCast<GenericProposalModel>())
                sourceModels << model;
            if (basePosition == -1)
                basePosition = proposal->basePosition();
            else
                QTC_CHECK(basePosition == proposal->basePosition());
        }
    }
    setAsyncProposalAvailable(
        new GenericProposal(basePosition >= 0 ? basePosition : m_interface->position(),
                            GenericProposalModelPtr(new MergedProposalModel(sourceModels))));
}

MergedProposalModel::MergedProposalModel(const QList<GenericProposalModelPtr> &sourceModels)
{
    QList<AssistProposalItemInterface *> items;
    for (const GenericProposalModelPtr &model : sourceModels) {
        items << model->originalItems();
        model->loadContent({});
    }
    loadContent(items);
}

QbsCompletionAssistProcessor::QbsCompletionAssistProcessor(Client *client)
    : LanguageClientCompletionAssistProcessor(client, nullptr, {})
{}

QList<AssistProposalItemInterface *> QbsCompletionAssistProcessor::generateCompletionItems(
    const QList<LanguageServerProtocol::CompletionItem> &items) const
{
    return Utils::transform<QList<AssistProposalItemInterface *>>(
        items, [](const LanguageServerProtocol::CompletionItem &item) {
            return new QbsCompletionItem(item);
        });
}

QIcon QbsCompletionItem::icon() const
{
    if (!item().detail()) {
        return ProjectExplorer::DirectoryIcon(
                   ProjectExplorer::Constants::FILEOVERLAY_MODULES).icon();
    }
    return CodeModelIcon::iconForType(CodeModelIcon::Property);
}

} // namespace QbsProjectManager::Internal
