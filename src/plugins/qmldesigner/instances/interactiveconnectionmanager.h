// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "connectionmanager.h"

namespace QmlDesigner {

class InteractiveConnectionManager : public ConnectionManager
{
    Q_OBJECT

public:
    InteractiveConnectionManager();

    void setUp(NodeInstanceServerInterface *nodeInstanceServer,
               const QString &qrcMappingString,
               ProjectExplorer::Target *target,
               AbstractView *view,
               ExternalDependenciesInterface &externalDependencies) override;

    void shutDown() override;

    void showCannotConnectToPuppetWarningAndSwitchToEditMode() override;

protected:
    void dispatchCommand(const QVariant &command, Connection &connection) override;

private:
    void puppetTimeout(Connection &connection);
    void puppetAlive(Connection &connection);

private:
    AbstractView *m_view{};
};

} // namespace QmlDesigner
