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

#pragma once

#include <pchmanagerclientinterface.h>

#include <vector>

namespace ClangPchManager {
class PchManagerConnectionClient;

class PchManagerNotifierInterface;

class PchManagerClient final : public ClangBackEnd::PchManagerClientInterface
{
    friend class PchManagerNotifierInterface;
public:
    void alive() override;
    void precompiledHeadersUpdated(ClangBackEnd::PrecompiledHeadersUpdatedMessage &&message) override;

    void precompiledHeaderRemoved(const QString &projectPartId);

    void setConnectionClient(PchManagerConnectionClient *connectionClient);

unittest_public:
    const std::vector<PchManagerNotifierInterface*> &notifiers() const;
    void precompiledHeaderUpdated(const QString &projectPartId, const QString &pchFilePath);

    void attach(PchManagerNotifierInterface *notifier);
    void detach(PchManagerNotifierInterface *notifier);

private:
    std::vector<PchManagerNotifierInterface*> m_notifiers;
    PchManagerConnectionClient *m_connectionClient=nullptr;
};

} // namespace ClangPchManager
