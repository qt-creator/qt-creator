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

#include "nodeinstanceserverinterface.h"
#include <qmetatype.h>

#include "propertyabstractcontainer.h"
#include "propertyvaluecontainer.h"
#include "propertybindingcontainer.h"
#include "instancecontainer.h"
#include "createinstancescommand.h"
#include "createscenecommand.h"
#include "changevaluescommand.h"
#include "changebindingscommand.h"
#include "changeauxiliarycommand.h"
#include "changefileurlcommand.h"
#include "removeinstancescommand.h"
#include "clearscenecommand.h"
#include "removepropertiescommand.h"
#include "reparentinstancescommand.h"
#include "changeidscommand.h"
#include "changestatecommand.h"
#include "completecomponentcommand.h"
#include "addimportcontainer.h"
#include "changenodesourcecommand.h"

#include "informationchangedcommand.h"
#include "pixmapchangedcommand.h"
#include "valueschangedcommand.h"
#include "childrenchangedcommand.h"
#include "imagecontainer.h"
#include "statepreviewimagechangedcommand.h"
#include "componentcompletedcommand.h"
#include "synchronizecommand.h"
#include "tokencommand.h"
#include "removesharedmemorycommand.h"
#include "endpuppetcommand.h"
#include "debugoutputcommand.h"

