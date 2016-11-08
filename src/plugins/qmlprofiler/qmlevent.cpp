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

#include "qmlevent.h"
#include <QDataStream>
#include <QDebug>

namespace QmlProfiler {

bool operator==(const QmlEvent &event1, const QmlEvent &event2)
{
    if (event1.timestamp() != event2.timestamp() || event1.typeIndex() != event2.typeIndex())
        return false;

    // This is not particularly efficient, but we also don't need to do this very often.
    return event1.numbers<QVarLengthArray<qint64>>() == event2.numbers<QVarLengthArray<qint64>>();
}

bool operator!=(const QmlEvent &event1, const QmlEvent &event2)
{
    return !(event1 == event2);
}

enum SerializationType {
    OneByte    = 0,
    TwoByte    = 1,
    FourByte   = 2,
    EightByte  = 3,
    TypeMask   = 0x3
};

enum SerializationTypeOffset {
    TimestampOffset  = 0,
    TypeIndexOffset  = 2,
    DataLengthOffset = 4,
    DataOffset       = 6
};

template<typename Number>
static inline void readNumbers(QDataStream &stream, Number *data, quint16 length)
{
    for (quint16 i = 0; i != length; ++i)
        stream >> data[i];
}

template<typename Number>
static inline Number readNumber(QDataStream &stream, qint8 type)
{
    switch (type) {
    case OneByte: {
        qint8 value;
        stream >> value;
        return static_cast<Number>(value);
    }
    case TwoByte: {
        qint16 value;
        stream >> value;
        return static_cast<Number>(value);
    }
    case FourByte: {
        qint32 value;
        stream >> value;
        return static_cast<Number>(value);
    }
    case EightByte: {
        qint64 value;
        stream >> value;
        return static_cast<Number>(value);
    }
    default:
        Q_UNREACHABLE();
        return 0;
    }
}

QDataStream &operator>>(QDataStream &stream, QmlEvent &event)
{
    qint8 type;
    stream >> type;

    event.m_timestamp = readNumber<qint64>(stream, (type >> TimestampOffset) & TypeMask);
    event.m_typeIndex = readNumber<qint32>(stream, (type >> TypeIndexOffset) & TypeMask);
    event.m_dataLength = readNumber<qint16>(stream, (type >> DataLengthOffset) & TypeMask);

    quint8 bytesPerNumber = 1 << ((type >> DataOffset) & TypeMask);

    if (event.m_dataLength * bytesPerNumber > sizeof(event.m_data)) {
        event.m_dataType = static_cast<QmlEvent::Type>((bytesPerNumber * 8) | QmlEvent::External);
        event.m_data.external = malloc(event.m_dataLength * bytesPerNumber);
    } else {
        event.m_dataType = static_cast<QmlEvent::Type>(bytesPerNumber * 8);
    }

    switch (event.m_dataType) {
    case QmlEvent::Inline8Bit:
        readNumbers<qint8>(stream, event.m_data.internal8bit, event.m_dataLength);
        break;
    case QmlEvent::External8Bit:
        readNumbers<qint8>(stream, static_cast<qint8 *>(event.m_data.external),
                           event.m_dataLength);
        break;
    case QmlEvent::Inline16Bit:
        readNumbers<qint16>(stream, event.m_data.internal16bit, event.m_dataLength);
        break;
    case QmlEvent::External16Bit:
        readNumbers<qint16>(stream, static_cast<qint16 *>(event.m_data.external),
                            event.m_dataLength);
        break;
    case QmlEvent::Inline32Bit:
        readNumbers<qint32>(stream, event.m_data.internal32bit, event.m_dataLength);
        break;
    case QmlEvent::External32Bit:
        readNumbers<qint32>(stream, static_cast<qint32 *>(event.m_data.external),
                            event.m_dataLength);
        break;
    case QmlEvent::Inline64Bit:
        readNumbers<qint64>(stream, event.m_data.internal64bit, event.m_dataLength);
        break;
    case QmlEvent::External64Bit:
        readNumbers<qint64>(stream, static_cast<qint64 *>(event.m_data.external),
                            event.m_dataLength);
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    return stream;
}

static inline qint8 minimumType(const QmlEvent &event, quint16 length, quint16 origBitsPerNumber)
{
    qint8 type = OneByte;
    bool ok = true;
    for (quint16 i = 0; i < length;) {
        if ((1 << type) == origBitsPerNumber / 8)
            return type;
        switch (type) {
        case OneByte:
            ok = (event.number<qint8>(i) == event.number<qint64>(i));
            break;
        case TwoByte:
            ok = (event.number<qint16>(i) == event.number<qint64>(i));
            break;
        case FourByte:
            ok = (event.number<qint32>(i) == event.number<qint64>(i));
            break;
        default:
            // EightByte isn't possible, as (1 << type) == origBitsPerNumber / 8 then.
            Q_UNREACHABLE();
            break;
        }

        if (ok)
            ++i;
        else
            ++type;
    }
    return type;
}

template<typename Number>
static inline qint8 minimumType(Number number)
{
     if (static_cast<qint8>(number) == number)
         return OneByte;
     if (static_cast<qint16>(number) == number)
         return TwoByte;
     if (static_cast<qint32>(number) == number)
         return FourByte;
     return EightByte;
}

template<typename Number>
static inline void writeNumbers(QDataStream &stream, const QmlEvent &event, quint16 length)
{
    for (quint16 i = 0; i != length; ++i)
        stream << event.number<Number>(i);
}

template<typename Number>
static inline void writeNumber(QDataStream &stream, Number number, qint8 type)
{
    switch (type) {
    case OneByte:
        stream << static_cast<qint8>(number);
        break;
    case TwoByte:
        stream << static_cast<qint16>(number);
        break;
    case FourByte:
        stream << static_cast<qint32>(number);
        break;
    case EightByte:
        stream << static_cast<qint64>(number);
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
}

QDataStream &operator<<(QDataStream &stream, const QmlEvent &event)
{
    qint8 type = minimumType(event.m_timestamp) << TimestampOffset;
    type |= minimumType(event.m_typeIndex) << TypeIndexOffset;
    type |= minimumType(event.m_dataLength) << DataLengthOffset;
    type |= minimumType(event, event.m_dataLength, event.m_dataType) << DataOffset;
    stream << type;

    writeNumber(stream, event.m_timestamp, (type >> TimestampOffset) & TypeMask);
    writeNumber(stream, event.m_typeIndex, (type >> TypeIndexOffset) & TypeMask);
    writeNumber(stream, event.m_dataLength, (type >> DataLengthOffset) & TypeMask);

    switch ((type >> DataOffset) & TypeMask) {
    case OneByte:
        writeNumbers<qint8>(stream, event, event.m_dataLength);
        break;
    case TwoByte:
        writeNumbers<qint16>(stream, event, event.m_dataLength);
        break;
    case FourByte:
        writeNumbers<qint32>(stream, event, event.m_dataLength);
        break;
    case EightByte:
        writeNumbers<qint64>(stream, event, event.m_dataLength);
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    return stream;
}

} // namespace QmlProfiler
