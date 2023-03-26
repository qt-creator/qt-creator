// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodeinstanceserverinterface.h"
#include <qmetatype.h>

#include "addimportcontainer.h"
#include "captureddatacommand.h"
#include "changeauxiliarycommand.h"
#include "changebindingscommand.h"
#include "changefileurlcommand.h"
#include "changeidscommand.h"
#include "changelanguagecommand.h"
#include "changenodesourcecommand.h"
#include "changepreviewimagesizecommand.h"
#include "changeselectioncommand.h"
#include "changestatecommand.h"
#include "changevaluescommand.h"
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
#include "nanotracecommand.h"
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
#include "scenecreatedcommand.h"
#include "statepreviewimagechangedcommand.h"
#include "synchronizecommand.h"
#include "tokencommand.h"
#include "update3dviewstatecommand.h"
#include "valueschangedcommand.h"
#include "view3dactioncommand.h"
#include "requestmodelnodepreviewimagecommand.h"

#include <enumeration.h>

namespace QmlDesigner {

static bool isRegistered = false;

NodeInstanceServerInterface::NodeInstanceServerInterface(QObject *parent) :
    QObject(parent)
{
    registerCommands();
}

template<typename T>
inline void registerCommand(const char *typeName)
{
    qRegisterMetaType<T>(typeName);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<T>(typeName);
#endif
}

void NodeInstanceServerInterface::registerCommands()
{
    if (isRegistered)
        return;

    isRegistered = true;

    registerCommand<CreateInstancesCommand>("CreateInstancesCommand");
    registerCommand<ClearSceneCommand>("ClearSceneCommand");
    registerCommand<CreateSceneCommand>("CreateSceneCommand");
    registerCommand<Update3dViewStateCommand>("Update3dViewStateCommand");
    registerCommand<ChangeBindingsCommand>("ChangeBindingsCommand");
    registerCommand<ChangeValuesCommand>("ChangeValuesCommand");
    registerCommand<ChangeFileUrlCommand>("ChangeFileUrlCommand");
    registerCommand<ChangeStateCommand>("ChangeStateCommand");
    registerCommand<RemoveInstancesCommand>("RemoveInstancesCommand");
    registerCommand<ChangeSelectionCommand>("ChangeSelectionCommand");
    registerCommand<RemovePropertiesCommand>("RemovePropertiesCommand");
    registerCommand<ReparentInstancesCommand>("ReparentInstancesCommand");
    registerCommand<ChangeIdsCommand>("ChangeIdsCommand");
    registerCommand<PropertyAbstractContainer>("PropertyAbstractContainer");
    registerCommand<InformationChangedCommand>("InformationChangedCommand");
    registerCommand<ValuesChangedCommand>("ValuesChangedCommand");
    registerCommand<ValuesModifiedCommand>("ValuesModifiedCommand");
    registerCommand<PixmapChangedCommand>("PixmapChangedCommand");
    registerCommand<InformationContainer>("InformationContainer");
    registerCommand<PropertyValueContainer>("PropertyValueContainer");
    registerCommand<PropertyBindingContainer>("PropertyBindingContainer");
    registerCommand<PropertyAbstractContainer>("PropertyAbstractContainer");
    registerCommand<InstanceContainer>("InstanceContainer");
    registerCommand<IdContainer>("IdContainer");
    registerCommand<ChildrenChangedCommand>("ChildrenChangedCommand");
    registerCommand<ImageContainer>("ImageContainer");
    registerCommand<StatePreviewImageChangedCommand>("StatePreviewImageChangedCommand");
    registerCommand<CompleteComponentCommand>("CompleteComponentCommand");
    registerCommand<ComponentCompletedCommand>("ComponentCompletedCommand");
    registerCommand<AddImportContainer>("AddImportContainer");
    registerCommand<SynchronizeCommand>("SynchronizeCommand");
    registerCommand<ChangeNodeSourceCommand>("ChangeNodeSourceCommand");
    registerCommand<ChangeAuxiliaryCommand>("ChangeAuxiliaryCommand");
    registerCommand<TokenCommand>("TokenCommand");
    registerCommand<RemoveSharedMemoryCommand>("RemoveSharedMemoryCommand");
    registerCommand<EndPuppetCommand>("EndPuppetCommand");
    registerCommand<DebugOutputCommand>("DebugOutputCommand");
    registerCommand<Enumeration>("Enumeration");
    registerCommand<PuppetAliveCommand>("PuppetAliveCommand");
    registerCommand<PuppetToCreatorCommand>("PuppetToCreatorCommand");
    registerCommand<InputEventCommand>("InputEventCommand");
    registerCommand<View3DActionCommand>("View3DActionCommand");
    registerCommand<RequestModelNodePreviewImageCommand>("RequestModelNodePreviewImageCommand");
    registerCommand<QPair<int, int>>("QPairIntInt");
    registerCommand<QList<QColor>>("QColorList");
    registerCommand<ChangeLanguageCommand>("ChangeLanguageCommand");
    registerCommand<ChangePreviewImageSizeCommand>("ChangePreviewImageSizeCommand");
    registerCommand<CapturedDataCommand>("CapturedDataCommand");
    registerCommand<SceneCreatedCommand>("SceneCreatedCommand");
    registerCommand<StartNanotraceCommand>("StartNanotraceCommand");
    registerCommand<EndNanotraceCommand>("EndNanotraceCommand");
    registerCommand<SyncNanotraceCommand>("SyncNanotraceCommand");
}

}
