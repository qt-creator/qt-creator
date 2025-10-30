// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "credentialquery.h"

#include <qtkeychain/keychain.h>

using namespace QKeychain;
using namespace QtTaskTree;

namespace Core {

CredentialQueryTaskAdapter::~CredentialQueryTaskAdapter() = default;

void CredentialQueryTaskAdapter::operator()(CredentialQuery *task, QTaskInterface *iface)
{
    Job *job = nullptr;
    ReadPasswordJob *reader = nullptr;

    switch (task->m_operation) {
    case CredentialOperation::Get: {
        job = reader = new ReadPasswordJob(task->m_service);
        break;
    }
    case CredentialOperation::Set: {
        WritePasswordJob *writer = new WritePasswordJob(task->m_service);
        if (task->m_data)
            writer->setBinaryData(*task->m_data);
        job = writer;
        break;
    }
    case CredentialOperation::Delete:
        job = new DeletePasswordJob(task->m_service);
        break;
    }

    job->setAutoDelete(false);
    job->setKey(task->m_key);
    m_guard.reset(job);

    QObject::connect(job, &Job::finished, iface, [this, iface, task, reader](Job *job) {
        const bool success = job->error() == NoError || job->error() == EntryNotFound;
        if (!success)
            task->m_errorString = job->errorString();
        else if (reader && job->error() == NoError)
            task->m_data = reader->binaryData();
        iface->reportDone(toDoneResult(success));
        m_guard.release()->deleteLater();
    }, Qt::SingleShotConnection);
    job->start();
}

} // Core
