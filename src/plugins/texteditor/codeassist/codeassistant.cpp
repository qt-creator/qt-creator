/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "codeassistant.h"
#include "completionassistprovider.h"
#include "quickfixassistprovider.h"
#include "iassistprocessor.h"
#include "textdocument.h"
#include "iassistproposal.h"
#include "iassistproposalmodel.h"
#include "iassistproposalwidget.h"
#include "assistinterface.h"
#include "assistproposalitem.h"
#include "runner.h"
#include "textdocumentmanipulator.h"

#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/completionsettings.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QKeyEvent>
#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QTimer>

using namespace TextEditor::Internal;

namespace TextEditor {

class CodeAssistantPrivate : public QObject
{
public:
    CodeAssistantPrivate(CodeAssistant *assistant);

    void configure(TextEditorWidget *editorWidget);
    bool isConfigured() const;

    void invoke(AssistKind kind, IAssistProvider *provider = 0);
    void process();
    void requestProposal(AssistReason reason, AssistKind kind, IAssistProvider *provider = 0);
    void cancelCurrentRequest();
    void invalidateCurrentRequestData();
    void displayProposal(IAssistProposal *newProposal, AssistReason reason);
    bool isDisplayingProposal() const;
    bool isWaitingForProposal() const;

    void notifyChange();
    bool hasContext() const;
    void destroyContext();

    CompletionAssistProvider *identifyActivationSequence();

    void stopAutomaticProposalTimer();
    void startAutomaticProposalTimer();
    void automaticProposalTimeout();
    void clearAbortedPosition();
    void updateFromCompletionSettings(const TextEditor::CompletionSettings &settings);

    virtual bool eventFilter(QObject *o, QEvent *e);

private:
    void processProposalItem(AssistProposalItemInterface *proposalItem);
    void handlePrefixExpansion(const QString &newPrefix);
    void finalizeProposal();
    void explicitlyAborted();

private:
    CodeAssistant *q;
    TextEditorWidget *m_editorWidget;
    Internal::ProcessorRunner *m_requestRunner;
    QMetaObject::Connection m_runnerConnection;
    IAssistProvider *m_requestProvider;
    IAssistProcessor *m_asyncProcessor;
    AssistKind m_assistKind;
    IAssistProposalWidget *m_proposalWidget;
    QScopedPointer<IAssistProposal> m_proposal;
    bool m_receivedContentWhileWaiting;
    QTimer m_automaticProposalTimer;
    CompletionSettings m_settings;
    int m_abortedBasePosition;
    static const QChar m_null;
};

// --------------------
// CodeAssistantPrivate
// --------------------
const QChar CodeAssistantPrivate::m_null;

CodeAssistantPrivate::CodeAssistantPrivate(CodeAssistant *assistant)
    : q(assistant)
    , m_editorWidget(0)
    , m_requestRunner(0)
    , m_requestProvider(0)
    , m_asyncProcessor(0)
    , m_assistKind(TextEditor::Completion)
    , m_proposalWidget(0)
    , m_receivedContentWhileWaiting(false)
    , m_abortedBasePosition(-1)
{
    m_automaticProposalTimer.setSingleShot(true);
    connect(&m_automaticProposalTimer, &QTimer::timeout,
            this, &CodeAssistantPrivate::automaticProposalTimeout);

    m_settings = TextEditorSettings::completionSettings();
    connect(TextEditorSettings::instance(), &TextEditorSettings::completionSettingsChanged,
            this, &CodeAssistantPrivate::updateFromCompletionSettings);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &CodeAssistantPrivate::clearAbortedPosition);
}

void CodeAssistantPrivate::configure(TextEditorWidget *editorWidget)
{
    m_editorWidget = editorWidget;
    m_editorWidget->installEventFilter(this);
}

bool CodeAssistantPrivate::isConfigured() const
{
    return m_editorWidget != 0;
}

