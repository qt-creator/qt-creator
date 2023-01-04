// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace QmlDesigner {

class ValuesChangedCommand;
class ValuesModifiedCommand;
class PixmapChangedCommand;
class InformationChangedCommand;
class ChildrenChangedCommand;
class StatePreviewImageChangedCommand;
class ComponentCompletedCommand;
class TokenCommand;
class RemoveSharedMemoryCommand;
class DebugOutputCommand;
class PuppetAliveCommand;
class ChangeSelectionCommand;
class PuppetToCreatorCommand;
class CapturedDataCommand;
class SceneCreatedCommand;

class NodeInstanceClientInterface
{
public:
    virtual void informationChanged(const InformationChangedCommand &command) = 0;
    virtual void valuesChanged(const ValuesChangedCommand &command) = 0;
    virtual void valuesModified(const ValuesModifiedCommand &command) = 0;
    virtual void pixmapChanged(const PixmapChangedCommand &command) = 0;
    virtual void childrenChanged(const ChildrenChangedCommand &command) = 0;
    virtual void statePreviewImagesChanged(const StatePreviewImageChangedCommand &command) = 0;
    virtual void componentCompleted(const ComponentCompletedCommand &command) = 0;
    virtual void token(const TokenCommand &command) = 0;
    virtual void debugOutput(const DebugOutputCommand &command) = 0;
    virtual void selectionChanged(const ChangeSelectionCommand &command) = 0;
    virtual void handlePuppetToCreatorCommand(const PuppetToCreatorCommand &command) = 0;
    virtual void capturedData(const CapturedDataCommand &command) = 0;
    virtual void sceneCreated(const SceneCreatedCommand &command) = 0;

    virtual void flush() {}
    virtual void synchronizeWithClientProcess() {}
    virtual qint64 bytesToWrite() const {return 0;}
};

}
