/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "connectionmanagerinterface.h"

#include <QProcess>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class AbstractView;
class NodeInstanceServerProxy;

class QMLDESIGNERCORE_EXPORT BaseConnectionManager : public QObject, public ConnectionManagerInterface
{
    Q_OBJECT

public:
    BaseConnectionManager() = default;

    void setUp(NodeInstanceServerProxy *nodeInstanceServerProxy,
               const QString &qrcMappingString,
               ProjectExplorer::Target *target) override;
    void shutDown() override;

    bool isActive() const;

protected:
    void dispatchCommand(const QVariant &command, Connection &connection) override;
    virtual void showCannotConnectToPuppetWarningAndSwitchToEditMode();
    using ConnectionManagerInterface::processFinished;
    void processFinished();
    void writeCommandToIODevice(const QVariant &command,
                                QIODevice *ioDevice,
                                unsigned int commandCounter);
    void readDataStream(Connection &connection);

    NodeInstanceServerProxy *nodeInstanceServerProxy() const { return m_nodeInstanceServerProxy; }

private:
    NodeInstanceServerProxy *m_nodeInstanceServerProxy{};
    bool m_isActive = false;
};

} // namespace QmlDesigner
