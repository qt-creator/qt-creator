// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlDesigner {

class PropertyAbstractContainer;
class PropertyBindingContainer;
class PropertyValueContainer;

class Update3dViewStateCommand;
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
class ChangeSelectionCommand;
class InputEventCommand;
class View3DActionCommand;
class RequestModelNodePreviewImageCommand;
class ChangeLanguageCommand;
class ChangePreviewImageSizeCommand;

class NodeInstanceServerInterface : public QObject
{
    Q_OBJECT
public:
    explicit NodeInstanceServerInterface(QObject *parent = nullptr);

    virtual void createInstances(const CreateInstancesCommand &command) = 0;
    virtual void changeFileUrl(const ChangeFileUrlCommand &command) = 0;
    virtual void createScene(const CreateSceneCommand &command) = 0;
    virtual void clearScene(const ClearSceneCommand &command) = 0;
    virtual void update3DViewState(const Update3dViewStateCommand &command) = 0;
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
    virtual void changeSelection(const ChangeSelectionCommand &command) = 0;
    virtual void inputEvent(const InputEventCommand &command) = 0;
    virtual void view3DAction(const View3DActionCommand &command) = 0;
    virtual void requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command) = 0;
    virtual void changeLanguage(const ChangeLanguageCommand &command) = 0;
    virtual void changePreviewImageSize(const ChangePreviewImageSizeCommand &command) = 0;
    virtual void dispatchCommand(const QVariant &) {}

    virtual void benchmark(const QString &) {}

    static void registerCommands();
};

}
