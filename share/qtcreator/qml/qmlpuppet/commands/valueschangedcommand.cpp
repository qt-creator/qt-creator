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

#include "valueschangedcommand.h"

#include "sharedmemory.h"
#include <QCache>
#include <QDebug>

#include <cstring>

#include <algorithm>

namespace QmlDesigner {

// using cache as a container which deletes sharedmemory pointers at process exit
typedef QCache<qint32, SharedMemory> GlobalSharedMemoryContainer;
Q_GLOBAL_STATIC_WITH_ARGS(GlobalSharedMemoryContainer, globalSharedMemoryContainer, (10000))

ValuesChangedCommand::ValuesChangedCommand()
    : m_keyNumber(0)
{
}

ValuesChangedCommand::ValuesChangedCommand(const QVector<PropertyValueContainer> &valueChangeVector)
    : m_valueChangeVector (valueChangeVector),
      m_keyNumber(0)
{
}

QVector<PropertyValueContainer> ValuesChangedCommand::valueChanges() const
{
    return m_valueChangeVector;
}

quint32 ValuesChangedCommand::keyNumber() const
{
    return m_keyNumber;
}

void ValuesChangedCommand::removeSharedMemorys(const QVector<qint32> &keyNumberVector)
{
    foreach (qint32 keyNumber, keyNumberVector) {
        SharedMemory *sharedMemory = globalSharedMemoryContainer()->take(keyNumber);
        delete sharedMemory;
    }
}

void ValuesChangedCommand::sort()
{
    std::sort(m_valueChangeVector.begin(), m_valueChangeVector.end());
}

static const QLatin1String valueKeyTemplateString("Values-%1");

static SharedMemory *createSharedMemory(qint32 key, int byteCount)
{
    SharedMemory *sharedMemory = new SharedMemory(QString(valueKeyTemplateString).arg(key));

    bool sharedMemoryIsCreated = sharedMemory->create(byteCount);

    if (sharedMemoryIsCreated) {
        globalSharedMemoryContainer()->insert(key, sharedMemory);
        return sharedMemory;
    } else {
        delete sharedMemory;
    }

    return 0;
}

QDataStream &operator<<(QDataStream &out, const ValuesChangedCommand &command)
{
    static const bool dontUseSharedMemory = !qgetenv("DESIGNER_DONT_USE_SHARED_MEMORY").isEmpty();

    if (!dontUseSharedMemory && command.valueChanges().count() > 5) {
        static quint32 keyCounter = 0;
        ++keyCounter;
        command.m_keyNumber = keyCounter;
        QByteArray outDataStreamByteArray;
        QDataStream temporaryOutDataStream(&outDataStreamByteArray, QIODevice::WriteOnly);
        temporaryOutDataStream.setVersion(QDataStream::Qt_4_8);

        temporaryOutDataStream << command.valueChanges();

        SharedMemory *sharedMemory = createSharedMemory(keyCounter, outDataStreamByteArray.size());

        if (sharedMemory) {
            sharedMemory->lock();
            std::memcpy(sharedMemory->data(), outDataStreamByteArray.constData(), sharedMemory->size());
            sharedMemory->unlock();

            out << command.keyNumber();
            return out;
        }
    }

    out << qint32(0);
    out << command.valueChanges();

    return out;
}

void readSharedMemory(qint32 key, QVector<PropertyValueContainer> *valueChangeVector)
{
    SharedMemory sharedMemory(QString(valueKeyTemplateString).arg(key));
    bool canAttach = sharedMemory.attach(QSharedMemory::ReadOnly);

    if (canAttach) {
        sharedMemory.lock();

        QDataStream in(QByteArray::fromRawData(static_cast<const char*>(sharedMemory.constData()), sharedMemory.size()));
        in.setVersion(QDataStream::Qt_4_8);
        in >> *valueChangeVector;

        sharedMemory.unlock();
        sharedMemory.detach();
    }
}

QDataStream &operator>>(QDataStream &in, ValuesChangedCommand &command)
{
    in >> command.m_keyNumber;

    if (command.keyNumber() > 0) {
        readSharedMemory(command.keyNumber(), &command.m_valueChangeVector);
    } else {
        in >> command.m_valueChangeVector;
    }
    return in;
}

bool operator ==(const ValuesChangedCommand &first, const ValuesChangedCommand &second)
{
    return first.m_valueChangeVector == second.m_valueChangeVector;
}

QDebug operator <<(QDebug debug, const ValuesChangedCommand &command)
{
    return debug.nospace() << "ValuesChangedCommand("
                    << "keyNumber: " << command.keyNumber() << ", "
                    << command.valueChanges() << ")";
}

} // namespace QmlDesigner
