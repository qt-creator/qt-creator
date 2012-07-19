/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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

    virtual void flush() {};
    virtual void synchronizeWithClientProcess() {}
    virtual qint64 bytesToWrite() const {return 0;}

};

}

#endif // NODEINSTANCECLIENTINTERFACE_H
