// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <solutions/tasking/tasktree.h>

namespace Core {

enum class CredentialOperation { Get, Set, Delete };

class CORE_EXPORT CredentialQuery
{
public:
    void setOperation(CredentialOperation operation) { m_operation = operation; }
    void setService(const QString &service) { m_service = service; }
    void setKey(const QString &key) { m_key = key; }
    void setData(const QByteArray &data) { m_data = data; }

    CredentialOperation operation() const { return m_operation; }
    std::optional<QByteArray> data() const { return m_data; }
    QString errorString() const { return m_errorString; }

private:
    CredentialOperation m_operation = CredentialOperation::Get;
    QString m_service;
    QString m_key;
    std::optional<QByteArray> m_data; // Used for input when Set and for output when Get.
    QString m_errorString;
    friend class CredentialQueryTaskAdapter;
};

class CORE_EXPORT CredentialQueryTaskAdapter final : public Tasking::TaskAdapter<CredentialQuery>
{
private:
    ~CredentialQueryTaskAdapter();
    void start() final;
    std::unique_ptr<QObject> m_guard;
};

using CredentialQueryTask = Tasking::CustomTask<CredentialQueryTaskAdapter>;

} // Core
