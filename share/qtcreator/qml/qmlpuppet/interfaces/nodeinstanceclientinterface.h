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

#ifndef NODEINSTANCECLIENTINTERFACE_H
#define NODEINSTANCECLIENTINTERFACE_H

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

    virtual void flush() {};
    virtual void synchronizeWithClientProcess() {}
    virtual qint64 bytesToWrite() const {return 0;}

};

}

#endif // NODEINSTANCECLIENTINTERFACE_H
