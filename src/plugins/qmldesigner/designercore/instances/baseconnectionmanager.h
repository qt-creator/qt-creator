// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "connectionmanagerinterface.h"

#include <mutex>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class AbstractView;

class QMLDESIGNERCORE_EXPORT BaseConnectionManager : public QObject, public ConnectionManagerInterface
{
    Q_OBJECT

public:
    BaseConnectionManager() = default;

    void setUp(NodeInstanceServerInterface *nodeInstanceServer,
               const QString &qrcMappingString,
               ProjectExplorer::Target *target,
               AbstractView *view,
               ExternalDependenciesInterface &) override;
    void shutDown() override;

    void setCrashCallback(std::function<void()> callback) override;

    bool isActive() const;

protected:
    void dispatchCommand(const QVariant &command, Connection &connection) override;
    virtual void showCannotConnectToPuppetWarningAndSwitchToEditMode();
    using ConnectionManagerInterface::processFinished;
    void processFinished(const QString &reason);
    static void writeCommandToIODevice(const QVariant &command,
                                       QIODevice *ioDevice,
                                       unsigned int commandCounter);
    void readDataStream(Connection &connection);

    NodeInstanceServerInterface *nodeInstanceServer() const { return m_nodeInstanceServer; }

    void callCrashCallback();

private:
    std::mutex m_callbackMutex;
    std::function<void()> m_crashCallback;
    NodeInstanceServerInterface *m_nodeInstanceServer{};
    bool m_isActive = false;
};

} // namespace QmlDesigner
