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

#include "declarativetoolsclient.h"
#include <QMetaEnum>
#include <QStringList>

namespace QmlDebug {
namespace Internal {

namespace Constants {

enum DesignTool {
    NoTool = 0,
    SelectionToolMode = 1,
    MarqueeSelectionToolMode = 2,
    MoveToolMode = 3,
    ResizeToolMode = 4,
    ZoomMode = 6
};

}

class InspectorProtocol : public QObject
{
    Q_OBJECT
    Q_ENUMS(Message Tool)

public:
    enum Message {
        ChangeTool             = 1,
        ColorChanged           = 3,
        CurrentObjectsChanged  = 6,
        ObjectIdList           = 9,
        Reload                 = 10,
        Reloaded               = 11,
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
        return QString::fromUtf8(staticMetaObject.enumerator(0).valueToKey(message));
    }

    static inline QString toString(Tool tool)
    {
        return QString::fromUtf8(staticMetaObject.enumerator(1).valueToKey(tool));
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

} // internal

using namespace Internal;

DeclarativeToolsClient::DeclarativeToolsClient(QmlDebugConnection *client)
    : BaseToolsClient(client,QLatin1String("QDeclarativeObserverMode")),
      m_connection(client)
{
    setObjectName(name());
}

void DeclarativeToolsClient::messageReceived(const QByteArray &message)
{
    QDataStream ds(message);

    InspectorProtocol::Message type;
    ds >> type;

    switch (type) {
    case InspectorProtocol::CurrentObjectsChanged: {
        int objectCount;
        ds >> objectCount;

        log(LogReceive, type, QString::fromLatin1("%1 [list of debug ids]").arg(objectCount));

        QList<int> currentDebugIds;

        for (int i = 0; i < objectCount; ++i) {
            int debugId;
            ds >> debugId;
            if (debugId != -1)
                currentDebugIds << debugId;
        }

        emit currentObjectsChanged(currentDebugIds);
        break;
    }
    case InspectorProtocol::ToolChanged: {
        int toolId;
        ds >> toolId;

        log(LogReceive, type, QString::number(toolId));

        if (toolId == Constants::ZoomMode)
            emit zoomToolActivated();
        else if (toolId == Constants::SelectionToolMode)
            emit selectToolActivated();
        else if (toolId == Constants::MarqueeSelectionToolMode)
            emit selectMarqueeToolActivated();
        break;
    }
    case InspectorProtocol::SetDesignMode: {
        bool inDesignMode;
        ds >> inDesignMode;

        log(LogReceive, type, QLatin1String(inDesignMode ? "true" : "false"));

        emit designModeBehaviorChanged(inDesignMode);
        break;
    }
    case InspectorProtocol::ShowAppOnTop: {
        bool showAppOnTop;
        ds >> showAppOnTop;

        log(LogReceive, type, QLatin1String(showAppOnTop ? "true" : "false"));
        break;
    }
    case InspectorProtocol::Reloaded: {
        log(LogReceive, type);
        emit reloaded();
        break;
    }
    default:
        log(LogReceive, type, QLatin1String("Warning: Not handling message"));
    }
}

void DeclarativeToolsClient::setObjectIdList(
        const QList<ObjectReference> &objectRoots)
{
    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    QList<int> debugIds;
    QList<QString> objectIds;

    foreach (const ObjectReference &ref, objectRoots)
        recurseObjectIdList(ref, debugIds, objectIds);

    InspectorProtocol::Message cmd = InspectorProtocol::ObjectIdList;
    ds << cmd
       << debugIds.length();

    Q_ASSERT(debugIds.length() == objectIds.length());

    for (int i = 0; i < debugIds.length(); ++i) {
        ds << debugIds[i] << objectIds[i];
    }

    log(LogSend, cmd,
        QString::fromLatin1("%1 %2 [list of debug / object ids]").arg(debugIds.length()));

    sendMessage(message);
}

void DeclarativeToolsClient::setDesignModeBehavior(bool inDesignMode)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::SetDesignMode;
    ds << cmd
       << inDesignMode;

    log(LogSend, cmd, QLatin1String(inDesignMode ? "true" : "false"));

    sendMessage(message);
}

void DeclarativeToolsClient::changeToSelectTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ChangeTool;
    InspectorProtocol::Tool tool = InspectorProtocol::SelectTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, InspectorProtocol::toString(tool));

    sendMessage(message);
}

void DeclarativeToolsClient::changeToSelectMarqueeTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ChangeTool;
    InspectorProtocol::Tool tool = InspectorProtocol::SelectMarqueeTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, InspectorProtocol::toString(tool));

    sendMessage(message);
}

void DeclarativeToolsClient::changeToZoomTool()
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ChangeTool;
    InspectorProtocol::Tool tool = InspectorProtocol::ZoomTool;
    ds << cmd
       << tool;

    log(LogSend, cmd, InspectorProtocol::toString(tool));

    sendMessage(message);
}

void DeclarativeToolsClient::showAppOnTop(bool showOnTop)
{
    if (!m_connection || !m_connection->isConnected())
        return;

    QByteArray message;
    QDataStream ds(&message, QIODevice::WriteOnly);

    InspectorProtocol::Message cmd = InspectorProtocol::ShowAppOnTop;
    ds << cmd << showOnTop;

    log(LogSend, cmd, QLatin1String(showOnTop ? "true" : "false"));

    sendMessage(message);
}

void DeclarativeToolsClient::log(LogDirection direction,
                               int message,
                               const QString &extra)
{
    QString msg;
    if (direction == LogSend)
        msg += QLatin1String("sending ");
    else
        msg += QLatin1String("receiving ");

    InspectorProtocol::Message msgType
            = static_cast<InspectorProtocol::Message>(message);
    msg += InspectorProtocol::toString(msgType);
    msg += QLatin1Char(' ');
    msg += extra;
    emit logActivity(name(), msg);
}

} // namespace QmlDebug

#include "declarativetoolsclient.moc"
