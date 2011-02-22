/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef NODEINSTANCECLIENTPROXY_H
#define NODEINSTANCECLIENTPROXY_H

#include "nodeinstanceclientinterface.h"

#include <QObject>
#include <QHash>
#include <QWeakPointer>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceServerInterface;
class CreateSceneCommand;
class CreateInstancesCommand;
class ClearSceneCommand;
class ReparentInstancesCommand;
class ChangeFileUrlCommand;
class ChangeValuesCommand;
class ChangeBindingsCommand;
class ChangeIdsCommand;
class RemoveInstancesCommand;
class RemovePropertiesCommand;
class AddImportCommand;
class CompleteComponentCommand;
class ChangeStateCommand;

class NodeInstanceClientProxy : public QObject, public NodeInstanceClientInterface
{
    Q_OBJECT

public:
    NodeInstanceClientProxy(QObject *parent = 0);

    void informationChanged(const InformationChangedCommand &command);
    void valuesChanged(const ValuesChangedCommand &command);
    void pixmapChanged(const PixmapChangedCommand &command);
    void childrenChanged(const ChildrenChangedCommand &command);
    void statePreviewImagesChanged(const StatePreviewImageChangedCommand &command);
    void componentCompleted(const ComponentCompletedCommand &command);

    void flush();
    void synchronizeWithClientProcess();
    qint64 bytesToWrite() const;

protected:
    void writeCommand(const QVariant &command);
    void dispatchCommand(const QVariant &command);
    NodeInstanceServerInterface *nodeInstanceServer() const;

    void createInstances(const CreateInstancesCommand &command);
    void changeFileUrl(const ChangeFileUrlCommand &command);
    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void removeInstances(const RemoveInstancesCommand &command);
    void removeProperties(const RemovePropertiesCommand &command);
    void changePropertyBindings(const ChangeBindingsCommand &command);
    void changePropertyValues(const ChangeValuesCommand &command);
    void reparentInstances(const ReparentInstancesCommand &command);
    void changeIds(const ChangeIdsCommand &command);
    void changeState(const ChangeStateCommand &command);
    void addImport(const AddImportCommand &command);
    void completeComponent(const CompleteComponentCommand &command);

private slots:
    void readDataStream();

private:
    QLocalSocket *m_socket;
    NodeInstanceServerInterface *m_nodeInstanceServer;
    quint32 m_blockSize;
    int m_synchronizeId;
};

} // namespace QmlDesigner

#endif // NODEINSTANCECLIENTPROXY_H
