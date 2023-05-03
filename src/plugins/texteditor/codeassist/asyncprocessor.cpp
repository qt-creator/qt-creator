// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "asyncprocessor.h"

#include "assistinterface.h"
#include "iassistproposal.h"

#include <utils/async.h>

namespace TextEditor {

AsyncProcessor::AsyncProcessor()
{
    QObject::connect(&m_watcher, &QFutureWatcher<IAssistProposal *>::finished, &m_watcher, [this] {
        setAsyncProposalAvailable(m_watcher.result());
    });
}

IAssistProposal *AsyncProcessor::perform()
{
    IAssistProposal *result = immediateProposal();
    interface()->prepareForAsyncUse();
    m_watcher.setFuture(Utils::asyncRun([this] {
        interface()->recreateTextDocument();
        return performAsync();
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

IAssistProposal *AsyncProcessor::immediateProposal()
{
    return nullptr;
}

bool AsyncProcessor::isCanceled() const
{
    return m_watcher.isCanceled();
}

} // namespace TextEditor
