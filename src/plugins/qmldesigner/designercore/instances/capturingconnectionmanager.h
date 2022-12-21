// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <interactiveconnectionmanager.h>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT CapturingConnectionManager : public InteractiveConnectionManager
{
    Q_OBJECT

public:
    void setUp(NodeInstanceServerInterface *nodeInstanceServer,
               const QString &qrcMappingString,
               ProjectExplorer::Target *target,
               AbstractView *view,
               ExternalDependenciesInterface &externalDependencies) override;

    void processFinished(int exitCode, QProcess::ExitStatus exitStatus, const QString &connectionName) override;

    void writeCommand(const QVariant &command) override;

private:
    QFile m_captureFileForTest;
};

} // namespace QmlDesigner

