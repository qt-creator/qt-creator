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

#include "nodeinstanceclientinterface.h"

#include <QObject>
#include <QHash>
#include <QWeakPointer>
#include <QFile>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLocalSocket;
class QIODevice;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceServerInterface;
class CreateSceneCommand;
class CreateInstancesCommand;
class ClearSceneCommand;
class ReparentInstancesCommand;
class ChangeFileUrlCommand;
class ChangeValuesCommand;
class ChangeAuxiliaryCommand;
class ChangeBindingsCommand;
class ChangeIdsCommand;
class RemoveInstancesCommand;
class RemovePropertiesCommand;
class CompleteComponentCommand;
class ChangeStateCommand;
class ChangeNodeSourceCommand;
class EndPuppetCommand;


class NodeInstanceClientProxy : public QObject, public NodeInstanceClientInterface
{
    Q_OBJECT

public:
    NodeInstanceClientProxy(QObject *parent = 0);

    void informationChanged(const InformationChangedCommand &command) override;
    void valuesChanged(const ValuesChangedCommand &command) override;
    void pixmapChanged(const PixmapChangedCommand &command) override;
    void childrenChanged(const ChildrenChangedCommand &command) override;
    void statePreviewImagesChanged(const StatePreviewImageChangedCommand &command) override;
    void componentCompleted(const ComponentCompletedCommand &command) override;
    void token(const TokenCommand &command) override;
    void debugOutput(const DebugOutputCommand &command) override;
    void puppetAlive(const PuppetAliveCommand &command);

    void flush() override;
    void synchronizeWithClientProcess() override;
    qint64 bytesToWrite() const override;

protected:
    void initializeSocket();
    void initializeCapturedStream(const QString &fileName);
    void writeCommand(const QVariant &command);
    void dispatchCommand(const QVariant &command);
    NodeInstanceServerInterface *nodeInstanceServer() const;
    void setNodeInstanceServer(NodeInstanceServerInterface *nodeInstanceServer);

    void createInstances(const CreateInstancesCommand &command);
    void changeFileUrl(const ChangeFileUrlCommand &command);
    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void removeInstances(const RemoveInstancesCommand &command);
    void removeProperties(const RemovePropertiesCommand &command);
    void changePropertyBindings(const ChangeBindingsCommand &command);
    void changePropertyValues(const ChangeValuesCommand &command);
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command);
    void reparentInstances(const ReparentInstancesCommand &command);
    void changeIds(const ChangeIdsCommand &command);
    void changeState(const ChangeStateCommand &command);
    void completeComponent(const CompleteComponentCommand &command);
    void changeNodeSource(const ChangeNodeSourceCommand &command);
    void removeSharedMemory(const RemoveSharedMemoryCommand &command);
    void redirectToken(const TokenCommand &command);
    void redirectToken(const EndPuppetCommand &command);
    static QVariant readCommandFromIOStream(QIODevice *ioDevice, quint32 *readCommandCounter, quint32 *blockSize);

protected slots:
    void readDataStream();
    void sendPuppetAliveCommand();

private:
    QFile m_controlStream;
    QTimer m_puppetAliveTimer;
    QIODevice *m_inputIoDevice;
    QIODevice *m_outputIoDevice;
    NodeInstanceServerInterface *m_nodeInstanceServer;
    quint32 m_writeCommandCounter;
    int m_synchronizeId;
};

} // namespace QmlDesigner