void CodeAssistantPrivate::invoke(AssistKind kind, IAssistProvider *provider)
{
    if (!isConfigured())
        return;

    stopAutomaticProposalTimer();

    if (isDisplayingProposal() && m_assistKind == kind && !m_proposal->isFragile()) {
        m_proposalWidget->setReason(ExplicitlyInvoked);
        m_proposalWidget->updateProposal(m_editorWidget->textAt(
                        m_proposal->basePosition(),
                        m_editorWidget->position() - m_proposal->basePosition()));
    } else {
        destroyContext();
        requestProposal(ExplicitlyInvoked, kind, provider);
    }
}

void CodeAssistantPrivate::process()
{
    if (!isConfigured())
        return;

    stopAutomaticProposalTimer();

    if (m_assistKind == TextEditor::Completion) {
        if (m_settings.m_completionTrigger != ManualCompletion) {
            if (CompletionAssistProvider *provider = identifyActivationSequence()) {
                if (isWaitingForProposal())
                    cancelCurrentRequest();
                requestProposal(ActivationCharacter, Completion, provider);
                return;
            }
        }

        if (!isDisplayingProposal())
            startAutomaticProposalTimer();
    } else {
        m_assistKind = TextEditor::Completion;
    }
}

void CodeAssistantPrivate::requestProposal(AssistReason reason,
                                           AssistKind kind,
                                           IAssistProvider *provider)
{
    QTC_ASSERT(!isWaitingForProposal(), return);

    if (m_editorWidget->hasBlockSelection())
        return; // TODO

    if (!provider) {
        if (kind == Completion)
            provider = m_editorWidget->textDocument()->completionAssistProvider();
        else
            provider = m_editorWidget->textDocument()->quickFixAssistProvider();

        if (!provider)
            return;
    }

    AssistInterface *assistInterface = m_editorWidget->createAssistInterface(kind, reason);
    if (!assistInterface)
        return;

    m_assistKind = kind;
    IAssistProcessor *processor = provider->createProcessor();

    switch (provider->runType()) {
    case IAssistProvider::Synchronous: {
        if (IAssistProposal *newProposal = processor->perform(assistInterface))
            displayProposal(newProposal, reason);
        delete processor;
        break;
    }
    case IAssistProvider::AsynchronousWithThread: {
        if (IAssistProposal *newProposal = processor->immediateProposal(assistInterface))
            displayProposal(newProposal, reason);

        m_requestProvider = provider;
        m_requestRunner = new ProcessorRunner;
        m_runnerConnection = connect(m_requestRunner, &ProcessorRunner::finished,
                                     this, [this, reason](){
            // Since the request runner is a different thread, there's still a gap in which the
            // queued signal could be processed after an invalidation of the current request.
            if (!m_requestRunner || m_requestRunner != sender())
                return;

            IAssistProposal *proposal = m_requestRunner->proposal();
            invalidateCurrentRequestData();
            displayProposal(proposal, reason);
            emit q->finished();
        });
        connect(m_requestRunner, &ProcessorRunner::finished,
                m_requestRunner, &ProcessorRunner::deleteLater);
        assistInterface->prepareForAsyncUse();
        m_requestRunner->setProcessor(processor);
        m_requestRunner->setAssistInterface(assistInterface);
        m_requestRunner->start();
        break;
    }
    case IAssistProvider::Asynchronous: {
        processor->setAsyncCompletionAvailableHandler(
            [this, processor, reason](IAssistProposal *newProposal){
                QTC_CHECK(newProposal);
                invalidateCurrentRequestData();
                displayProposal(newProposal, reason);

                emit q->finished();
        });

        // If there is a proposal, nothing asynchronous happened...
        if (IAssistProposal *newProposal = processor->perform(assistInterface)) {
            displayProposal(newProposal, reason);
            delete processor;
        } else if (!processor->performWasApplicable()) {
            delete processor;
        } else { // ...async request was triggered
            m_asyncProcessor = processor;
        }

        break;
    }
    } // switch
}

void CodeAssistantPrivate::cancelCurrentRequest()
{
    if (m_requestRunner) {
        m_requestRunner->setDiscardProposal(true);
        disconnect(m_runnerConnection);
    }
    invalidateCurrentRequestData();
}

