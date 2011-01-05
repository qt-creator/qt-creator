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
#include "changefileurlcommand.h"
#include "removeinstancescommand.h"
#include "clearscenecommand.h"
#include "removepropertiescommand.h"
#include "reparentinstancescommand.h"
#include "changeidscommand.h"
#include "changestatecommand.h"
#include "completecomponentcommand.h"
#include "addimportcontainer.h"

#include "informationchangedcommand.h"
#include "pixmapchangedcommand.h"
#include "valueschangedcommand.h"
#include "addimportcommand.h"
#include "childrenchangedcommand.h"
#include "imagecontainer.h"
#include "statepreviewimagechangedcommand.h"
#include "componentcompletedcommand.h"


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
    qRegisterMetaTypeStreamOperators<PropertyAbstractContainer>("ChangeStateCommand");

    qRegisterMetaType<RemoveInstancesCommand>("RemoveInstancesCommand");
    qRegisterMetaTypeStreamOperators<RemoveInstancesCommand>("RemoveInstancesCommand");

    qRegisterMetaType<RemovePropertiesCommand>("RemovePropertiesCommand");
    qRegisterMetaTypeStreamOperators<RemovePropertiesCommand>("RemovePropertiesCommand");

    qRegisterMetaType<ReparentInstancesCommand>("ReparentInstancesCommand");
    qRegisterMetaTypeStreamOperators<ReparentInstancesCommand>("ReparentInstancesCommand");

    qRegisterMetaType<ChangeIdsCommand>("ChangeIdsCommand");
    qRegisterMetaTypeStreamOperators<ChangeIdsCommand>("ChangeIdsCommand");

    qRegisterMetaType<ChangeStateCommand>("ChangeStateCommand");
    qRegisterMetaTypeStreamOperators<ChangeStateCommand>("ChangeStateCommand");

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

    qRegisterMetaType<AddImportCommand>("AddImportCommand");
    qRegisterMetaTypeStreamOperators<AddImportCommand>("AddImportCommand");

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
}

}