namespace QmlDesigner {

static bool isRegistered=false;

NodeInstanceServerInterface::NodeInstanceServerInterface(QObject *parent) :
    QObject(parent)
{
    registerCommands();
}

void NodeInstanceServerInterface::registerCommands()
{
    if (isRegistered)
        return;

    isRegistered = true;

    qRegisterMetaType<CreateInstancesCommand>("CreateInstancesCommand");
    qRegisterMetaTypeStreamOperators<CreateInstancesCommand>("CreateInstancesCommand");

    qRegisterMetaType<ClearSceneCommand>("ClearSceneCommand");
    qRegisterMetaTypeStreamOperators<ClearSceneCommand>("ClearSceneCommand");

    qRegisterMetaType<CreateSceneCommand>("CreateSceneCommand");
    qRegisterMetaTypeStreamOperators<CreateSceneCommand>("CreateSceneCommand");

    qRegisterMetaType<ChangeBindingsCommand>("ChangeBindingsCommand");
    qRegisterMetaTypeStreamOperators<ChangeBindingsCommand>("ChangeBindingsCommand");

    qRegisterMetaType<ChangeValuesCommand>("ChangeValuesCommand");
    qRegisterMetaTypeStreamOperators<ChangeValuesCommand>("ChangeValuesCommand");

    qRegisterMetaType<ChangeFileUrlCommand>("ChangeFileUrlCommand");
    qRegisterMetaTypeStreamOperators<ChangeFileUrlCommand>("ChangeFileUrlCommand");

    qRegisterMetaType<ChangeStateCommand>("ChangeStateCommand");
    qRegisterMetaTypeStreamOperators<ChangeStateCommand>("ChangeStateCommand");

    qRegisterMetaType<RemoveInstancesCommand>("RemoveInstancesCommand");
    qRegisterMetaTypeStreamOperators<RemoveInstancesCommand>("RemoveInstancesCommand");

    qRegisterMetaType<RemovePropertiesCommand>("RemovePropertiesCommand");
    qRegisterMetaTypeStreamOperators<RemovePropertiesCommand>("RemovePropertiesCommand");

    qRegisterMetaType<ReparentInstancesCommand>("ReparentInstancesCommand");
    qRegisterMetaTypeStreamOperators<ReparentInstancesCommand>("ReparentInstancesCommand");

    qRegisterMetaType<ChangeIdsCommand>("ChangeIdsCommand");
    qRegisterMetaTypeStreamOperators<ChangeIdsCommand>("ChangeIdsCommand");

    qRegisterMetaType<PropertyAbstractContainer>("PropertyAbstractContainer");
    qRegisterMetaTypeStreamOperators<PropertyAbstractContainer>("PropertyAbstractContainer");

    qRegisterMetaType<InformationChangedCommand>("InformationChangedCommand");
    qRegisterMetaTypeStreamOperators<InformationChangedCommand>("InformationChangedCommand");

    qRegisterMetaType<ValuesChangedCommand>("ValuesChangedCommand");
    qRegisterMetaTypeStreamOperators<ValuesChangedCommand>("ValuesChangedCommand");

    qRegisterMetaType<PixmapChangedCommand>("PixmapChangedCommand");
    qRegisterMetaTypeStreamOperators<PixmapChangedCommand>("PixmapChangedCommand");

    qRegisterMetaType<InformationContainer>("InformationContainer");
    qRegisterMetaTypeStreamOperators<InformationContainer>("InformationContainer");

    qRegisterMetaType<PropertyValueContainer>("PropertyValueContainer");
    qRegisterMetaTypeStreamOperators<PropertyValueContainer>("PropertyValueContainer");

    qRegisterMetaType<PropertyBindingContainer>("PropertyBindingContainer");
    qRegisterMetaTypeStreamOperators<PropertyBindingContainer>("PropertyBindingContainer");

    qRegisterMetaType<PropertyAbstractContainer>("PropertyAbstractContainer");
    qRegisterMetaTypeStreamOperators<PropertyAbstractContainer>("PropertyAbstractContainer");

    qRegisterMetaType<InstanceContainer>("InstanceContainer");
    qRegisterMetaTypeStreamOperators<InstanceContainer>("InstanceContainer");

    qRegisterMetaType<IdContainer>("IdContainer");
    qRegisterMetaTypeStreamOperators<IdContainer>("IdContainer");

    qRegisterMetaType<ChildrenChangedCommand>("ChildrenChangedCommand");
    qRegisterMetaTypeStreamOperators<ChildrenChangedCommand>("ChildrenChangedCommand");

    qRegisterMetaType<ImageContainer>("ImageContainer");
    qRegisterMetaTypeStreamOperators<ImageContainer>("ImageContainer");

    qRegisterMetaType<StatePreviewImageChangedCommand>("StatePreviewImageChangedCommand");
    qRegisterMetaTypeStreamOperators<StatePreviewImageChangedCommand>("StatePreviewImageChangedCommand");

    qRegisterMetaType<CompleteComponentCommand>("CompleteComponentCommand");
    qRegisterMetaTypeStreamOperators<CompleteComponentCommand>("CompleteComponentCommand");

    qRegisterMetaType<ComponentCompletedCommand>("ComponentCompletedCommand");
    qRegisterMetaTypeStreamOperators<ComponentCompletedCommand>("ComponentCompletedCommand");

    qRegisterMetaType<AddImportContainer>("AddImportContainer");
    qRegisterMetaTypeStreamOperators<AddImportContainer>("AddImportContainer");

    qRegisterMetaType<SynchronizeCommand>("SynchronizeCommand");
    qRegisterMetaTypeStreamOperators<SynchronizeCommand>("SynchronizeCommand");

    qRegisterMetaType<ChangeNodeSourceCommand>("ChangeNodeSourceCommand");
    qRegisterMetaTypeStreamOperators<ChangeNodeSourceCommand>("ChangeNodeSourceCommand");

    qRegisterMetaType<ChangeAuxiliaryCommand>("ChangeAuxiliaryCommand");
    qRegisterMetaTypeStreamOperators<ChangeAuxiliaryCommand>("ChangeAuxiliaryCommand");

    qRegisterMetaType<TokenCommand>("TokenCommand");
    qRegisterMetaTypeStreamOperators<TokenCommand>("TokenCommand");

    qRegisterMetaType<RemoveSharedMemoryCommand>("RemoveSharedMemoryCommand");
    qRegisterMetaTypeStreamOperators<RemoveSharedMemoryCommand>("RemoveSharedMemoryCommand");

    qRegisterMetaType<EndPuppetCommand>("EndPuppetCommand");
    qRegisterMetaTypeStreamOperators<EndPuppetCommand>("EndPuppetCommand");

    qRegisterMetaType<DebugOutputCommand>("DebugOutputCommand");
    qRegisterMetaTypeStreamOperators<DebugOutputCommand>("DebugOutputCommand");
}

}
