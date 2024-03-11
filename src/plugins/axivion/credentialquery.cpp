// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "credentialquery.h"

#include <qtkeychain/keychain.h>

using namespace QKeychain;
using namespace Tasking;

namespace Axivion::Internal {

CredentialQueryTaskAdapter::~CredentialQueryTaskAdapter() = default;

void CredentialQueryTaskAdapter::start()
{
    Job *job = nullptr;
    ReadPasswordJob *reader = nullptr;

    switch (task()->m_operation) {
    case CredentialOperation::Get: {
        job = reader = new ReadPasswordJob(task()->m_service);
        break;
    }
    case CredentialOperation::Set: {
        WritePasswordJob *writer = new WritePasswordJob(task()->m_service);
        if (task()->m_data)
            writer->setBinaryData(*task()->m_data);
        job = writer;
        break;
    }
    case CredentialOperation::Delete:
        job = new DeletePasswordJob(task()->m_service);
        break;
    }

    job->setAutoDelete(false);
    job->setKey(task()->m_key);
    m_guard.reset(job);

    connect(job, &Job::finished, this, [this, reader](Job *job) {
        const bool success = job->error() == NoError || job->error() == EntryNotFound;
        if (!success)
            task()->m_errorString = job->errorString();
        else if (reader && job->error() == NoError)
            task()->m_data = reader->binaryData();
        disconnect(job, &Job::finished, this, nullptr);
        emit done(toDoneResult(success));
        m_guard.release()->deleteLater();
    });
    job->start();
}

} // Axivion::Internal
