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

Utils::optional<bool> DynamicCapabilities::isRegistered(const QString &method) const
{
    if (!m_capability.contains(method))
        return Utils::nullopt;
    return m_capability[method].enabled();
}

void DynamicCapabilities::reset()
{
    m_capability.clear();
    m_methodForId.clear();
}

} // namespace LanguageClient
