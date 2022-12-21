// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dynamiccapabilities.h"

using namespace LanguageServerProtocol;

namespace LanguageClient {

void DynamicCapabilities::registerCapability(const QList<Registration> &registrations)
{
    for (const Registration& registration : registrations) {
        const QString &method = registration.method();
        m_capability[method].enable(registration.id(), registration.registerOptions());
        m_methodForId.insert(registration.id(), method);
    }
}

void DynamicCapabilities::unregisterCapability(const QList<Unregistration> &unregistrations)
{
    for (const Unregistration& unregistration : unregistrations) {
        QString method = unregistration.method();
        if (method.isEmpty())
            method = m_methodForId[unregistration.id()];
        m_capability[method].disable();
        m_methodForId.remove(unregistration.id());
    }
}

std::optional<bool> DynamicCapabilities::isRegistered(const QString &method) const
{
    if (!m_capability.contains(method))
        return std::nullopt;
    return m_capability[method].enabled();
}

QStringList DynamicCapabilities::registeredMethods() const
{
    return m_capability.keys();
}

void DynamicCapabilities::reset()
{
    m_capability.clear();
    m_methodForId.clear();
}

} // namespace LanguageClient
