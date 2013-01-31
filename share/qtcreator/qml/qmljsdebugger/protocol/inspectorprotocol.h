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

#ifndef INSPECTORPROTOCOL_H
#define INSPECTORPROTOCOL_H

#include <QDebug>
#include <QMetaType>
#include <QMetaEnum>
#include <QObject>

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