void CodeAssistantPrivate::displayProposal(IAssistProposal *newProposal, AssistReason reason)
{
    if (!newProposal)
        return;

    QScopedPointer<IAssistProposal> proposalCandidate(newProposal);

    if (isDisplayingProposal()) {
        if (!m_proposal->isFragile())
            return;
        destroyContext();
    }

    int basePosition = proposalCandidate->basePosition();
    if (m_editorWidget->position() < basePosition)
        return;

    if (m_abortedBasePosition == basePosition && reason != ExplicitlyInvoked)
        return;

    clearAbortedPosition();
    m_proposal.reset(proposalCandidate.take());

    if (m_proposal->isCorrective(m_editorWidget))
        m_proposal->makeCorrection(m_editorWidget);

    m_editorWidget->keepAutoCompletionHighlight(true);
    basePosition = m_proposal->basePosition();
    m_proposalWidget = m_proposal->createWidget();
    connect(m_proposalWidget, &QObject::destroyed,
            this, &CodeAssistantPrivate::finalizeProposal);
    connect(m_proposalWidget, &IAssistProposalWidget::prefixExpanded,
            this, &CodeAssistantPrivate::handlePrefixExpansion);
    connect(m_proposalWidget, &IAssistProposalWidget::proposalItemActivated,
            this, &CodeAssistantPrivate::processProposalItem);
    connect(m_proposalWidget, &IAssistProposalWidget::explicitlyAborted,
            this, &CodeAssistantPrivate::explicitlyAborted);
    m_proposalWidget->setAssistant(q);
    m_proposalWidget->setReason(reason);
    m_proposalWidget->setKind(m_assistKind);
    m_proposalWidget->setUnderlyingWidget(m_editorWidget);
    m_proposalWidget->setModel(m_proposal->model());
    m_proposalWidget->setDisplayRect(m_editorWidget->cursorRect(basePosition));
    m_proposalWidget->setIsSynchronized(!m_receivedContentWhileWaiting);
    m_proposalWidget->showProposal(m_editorWidget->textAt(
                                       basePosition,
                                       m_editorWidget->position() - basePosition));
}

void CodeAssistantPrivate::processProposalItem(AssistProposalItemInterface *proposalItem)
{
    QTC_ASSERT(m_proposal, return);
    TextDocumentManipulator manipulator(m_editorWidget);
    proposalItem->apply(manipulator, m_proposal->basePosition());
    destroyContext();
    process();
}

void CodeAssistantPrivate::handlePrefixExpansion(const QString &newPrefix)
{
    QTC_ASSERT(m_proposal, return);
    const int currentPosition = m_editorWidget->position();
    m_editorWidget->setCursorPosition(m_proposal->basePosition());
    m_editorWidget->replace(currentPosition - m_proposal->basePosition(), newPrefix);
    notifyChange();
}

void CodeAssistantPrivate::finalizeProposal()
{
    stopAutomaticProposalTimer();
    m_proposal.reset();
    m_proposalWidget = 0;
    if (m_receivedContentWhileWaiting)
        m_receivedContentWhileWaiting = false;
}

bool CodeAssistantPrivate::isDisplayingProposal() const
{
    return m_proposalWidget != 0;
}

bool CodeAssistantPrivate::isWaitingForProposal() const
{
    return m_requestRunner != 0 || m_asyncProcessor != 0;
}

void CodeAssistantPrivate::invalidateCurrentRequestData()
{
    m_asyncProcessor = 0;
    m_requestRunner = 0;
    m_requestProvider = 0;
}

CompletionAssistProvider *CodeAssistantPrivate::identifyActivationSequence()
{
    CompletionAssistProvider *completionProvider = m_editorWidget->textDocument()->completionAssistProvider();
    if (!completionProvider)
        return 0;

    const int length = completionProvider->activationCharSequenceLength();
    if (length == 0)
        return 0;
    QString sequence = m_editorWidget->textAt(m_editorWidget->position() - length, length);
    // In pretty much all cases the sequence will have the appropriate length. Only in the
    // case of typing the very first characters in the document for providers that request a
    // length greater than 1 (currently only C++, which specifies 3), the sequence needs to
    // be prepended so it has the expected length.
    const int lengthDiff = length - sequence.length();
    for (int j = 0; j < lengthDiff; ++j)
        sequence.prepend(m_null);
    return completionProvider->isActivationCharSequence(sequence) ? completionProvider : 0;
}

