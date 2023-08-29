#pragma once

/*
 * Copyright (C) 2022-current by Axivion GmbH
 * https://www.axivion.com/
 *
 * SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
 */

#include "dashboard/dto.h"

#include <utils/expected.h>
#include <utils/networkaccessmanager.h>

#include <QFuture>

namespace Axivion::Internal
{

class DashboardClient
{
public:
    using RawProjectInfo = Utils::expected_str<Dto::ProjectInfoDto>;

    DashboardClient(Utils::NetworkAccessManager &networkAccessManager);

    QFuture<RawProjectInfo> fetchProjectInfo(const QString &projectName);

private:
    Utils::NetworkAccessManager &m_networkAccessManager;
};

} // namespace Axivion::Internal
