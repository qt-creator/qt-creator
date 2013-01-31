/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef NODEINSTANCESERVERPROXY_H
#define NODEINSTANCESERVERPROXY_H

#include "nodeinstanceserverinterface.h"

#include <QDataStream>
#include <QWeakPointer>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
class QProcess;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceClientInterface;
class NodeInstanceView;
class NodeInstanceClientProxy;

class NodeInstanceServerProxy : public NodeInstanceServerInterface
{
    Q_OBJECT
public:
    explicit NodeInstanceServerProxy(NodeInstanceView *nodeInstanceView, RunModus runModus = NormalModus, const QString &pathToQt = QString());
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
    void dispatchCommand(const QVariant &command);
    NodeInstanceClientInterface *nodeInstanceClient() const;

signals:
    void processCrashed();

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void readFirstDataStream();
    void readSecondDataStream();
    void readThirdDataStream();

private:
    QString qmlPuppetApplicationName() const;
    QString macOSBundlePath(const QString &path) const;

    QWeakPointer<QLocalServer> m_localServer;
    QWeakPointer<QLocalSocket> m_firstSocket;
    QWeakPointer<QLocalSocket> m_secondSocket;
    QWeakPointer<QLocalSocket> m_thirdSocket;
    QWeakPointer<NodeInstanceView> m_nodeInstanceView;
    QWeakPointer<QProcess> m_qmlPuppetEditorProcess;
    QWeakPointer<QProcess> m_qmlPuppetPreviewProcess;
    QWeakPointer<QProcess> m_qmlPuppetRenderProcess;
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
