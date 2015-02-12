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

#ifndef NODEINSTANCESERVERINTERFACE_H
#define NODEINSTANCESERVERINTERFACE_H

#include <QObject>

namespace QmlDesigner {

class PropertyAbstractContainer;
class PropertyBindingContainer;
class PropertyValueContainer;

class ChangeFileUrlCommand;
class ChangeValuesCommand;
class ChangeBindingsCommand;
class ChangeAuxiliaryCommand;
class CreateSceneCommand;
class CreateInstancesCommand;
class ClearSceneCommand;
class ReparentInstancesCommand;
class ChangeIdsCommand;
class RemoveInstancesCommand;
class RemovePropertiesCommand;
class ChangeStateCommand;
class CompleteComponentCommand;
class ChangeNodeSourceCommand;
class TokenCommand;
class RemoveSharedMemoryCommand;

class NodeInstanceServerInterface : public QObject
{
    Q_OBJECT
public:
    enum RunModus {
        NormalModus,
        TestModus // No preview images and synchronized
    };

    explicit NodeInstanceServerInterface(QObject *parent = 0);

    virtual void createInstances(const CreateInstancesCommand &command) = 0;
    virtual void changeFileUrl(const ChangeFileUrlCommand &command) = 0;
    virtual void createScene(const CreateSceneCommand &command) = 0;
    virtual void clearScene(const ClearSceneCommand &command) = 0;
    virtual void removeInstances(const RemoveInstancesCommand &command) = 0;
    virtual void removeProperties(const RemovePropertiesCommand &command) = 0;
    virtual void changePropertyBindings(const ChangeBindingsCommand &command) = 0;
    virtual void changePropertyValues(const ChangeValuesCommand &command) = 0;
    virtual void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command) = 0;
    virtual void reparentInstances(const ReparentInstancesCommand &command) = 0;
    virtual void changeIds(const ChangeIdsCommand &command) = 0;
    virtual void changeState(const ChangeStateCommand &command) = 0;
    virtual void completeComponent(const CompleteComponentCommand &command) = 0;
    virtual void changeNodeSource(const ChangeNodeSourceCommand &command) = 0;
    virtual void token(const TokenCommand &command) = 0;
    virtual void removeSharedMemory(const RemoveSharedMemoryCommand &command) = 0;

    static void registerCommands();
};

}
#endif // NODEINSTANCESERVERINTERFACE_H
