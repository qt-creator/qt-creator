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

#include "nodeinstanceserverinterface.h"
#include <qmetatype.h>

#include "addimportcontainer.h"
#include "changeauxiliarycommand.h"
#include "changebindingscommand.h"
#include "changefileurlcommand.h"
#include "changeidscommand.h"
#include "changelanguagecommand.h"
#include "changenodesourcecommand.h"
#include "changeselectioncommand.h"
#include "changestatecommand.h"
#include "changevaluescommand.h"
#include "changepreviewimagesizecommand.h"
#include "childrenchangedcommand.h"
#include "clearscenecommand.h"
#include "completecomponentcommand.h"
#include "componentcompletedcommand.h"
#include "createinstancescommand.h"
#include "createscenecommand.h"
#include "debugoutputcommand.h"
#include "endpuppetcommand.h"
#include "imagecontainer.h"
#include "informationchangedcommand.h"
#include "inputeventcommand.h"
#include "instancecontainer.h"
#include "pixmapchangedcommand.h"
#include "propertyabstractcontainer.h"
#include "propertybindingcontainer.h"
#include "propertyvaluecontainer.h"
#include "puppetalivecommand.h"
#include "puppettocreatorcommand.h"
#include "removeinstancescommand.h"
#include "removepropertiescommand.h"
#include "removesharedmemorycommand.h"
#include "reparentinstancescommand.h"
#include "statepreviewimagechangedcommand.h"
#include "synchronizecommand.h"
#include "tokencommand.h"
#include "update3dviewstatecommand.h"
#include "valueschangedcommand.h"
#include "view3dactioncommand.h"

#include <enumeration.h>

namespace QmlDesigner {

static bool isRegistered = false;

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

    qRegisterMetaType<Update3dViewStateCommand>("Update3dViewStateCommand");
    qRegisterMetaTypeStreamOperators<Update3dViewStateCommand>("Update3dViewStateCommand");

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

    qRegisterMetaType<ChangeSelectionCommand>("ChangeSelectionCommand");
    qRegisterMetaTypeStreamOperators<ChangeSelectionCommand>("ChangeSelectionCommand");

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

    qRegisterMetaType<ValuesModifiedCommand>("ValuesModifiedCommand");
    qRegisterMetaTypeStreamOperators<ValuesModifiedCommand>("ValuesModifiedCommand");

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

    qRegisterMetaType<Enumeration>("Enumeration");
    qRegisterMetaTypeStreamOperators<Enumeration>("Enumeration");

    qRegisterMetaType<PuppetAliveCommand>("PuppetAliveCommand");
    qRegisterMetaTypeStreamOperators<PuppetAliveCommand>("PuppetAliveCommand");

    qRegisterMetaType<PuppetToCreatorCommand>("PuppetToCreatorCommand");
    qRegisterMetaTypeStreamOperators<PuppetToCreatorCommand>("PuppetToCreatorCommand");

    qRegisterMetaType<InputEventCommand>("InputEventCommand");
    qRegisterMetaTypeStreamOperators<InputEventCommand>("InputEventCommand");

    qRegisterMetaType<View3DActionCommand>("View3DActionCommand");
    qRegisterMetaTypeStreamOperators<View3DActionCommand>("View3DActionCommand");

    qRegisterMetaType<QPair<int, int>>("QPairIntInt");
    qRegisterMetaTypeStreamOperators<QPair<int, int>>("QPairIntInt");

    qRegisterMetaType<ChangeLanguageCommand>("ChangeLanguageCommand");
    qRegisterMetaTypeStreamOperators<ChangeLanguageCommand>("ChangeLanguageCommand");

    qRegisterMetaType<ChangePreviewImageSizeCommand>("ChangePreviewImageSizeCommand");
    qRegisterMetaTypeStreamOperators<ChangePreviewImageSizeCommand>("ChangePreviewImageSizeCommand");
}

}
