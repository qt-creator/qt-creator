/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef NODEINSTANCESERVERPROXY_H
#define NODEINSTANCESERVERPROXY_H

#include "nodeinstanceserverinterface.h"

#include <QPointer>
#include <QProcess>
#include <QFile>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
class QProcess;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
}

namespace QmlDesigner {

class NodeInstanceClientInterface;
class NodeInstanceView;
class NodeInstanceClientProxy;

class NodeInstanceServerProxy : public NodeInstanceServerInterface
{
    Q_OBJECT

public:
    enum PuppetStreamType {
        FirstPuppetStream,
        SecondPuppetStream,
        ThirdPuppetStream,
    };

    explicit NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView, RunModus runModus = NormalModus, ProjectExplorer::Kit *kit = 0);
    ~NodeInstanceServerProxy();
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
    void token(const TokenCommand &command);
    void removeSharedMemory(const RemoveSharedMemoryCommand &command);

protected:
    void writeCommand(const QVariant &command);
    void dispatchCommand(const QVariant &command, PuppetStreamType puppetStreamType);
    NodeInstanceClientInterface *nodeInstanceClient() const;
    void puppetAlive(PuppetStreamType puppetStreamType);
    QString qrcMappingString() const;

signals:
    void processCrashed();

private slots:
    void processFinished();
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void readFirstDataStream();
    void readSecondDataStream();
    void readThirdDataStream();

    void printEditorProcessOutput();
    void printPreviewProcessOutput();
    void printRenderProcessOutput();
private:
    QFile m_captureFileForTest;
    QTimer m_firstTimer;
    QTimer m_secondTimer;
    QTimer m_thirdTimer;
    QPointer<QLocalServer> m_localServer;
    QPointer<QLocalSocket> m_firstSocket;
    QPointer<QLocalSocket> m_secondSocket;
    QPointer<QLocalSocket> m_thirdSocket;
    QPointer<NodeInstanceView> m_nodeInstanceView;
    QPointer<QProcess> m_qmlPuppetEditorProcess;
    QPointer<QProcess> m_qmlPuppetPreviewProcess;
    QPointer<QProcess> m_qmlPuppetRenderProcess;
    quint32 m_firstBlockSize;
    quint32 m_secondBlockSize;
    quint32 m_thirdBlockSize;
    quint32 m_writeCommandCounter;
    quint32 m_firstLastReadCommandCounter;
    quint32 m_secondLastReadCommandCounter;
    quint32 m_thirdLastReadCommandCounter;
    RunModus m_runModus;
    int m_synchronizeId;
};

} // namespace QmlDesigner

#endif // NODEINSTANCESERVERPROXY_H
