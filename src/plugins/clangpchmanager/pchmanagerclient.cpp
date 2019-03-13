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
#include <progressmanagerinterface.h>
#include <progressmessage.h>
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
    for (ClangBackEnd::ProjectPartPch &projectPartPch : message.takeProjectPartPchs()) {
        const QString pchPath{projectPartPch.pchPath};
        addProjectPartPch(std::move(projectPartPch));
        precompiledHeaderUpdated(projectPartPch.projectPartId, pchPath, projectPartPch.lastModified);
    }
}

void PchManagerClient::progress(ClangBackEnd::ProgressMessage &&message)
{
    switch (message.progressType) {
    case ClangBackEnd::ProgressType::PrecompiledHeader:
        m_pchCreationProgressManager.setProgress(message.progress, message.total);
        break;
    case ClangBackEnd::ProgressType::DependencyCreation:
        m_dependencyCreationProgressManager.setProgress(message.progress, message.total);
        break;
    default:
        break;
    }
}

void PchManagerClient::precompiledHeaderRemoved(ClangBackEnd::ProjectPartId projectPartId)
{
    for (auto notifier : m_notifiers) {
        removeProjectPartPch(projectPartId);
        notifier->precompiledHeaderRemoved(projectPartId);
    }
}

void PchManagerClient::setConnectionClient(PchManagerConnectionClient *connectionClient)
{
    m_connectionClient = connectionClient;
}

Utils::optional<ClangBackEnd::ProjectPartPch> PchManagerClient::projectPartPch(
    ClangBackEnd::ProjectPartId projectPartId) const
{
    auto found = std::lower_bound(m_projectPartPchs.cbegin(),
                                  m_projectPartPchs.cend(),
                                  projectPartId,
                                  [] (const auto &projectPartPch, auto projectPartId) {
            return projectPartId < projectPartPch.projectPartId;
    });

    if (found != m_projectPartPchs.end() && found->projectPartId == projectPartId)
        return *found;

    return Utils::nullopt;
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

void PchManagerClient::removeProjectPartPch(ClangBackEnd::ProjectPartId projectPartId)
{
    auto found = std::lower_bound(m_projectPartPchs.begin(),
                                  m_projectPartPchs.end(),
                                  projectPartId,
                                  [] (const auto &projectPartPch, auto projectPartId) {
            return projectPartId < projectPartPch.projectPartId;
    });

    if (found != m_projectPartPchs.end() && found->projectPartId == projectPartId) {
        *found = std::move(m_projectPartPchs.back());
        m_projectPartPchs.pop_back();
    }
}

void PchManagerClient::addProjectPartPch(ClangBackEnd::ProjectPartPch &&projectPartPch)
{
    auto found = std::lower_bound(m_projectPartPchs.begin(),
                                  m_projectPartPchs.end(),
                                  projectPartPch.projectPartId,
                                  [] (const auto &projectPartPch, auto projectPartId) {
            return projectPartId < projectPartPch.projectPartId;
    });

    if (found != m_projectPartPchs.end() && found->projectPartId == projectPartPch.projectPartId)
        *found = std::move(projectPartPch);
    else
        m_projectPartPchs.insert(found, std::move(projectPartPch));
}

const std::vector<PchManagerNotifierInterface *> &PchManagerClient::notifiers() const
{
    return m_notifiers;
}

void PchManagerClient::precompiledHeaderUpdated(ClangBackEnd::ProjectPartId projectPartId,
                                                const QString &pchFilePath,
                                                long long lastModified)
{
    for (auto notifier : m_notifiers)
        notifier->precompiledHeaderUpdated(projectPartId, pchFilePath, lastModified);
}

} // namespace ClangPchManager