void CodeAssistantPrivate::notifyChange()
{
    stopAutomaticProposalTimer();

    if (isDisplayingProposal()) {
        QTC_ASSERT(m_proposal, return);
        if (m_editorWidget->position() < m_proposal->basePosition()) {
            destroyContext();
        } else {
            m_proposalWidget->updateProposal(
                m_editorWidget->textAt(m_proposal->basePosition(),
                                     m_editorWidget->position() - m_proposal->basePosition()));
        }
    }
}

bool CodeAssistantPrivate::hasContext() const
{
    return m_requestRunner || m_asyncProcessor || m_proposalWidget;
}

void CodeAssistantPrivate::destroyContext()
{
    stopAutomaticProposalTimer();

    if (isWaitingForProposal()) {
        cancelCurrentRequest();
    } else if (isDisplayingProposal()) {
        m_editorWidget->keepAutoCompletionHighlight(false);
        m_proposalWidget->closeProposal();
        disconnect(m_proposalWidget, &QObject::destroyed,
                   this, &CodeAssistantPrivate::finalizeProposal);
        finalizeProposal();
    }
}

void CodeAssistantPrivate::startAutomaticProposalTimer()
{
    if (m_settings.m_completionTrigger == AutomaticCompletion)
        m_automaticProposalTimer.start();
}

void CodeAssistantPrivate::automaticProposalTimeout()
{
    if (isWaitingForProposal() || isDisplayingProposal())
        return;

    requestProposal(IdleEditor, Completion);
}

void CodeAssistantPrivate::stopAutomaticProposalTimer()
{
    if (m_automaticProposalTimer.isActive())
        m_automaticProposalTimer.stop();
}

void CodeAssistantPrivate::updateFromCompletionSettings(
        const TextEditor::CompletionSettings &settings)
{
    m_settings = settings;
    m_automaticProposalTimer.setInterval(m_settings.m_automaticProposalTimeoutInMs);
}

void CodeAssistantPrivate::explicitlyAborted()
{
    QTC_ASSERT(m_proposal, return);
    m_abortedBasePosition = m_proposal->basePosition();
}

void CodeAssistantPrivate::clearAbortedPosition()
{
    m_abortedBasePosition = -1;
}

bool CodeAssistantPrivate::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);

    if (isWaitingForProposal()) {
        QEvent::Type type = e->type();
        if (type == QEvent::FocusOut) {
            destroyContext();
        } else if (type == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            const QString &keyText = keyEvent->text();

            CompletionAssistProvider *completionProvider = 0;
            if ((keyText.isEmpty()
                 && keyEvent->key() != Qt::LeftArrow
                 && keyEvent->key() != Qt::RightArrow
                 && keyEvent->key() != Qt::Key_Shift)
                || (!keyText.isEmpty()
                    && (((completionProvider = dynamic_cast<CompletionAssistProvider *>(m_requestProvider))
                            ? !completionProvider->isContinuationChar(keyText.at(0))
                            : false)))) {
                destroyContext();
            } else if (!keyText.isEmpty() && !m_receivedContentWhileWaiting) {
                m_receivedContentWhileWaiting = true;
            }
        }
    }

    return false;
}

// -------------
// CodeAssistant
// -------------
CodeAssistant::CodeAssistant() : d(new CodeAssistantPrivate(this))
{
}

CodeAssistant::~CodeAssistant()
{
    destroyContext();
    delete d;
}

void CodeAssistant::configure(TextEditorWidget *editorWidget)
{
    d->configure(editorWidget);
}

void CodeAssistant::process()
{
    d->process();
}

void CodeAssistant::notifyChange()
{
    d->notifyChange();
}

bool CodeAssistant::hasContext() const
{
    return d->hasContext();
}

void CodeAssistant::destroyContext()
{
    d->destroyContext();
}

void CodeAssistant::invoke(AssistKind kind, IAssistProvider *provider)
{
    d->invoke(kind, provider);
}

} // namespace TextEditor
