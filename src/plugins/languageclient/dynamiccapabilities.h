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

#pragma once

#include <languageserverprotocol/client.h>

namespace LanguageClient {

class DynamicCapability
{
public:
    DynamicCapability() = default;
    void enable(QString id, QJsonValue options)
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

    Utils::optional<bool> isRegistered(const QString &method) const;
    QJsonValue option(const QString &method) const { return m_capability[method].options(); }

    void reset();

private:
    QHash<QString, DynamicCapability> m_capability;
    QHash<QString, QString> m_methodForId;
};

} // namespace LanguageClient
