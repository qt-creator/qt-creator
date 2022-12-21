// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltypedevent.h"
#include <QVarLengthArray>

namespace QmlProfiler {

QDataStream &operator>>(QDataStream &stream, QmlTypedEvent &event)
{
    qint64 time;
    qint32 messageType;
    qint32 subtype;

    stream >> time >> messageType;

    if (messageType < 0 || messageType >= MaximumMessage)
        messageType = UndefinedMessage;

    RangeType rangeType = UndefinedRangeType;
    if (!stream.atEnd()) {
        stream >> subtype;
        if (subtype >= 0 && subtype < MaximumRangeType)
            rangeType = static_cast<RangeType>(subtype);
    } else {
        subtype = -1;
    }

    event.event.setTimestamp(time > 0 ? time : 0);
    event.event.setTypeIndex(-1);
    event.serverTypeId = 0;

    switch (messageType) {
    case Event: {
        if (subtype >= MaximumEventType)
            subtype = UndefinedEventType;
        event.type = QmlEventType(static_cast<Message>(messageType), UndefinedRangeType, subtype);
        switch (subtype) {
        case StartTrace:
        case EndTrace: {
            QVarLengthArray<qint32> engineIds;
            while (!stream.atEnd()) {
                qint32 id;
                stream >> id;
                engineIds << id;
            }
            event.event.setNumbers<QVarLengthArray<qint32>, qint32>(engineIds);
            break;
        }
        case AnimationFrame: {
            qint32 frameRate, animationCount;
            qint32 threadId;
            stream >> frameRate >> animationCount;
            if (!stream.atEnd())
                stream >> threadId;
            else
                threadId = 0;

            event.event.setNumbers<qint32>({frameRate, animationCount, threadId});
            break;
        }
        case Mouse:
        case Key:
            int inputType = (subtype == Key ? InputKeyUnknown : InputMouseUnknown);
            if (!stream.atEnd())
                stream >> inputType;
            qint32 a = -1;
            if (!stream.atEnd())
                stream >> a;
            qint32 b = -1;
            if (!stream.atEnd())
                stream >> b;

            event.event.setNumbers<qint32>({inputType, a, b});
            break;
        }

        break;
    }
    case Complete: {
        event.type = QmlEventType(static_cast<Message>(messageType), UndefinedRangeType, subtype);
        break;
    }
    case SceneGraphFrame: {
        QVarLengthArray<qint64> params;
        qint64 param;

        while (!stream.atEnd()) {
            stream >> param;
            params.push_back(param);
        }

        event.type = QmlEventType(static_cast<Message>(messageType), UndefinedRangeType, subtype);
        event.event.setNumbers<QVarLengthArray<qint64>, qint64>(params);
        break;
    }
    case PixmapCacheEvent: {
        qint32 width = 0, height = 0, refcount = 0;
        QString filename;
        stream >> filename;
        if (subtype == PixmapReferenceCountChanged || subtype == PixmapCacheCountChanged) {
            stream >> refcount;
        } else if (subtype == PixmapSizeKnown) {
            stream >> width >> height;
            refcount = 1;
        }

        event.type = QmlEventType(static_cast<Message>(messageType), UndefinedRangeType, subtype,
                                  QmlEventLocation(filename, 0, 0));
        event.event.setNumbers<qint32>({width, height, refcount});
        break;
    }
    case MemoryAllocation: {
        qint64 delta;
        stream >> delta;

        event.type = QmlEventType(static_cast<Message>(messageType), UndefinedRangeType, subtype);
        event.event.setNumbers<qint64>({delta});
        break;
    }
    case RangeStart: {
        if (!stream.atEnd()) {
            qint64 typeId;
            stream >> typeId;
            if (stream.status() == QDataStream::Ok)
                event.serverTypeId = typeId;
            // otherwise it's the old binding type of 4 bytes
        }

        event.type = QmlEventType(UndefinedMessage, rangeType, -1);
        event.event.setRangeStage(RangeStart);
        break;
    }
    case RangeData: {
        QString data;
        stream >> data;

        event.type = QmlEventType(UndefinedMessage, rangeType, -1, QmlEventLocation(), data);
        event.event.setRangeStage(RangeData);
        if (!stream.atEnd())
            stream >> event.serverTypeId;
        break;
    }
    case RangeLocation: {
        QString filename;
        qint32 line = 0;
        qint32 column = 0;
        stream >> filename >> line;

        if (!stream.atEnd()) {
            stream >> column;
            if (!stream.atEnd())
                stream >> event.serverTypeId;
        }

        event.type = QmlEventType(UndefinedMessage, rangeType, -1,
                                  QmlEventLocation(filename, line, column));
        event.event.setRangeStage(RangeLocation);
        break;
    }
    case RangeEnd: {
        event.type = QmlEventType(UndefinedMessage, rangeType, -1);
        event.event.setRangeStage(RangeEnd);
        break;
    }
    case Quick3DEvent: {

        QVarLengthArray<qint64> params;
        qint64 param = 0;
        QByteArray str;
        if (subtype == Quick3DEventData) {
            stream >> str;
        } else {
            while (!stream.atEnd()) {
                stream >> param;
                params.push_back(param);
            }
        }

        event.type = QmlEventType(static_cast<Message>(messageType), UndefinedRangeType, subtype);
        event.type.setData(QString::fromUtf8(str));
        event.event.setNumbers<QVarLengthArray<qint64>, qint64>(params);
        break;
    }
    default:
        event.event.setNumbers<char>({});
        event.type = QmlEventType(static_cast<Message>(messageType), UndefinedRangeType, subtype);
        break;
    }

    return stream;
}

} // namespace QmlProfiler
