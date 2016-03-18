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

#pragma once

#include <QtGlobal>

namespace QmlDesigner {

class ValuesChangedCommand;
class PixmapChangedCommand;
class InformationChangedCommand;
class ChildrenChangedCommand;
class StatePreviewImageChangedCommand;
class ComponentCompletedCommand;
class TokenCommand;
class RemoveSharedMemoryCommand;
class DebugOutputCommand;
class PuppetAliveCommand;

class NodeInstanceClientInterface
{
public:
    virtual void informationChanged(const InformationChangedCommand &command) = 0;
    virtual void valuesChanged(const ValuesChangedCommand &command) = 0;
    virtual void pixmapChanged(const PixmapChangedCommand &command) = 0;
    virtual void childrenChanged(const ChildrenChangedCommand &command) = 0;
    virtual void statePreviewImagesChanged(const StatePreviewImageChangedCommand &command) = 0;
    virtual void componentCompleted(const ComponentCompletedCommand &command) = 0;
    virtual void token(const TokenCommand &command) = 0;
    virtual void debugOutput(const DebugOutputCommand &command) = 0;

    virtual void flush() {}
    virtual void synchronizeWithClientProcess() {}
    virtual qint64 bytesToWrite() const {return 0;}
};

}
