/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "imagecontainer.h"

#include <QSharedMemory>
#include <QCache>

namespace QmlDesigner {

static QCache<qint32, QSharedMemory> globalSharedMemoryCache(10000);

ImageContainer::ImageContainer()
    : m_instanceId(-1)
{
}

ImageContainer::ImageContainer(qint32 instanceId, const QImage &image)
    :  m_image(image),
       m_instanceId(instanceId)
{
}

qint32 ImageContainer::instanceId() const
{
    return m_instanceId;
}

QImage ImageContainer::image() const
{
    return m_image;
}

static QSharedMemory *createSharedMemory(qint32 key, int byteCount)
{
    QSharedMemory *sharedMemory = globalSharedMemoryCache.take(key);
    if (sharedMemory == 0)
        sharedMemory = new QSharedMemory(QString::number(key));

    if (sharedMemory->isAttached())
        sharedMemory->detach();

    bool sharedMemoryIsCreated = sharedMemory->create(byteCount);

    if (sharedMemoryIsCreated) {
        globalSharedMemoryCache.insert(key, sharedMemory);
        return sharedMemory;
    }

    return 0;
}

QDataStream &operator<<(QDataStream &out, const ImageContainer &container)
{
    out << container.instanceId();

    const QImage image = container.image();
    const QByteArray data(reinterpret_cast<const char*>(image.constBits()), image.byteCount());

    out << qint32(image.bytesPerLine());
    out << image.size();
    out << qint32(image.format());
    out << qint32(image.byteCount());

    QSharedMemory *sharedMemory = createSharedMemory(container.instanceId(), image.byteCount());

    out << qint32(sharedMemory != 0); // send if shared memory is used

    if (sharedMemory) {
        qMemCopy(sharedMemory->data(), image.constBits(), image.byteCount());
    } else {
        out.writeRawData(reinterpret_cast<const char*>(image.constBits()), image.byteCount());
    }


    return out;
}

void readSharedMemory(qint32 key, QImage *image, qint32 byteSize)
{
    QSharedMemory sharedMemory(QString::number(key));
    bool canAttach = sharedMemory.attach(QSharedMemory::ReadOnly);
    if (canAttach)
    {
        qMemCopy(image->bits(), sharedMemory.constData(), byteSize);
    }
}

QDataStream &operator>>(QDataStream &in, ImageContainer &container)
{

    qint32 byteSize;
    qint32 bytesPerLine;
    QSize imageSize;
    qint32 format;
    qint32 sharedmemoryIsUsed;

    in >> container.m_instanceId;

    in >> bytesPerLine;
    in >> imageSize;
    in >> format;
    in >> byteSize;
    in >> sharedmemoryIsUsed;

    container.m_image = QImage(imageSize, QImage::Format(format));

    if (sharedmemoryIsUsed)
        readSharedMemory(container.instanceId(), &container.m_image, byteSize);
    else
        in.readRawData(reinterpret_cast<char*>(container.m_image.bits()), byteSize);

    return in;
}

} // namespace QmlDesigner
