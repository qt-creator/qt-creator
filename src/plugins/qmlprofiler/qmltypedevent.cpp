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

    RangeType rangeType = MaximumRangeType;
    if (!stream.atEnd()) {
        stream >> subtype;
        rangeType = static_cast<RangeType>(subtype);
    } else {
        subtype = -1;
    }

    event.event.setTimestamp(time);
    event.event.setTypeIndex(-1);
    event.type.displayName.clear();
    event.type.data.clear();
    event.type.location.filename.clear();
    event.type.location.line = event.type.location.column = -1;

    switch (messageType) {
    case Event: {
        event.type.detailType = subtype;
        event.type.rangeType = MaximumRangeType;
        event.type.message = static_cast<Message>(messageType);
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
        event.type.message = static_cast<Message>(messageType);
        event.type.rangeType = MaximumRangeType;
        event.type.detailType = subtype;
        break;
    }
    case SceneGraphFrame: {
        QVarLengthArray<qint64> params;
        qint64 param;

        while (!stream.atEnd()) {
            stream >> param;
            params.push_back(param);
        }

        event.type.message = static_cast<Message>(messageType);
        event.type.rangeType = MaximumRangeType;
        event.type.detailType = subtype;
        event.event.setNumbers<QVarLengthArray<qint64>, qint64>(params);
        break;
    }
    case PixmapCacheEvent: {
        qint32 width = 0, height = 0, refcount = 0;
        stream >> event.type.location.filename;
        if (subtype == PixmapReferenceCountChanged || subtype == PixmapCacheCountChanged) {
            stream >> refcount;
        } else if (subtype == PixmapSizeKnown) {
            stream >> width >> height;
            refcount = 1;
        }

        event.type.message = static_cast<Message>(messageType);
        event.type.rangeType = MaximumRangeType;
        event.type.location.line = event.type.location.column = 0;
        event.type.detailType = subtype;
        event.event.setNumbers<qint32>({width, height, refcount});
        break;
    }
    case MemoryAllocation: {
        qint64 delta;
        stream >> delta;

        event.type.message = static_cast<Message>(messageType);
        event.type.rangeType = MaximumRangeType;
        event.type.detailType = subtype;
        event.event.setNumbers<qint64>({delta});
        break;
    }
    case RangeStart: {
        // read and ignore binding type
        if (rangeType == Binding && !stream.atEnd()) {
            qint32 bindingType;
            stream >> bindingType;
        }

        event.type.message = MaximumMessage;
        event.type.rangeType = rangeType;
        event.event.setRangeStage(RangeStart);
        event.type.detailType = -1;
        break;
    }
    case RangeData: {
        stream >> event.type.data;

        event.type.message = MaximumMessage;
        event.type.rangeType = rangeType;
        event.event.setRangeStage(RangeData);
        event.type.detailType = -1;
        break;
    }
    case RangeLocation: {
        stream >> event.type.location.filename
                >> static_cast<qint32 &>(event.type.location.line);

        if (!stream.atEnd())
            stream >> static_cast<qint32 &>(event.type.location.column);
        else
            event.type.location.column = -1;

        event.type.message = MaximumMessage;
        event.type.rangeType = rangeType;
        event.event.setRangeStage(RangeLocation);
        event.type.detailType = -1;
        break;
    }
    case RangeEnd: {
        event.type.message = MaximumMessage;
        event.type.rangeType = rangeType;
        event.event.setRangeStage(RangeEnd);
        event.type.detailType = -1;
        break;
    }
    default:
        event.type.message = static_cast<Message>(messageType);
        event.type.rangeType = MaximumRangeType;
        event.type.detailType = subtype;
        break;
    }

    return stream;
}

} // namespace QmlProfiler
