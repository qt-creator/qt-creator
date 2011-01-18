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
class CreateSceneCommand;
class CreateInstancesCommand;
class ClearSceneCommand;
class ReparentInstancesCommand;
class ChangeIdsCommand;
class RemoveInstancesCommand;
class RemovePropertiesCommand;
class ChangeStateCommand;
class AddImportCommand;
class CompleteComponentCommand;

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
    virtual void reparentInstances(const ReparentInstancesCommand &command) = 0;
    virtual void changeIds(const ChangeIdsCommand &command) = 0;
    virtual void changeState(const ChangeStateCommand &command) = 0;
    virtual void addImport(const AddImportCommand &command) = 0;
    virtual void completeComponent(const CompleteComponentCommand &command) = 0;

    static void registerCommands();
};

}
#endif // NODEINSTANCESERVERINTERFACE_H
