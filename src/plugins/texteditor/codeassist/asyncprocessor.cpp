// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "asyncprocessor.h"

#include "assistinterface.h"
#include "iassistproposal.h"

#include <utils/runextensions.h>

namespace TextEditor {

AsyncProcessor::AsyncProcessor()
{
    QObject::connect(&m_watcher, &QFutureWatcher<IAssistProposal *>::finished, [this]() {
        setAsyncProposalAvailable(m_watcher.result());
    });
}

IAssistProposal *AsyncProcessor::perform(AssistInterface *interface)
{
    IAssistProposal *result = immediateProposal(interface);
    m_interface = interface;
    m_interface->prepareForAsyncUse();
    m_watcher.setFuture(Utils::runAsync([this]() {
        m_interface->recreateTextDocument();
        return performAsync(m_interface);
    }));
    return result;
}

bool AsyncProcessor::running()
{
    return m_watcher.isRunning();
}

void AsyncProcessor::cancel()
{
    setAsyncCompletionAvailableHandler([this](IAssistProposal *proposal){
        delete proposal;
        QMetaObject::invokeMethod(QCoreApplication::instance(), [this] {
            delete this;
        }, Qt::QueuedConnection);
    });
}

IAssistProposal *AsyncProcessor::immediateProposal(AssistInterface *interface)
{
    Q_UNUSED(interface)
    return nullptr;
}

bool AsyncProcessor::isCanceled() const
{
    return m_watcher.isCanceled();
}

} // namespace TextEditor
