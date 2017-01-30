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

#include "pchmanagerclient.h"

#include <precompiledheadersupdatedmessage.h>
#include <pchmanagerconnectionclient.h>

#include <pchmanagernotifierinterface.h>

#include <algorithm>

namespace ClangPchManager {

void PchManagerClient::alive()
{
    if (m_connectionClient)
        m_connectionClient->resetProcessAliveTimer();
}

void PchManagerClient::precompiledHeadersUpdated(ClangBackEnd::PrecompiledHeadersUpdatedMessage &&message)
{
    for (const ClangBackEnd::ProjectPartPch &projectPartPch : message.projectPartPchs())
        precompiledHeaderUpdated(projectPartPch.id(), projectPartPch.path());
}

void PchManagerClient::precompiledHeaderRemoved(const QString &projectPartId)
{
    for (auto notifier : m_notifiers)
        notifier->precompiledHeaderRemoved(projectPartId);
}

void PchManagerClient::setConnectionClient(PchManagerConnectionClient *connectionClient)
{
    m_connectionClient = connectionClient;
}

void PchManagerClient::attach(PchManagerNotifierInterface *notifier)
{
    m_notifiers.push_back(notifier);
}

void PchManagerClient::detach(PchManagerNotifierInterface *notifierToBeDeleted)
{
    auto newEnd = std::partition(m_notifiers.begin(),
                                 m_notifiers.end(),
                                 [&] (PchManagerNotifierInterface *notifier) {
            return notifier != notifierToBeDeleted;
    });

    m_notifiers.erase(newEnd, m_notifiers.end());
}

const std::vector<PchManagerNotifierInterface *> &PchManagerClient::notifiers() const
{
    return m_notifiers;
}

void PchManagerClient::precompiledHeaderUpdated(const QString &projectPartId, const QString &pchFilePath)
{
    for (auto notifier : m_notifiers)
        notifier->precompiledHeaderUpdated(projectPartId, pchFilePath);
}

} // namespace ClangPchManager
