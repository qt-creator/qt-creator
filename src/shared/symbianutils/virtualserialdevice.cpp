/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "virtualserialdevice.h"
#include <QtCore/QThread>
#include <QtCore/QWaitCondition>

namespace SymbianUtils {

bool VirtualSerialDevice::isSequential() const
{
    return true;
}

VirtualSerialDevice::VirtualSerialDevice(const QString &aPortName, QObject *parent) :
    QIODevice(parent), portName(aPortName), lock(QMutex::NonRecursive), emittingBytesWritten(false), waiterForBytesWritten(NULL)
{
    platInit();
}

const QString& VirtualSerialDevice::getPortName() const
{
    return portName;
}

void VirtualSerialDevice::close()
{
    if (isOpen()) {
        QMutexLocker locker(&lock);
        delete waiterForBytesWritten;
        waiterForBytesWritten = NULL;
        QIODevice::close();
        platClose();
    }
}

void VirtualSerialDevice::emitBytesWrittenIfNeeded(QMutexLocker& locker, qint64 len)
{
    if (waiterForBytesWritten) {
        waiterForBytesWritten->wakeAll();
    }
    if (!emittingBytesWritten) {
        emittingBytesWritten = true;
        locker.unlock();
        emit bytesWritten(len);
        locker.relock();
        emittingBytesWritten = false;
    }
}

} // namespace SymbianUtils
