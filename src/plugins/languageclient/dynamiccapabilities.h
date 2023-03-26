// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageserverprotocol/client.h>

namespace LanguageClient {

class DynamicCapability
{
public:
    DynamicCapability() = default;
    void enable(const QString &id, const QJsonValue &options)
    {
        QTC_CHECK(!m_enabled);
        m_enabled = true;
        m_id = id;
        m_options = options;
    }

    void disable()
    {
        m_enabled = true;
        m_id.clear();
        m_options = QJsonValue();
    }

    bool enabled() const { return m_enabled; }

    QJsonValue options() const { return m_options; }

private:
    bool m_enabled = false;
    QString m_id;
    QJsonValue m_options;

};

class DynamicCapabilities
{
public:
    DynamicCapabilities() = default;

    void registerCapability(const QList<LanguageServerProtocol::Registration> &registrations);
    void unregisterCapability(const QList<LanguageServerProtocol::Unregistration> &unregistrations);

    std::optional<bool> isRegistered(const QString &method) const;
    QJsonValue option(const QString &method) const { return m_capability.value(method).options(); }
    QStringList registeredMethods() const;

    void reset();

private:
    QHash<QString, DynamicCapability> m_capability;
    QHash<QString, QString> m_methodForId;
};

} // namespace LanguageClient
