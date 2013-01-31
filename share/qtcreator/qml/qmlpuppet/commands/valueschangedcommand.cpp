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

#include "valueschangedcommand.h"

#include <QSharedMemory>
#include <QCache>

#include <cstring>

namespace QmlDesigner {

static QCache<qint32, QSharedMemory> globalSharedMemoryCache(10000);

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
        QSharedMemory *sharedMemory = globalSharedMemoryCache.take(keyNumber);
        delete sharedMemory;
    }
}

static const QLatin1String valueKeyTemplateString("Values-%1");

static QSharedMemory *createSharedMemory(qint32 key, int byteCount)
{
    QSharedMemory *sharedMemory = new QSharedMemory(QString(valueKeyTemplateString).arg(key));

    bool sharedMemoryIsCreated = sharedMemory->create(byteCount);
    if (!sharedMemoryIsCreated) {
        if (sharedMemory->isAttached())
            sharedMemory->attach();
        sharedMemory->detach();
        sharedMemoryIsCreated = sharedMemory->create(byteCount);
    }

    if (sharedMemoryIsCreated) {
        globalSharedMemoryCache.insert(key, sharedMemory);
        return sharedMemory;
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

        temporaryOutDataStream << command.valueChanges();;

        QSharedMemory *sharedMemory = createSharedMemory(keyCounter, outDataStreamByteArray.size());

        if (sharedMemory) {
            std::memcpy(sharedMemory->data(), outDataStreamByteArray.constData(), sharedMemory->size());
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
    QSharedMemory sharedMemory(QString(valueKeyTemplateString).arg(key));
    bool canAttach = sharedMemory.attach(QSharedMemory::ReadOnly);

    if (canAttach) {
        QDataStream in(QByteArray::fromRawData(static_cast<const char*>(sharedMemory.constData()), sharedMemory.size()));
        in.setVersion(QDataStream::Qt_4_8);
        in >> *valueChangeVector;
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

} // namespace QmlDesigner
