/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef INSPECTORPROTOCOL_H
#define INSPECTORPROTOCOL_H

#include <QtCore/QDebug>
#include <QtCore/QMetaType>
#include <QtCore/QMetaEnum>
#include <QtCore/QObject>

namespace QmlJSDebugger {

class InspectorProtocol : public QObject
{
    Q_OBJECT
    Q_ENUMS(Message Tool)

public:
    enum Message {
        AnimationSpeedChanged  = 0,
        AnimationPausedChanged = 19, // highest value
        ChangeTool             = 1,
        ClearComponentCache    = 2,
        ColorChanged           = 3,
        CreateObject           = 5,
        CurrentObjectsChanged  = 6,
        DestroyObject          = 7,
        MoveObject             = 8,
        ObjectIdList           = 9,
        Reload                 = 10,
        Reloaded               = 11,
        SetAnimationSpeed      = 12,
        SetAnimationPaused     = 18,
        SetCurrentObjects      = 14,
        SetDesignMode          = 15,
        ShowAppOnTop           = 16,
        ToolChanged            = 17
    };

    enum Tool {
        ColorPickerTool,
        SelectMarqueeTool,
        SelectTool,
        ZoomTool
    };

    static inline QString toString(Message message)
    {
        return staticMetaObject.enumerator(0).valueToKey(message);
    }

    static inline QString toString(Tool tool)
    {
        return staticMetaObject.enumerator(1).valueToKey(tool);
    }
};

inline QDataStream & operator<< (QDataStream &stream, InspectorProtocol::Message message)
{
    return stream << static_cast<quint32>(message);
}

inline QDataStream & operator>> (QDataStream &stream, InspectorProtocol::Message &message)
{
    quint32 i;
    stream >> i;
    message = static_cast<InspectorProtocol::Message>(i);
    return stream;
}

inline QDebug operator<< (QDebug dbg, InspectorProtocol::Message message)
{
    dbg << InspectorProtocol::toString(message);
    return dbg;
}

inline QDataStream & operator<< (QDataStream &stream, InspectorProtocol::Tool tool)
{
    return stream << static_cast<quint32>(tool);
}

inline QDataStream & operator>> (QDataStream &stream, InspectorProtocol::Tool &tool)
{
    quint32 i;
    stream >> i;
    tool = static_cast<InspectorProtocol::Tool>(i);
    return stream;
}

inline QDebug operator<< (QDebug dbg, InspectorProtocol::Tool tool)
{
    dbg << InspectorProtocol::toString(tool);
    return dbg;
}

} // namespace QmlJSDebugger

#endif // INSPECTORPROTOCOL_H
