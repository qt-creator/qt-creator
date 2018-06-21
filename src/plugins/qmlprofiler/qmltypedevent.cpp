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

#include "qmltypedevent.h"
#include <QVarLengthArray>

namespace QmlProfiler {

QDataStream &operator>>(QDataStream &stream, QmlTypedEvent &event)
{
    qint64 time;
    qint32 messageType;
    qint32 subtype;

    stream >> time >> messageType;

    if (messageType < 0 || messageType > MaximumMessage)
        messageType = MaximumMessage;

    RangeType rangeType = MaximumRangeType;
    if (!stream.atEnd()) {
        stream >> subtype;
        rangeType = static_cast<RangeType>(subtype);
        if (rangeType < 0 || rangeType > MaximumRangeType)
            rangeType = MaximumRangeType;
    } else {
        subtype = -1;
    }

    event.event.setTimestamp(time > 0 ? time : 0);
    event.event.setTypeIndex(-1);
    event.serverTypeId = 0;

    switch (messageType) {
    case Event: {
        event.type = QmlEventType(static_cast<Message>(messageType), MaximumRangeType, subtype);
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
        event.type = QmlEventType(static_cast<Message>(messageType), MaximumRangeType, subtype);
        break;
    }
    case SceneGraphFrame: {
        QVarLengthArray<qint64> params;
        qint64 param;

        while (!stream.atEnd()) {
            stream >> param;
            params.push_back(param);
        }

        event.type = QmlEventType(static_cast<Message>(messageType), MaximumRangeType, subtype);
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

        event.type = QmlEventType(static_cast<Message>(messageType), MaximumRangeType, subtype,
                                  QmlEventLocation(filename, 0, 0));
        event.event.setNumbers<qint32>({width, height, refcount});
        break;
    }
    case MemoryAllocation: {
        qint64 delta;
        stream >> delta;

        event.type = QmlEventType(static_cast<Message>(messageType), MaximumRangeType, subtype);
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

        event.type = QmlEventType(MaximumMessage, rangeType, -1);
        event.event.setRangeStage(RangeStart);
        break;
    }
    case RangeData: {
        QString data;
        stream >> data;

        event.type = QmlEventType(MaximumMessage, rangeType, -1, QmlEventLocation(), data);
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

        event.type = QmlEventType(MaximumMessage, rangeType, -1,
                                  QmlEventLocation(filename, line, column));
        event.event.setRangeStage(RangeLocation);
        break;
    }
    case RangeEnd: {
        event.type = QmlEventType(MaximumMessage, rangeType, -1);
        event.event.setRangeStage(RangeEnd);
        break;
    }
    default:
        event.event.setNumbers<char>({});
        event.type = QmlEventType(static_cast<Message>(messageType), MaximumRangeType, subtype);
        break;
    }

    return stream;
}

} // namespace QmlProfiler
