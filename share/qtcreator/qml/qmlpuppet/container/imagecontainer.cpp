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

#include "imagecontainer.h"

#include "sharedmemory.h"
#include <QCache>
#include <QDebug>

#include <cstring>

#define QTC_ASSERT_STRINGIFY_HELPER(x) #x
#define QTC_ASSERT_STRINGIFY(x) QTC_ASSERT_STRINGIFY_HELPER(x)
#define QTC_ASSERT_STRING(cond) qDebug("SOFT ASSERT: \"" cond"\" in file " __FILE__ ", line " QTC_ASSERT_STRINGIFY(__LINE__))
#define QTC_ASSERT(cond, action) if (cond) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)

namespace QmlDesigner {

// using cache as a container which deletes sharedmemory pointers at process exit
typedef QCache<qint32, SharedMemory> GlobalSharedMemoryContainer;
Q_GLOBAL_STATIC_WITH_ARGS(GlobalSharedMemoryContainer, globalSharedMemoryContainer, (10000))

ImageContainer::ImageContainer()
    : m_instanceId(-1),
      m_keyNumber(-2)
{
}

ImageContainer::ImageContainer(qint32 instanceId, const QImage &image, qint32 keyNumber)
    :  m_image(image),
       m_instanceId(instanceId),
       m_keyNumber(keyNumber)
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

qint32 ImageContainer::keyNumber() const
{
    return m_keyNumber;
}

void ImageContainer::setImage(const QImage &image)
{
    QTC_ASSERT(m_image.isNull(), /**/);

    m_image = image;
}

void ImageContainer::removeSharedMemorys(const QVector<qint32> &keyNumberVector)
{
    foreach (qint32 keyNumber, keyNumberVector) {
        SharedMemory *sharedMemory = globalSharedMemoryContainer()->take(keyNumber);
        delete sharedMemory;
    }
}

static const QLatin1String imageKeyTemplateString("Image-%1");

static SharedMemory *createSharedMemory(qint32 key, int byteCount)
{
    SharedMemory *sharedMemory = (*globalSharedMemoryContainer())[key];

    if (sharedMemory == 0) {
        sharedMemory = new SharedMemory(QString(imageKeyTemplateString).arg(key));
        bool sharedMemoryIsCreated = sharedMemory->create(byteCount);
        if (sharedMemoryIsCreated) {
            globalSharedMemoryContainer()->insert(key, sharedMemory);
        } else {
            delete sharedMemory;
            sharedMemory = 0;
        }
    } else {
        bool sharedMemoryIsAttached = sharedMemory->isAttached();
        if (!sharedMemoryIsAttached)
            sharedMemoryIsAttached = sharedMemory->attach();

        bool sharedMemorySizeIsSmallerThanByteCount = sharedMemory->size() < byteCount;
        bool sharedMemorySizeIsDoubleBiggerThanByteCount = sharedMemory->size() > (byteCount * 2);

        if (!sharedMemoryIsAttached) {
            sharedMemory->create(byteCount);
        } else if (sharedMemorySizeIsSmallerThanByteCount || sharedMemorySizeIsDoubleBiggerThanByteCount) {
            sharedMemory->detach();
            sharedMemory->create(byteCount);
        }

        if (!sharedMemory->isAttached()) {
            globalSharedMemoryContainer()->remove(key);
            sharedMemory = 0;
        }
    }

    return sharedMemory;
}

static void writeSharedMemory(SharedMemory *sharedMemory, const QImage &image)
{
    sharedMemory->lock();

    qint32 headerData[5];
    headerData[0] = image.byteCount();
    headerData[1] = image.bytesPerLine();
    headerData[2] = image.size().width();
    headerData[3] = image.size().height();
    headerData[4] = image.format();

    std::memcpy(sharedMemory->data(), headerData, 20);
    std::memcpy(reinterpret_cast<char*>(sharedMemory->data()) + 20, image.constBits(), image.byteCount());
    sharedMemory->unlock();
}

static void writeStream(QDataStream &out, const QImage &image)
{
    out << qint32(image.bytesPerLine());
    out << image.size();
    out << qint32(image.format());
    out << qint32(image.byteCount());
    out.writeRawData(reinterpret_cast<const char*>(image.constBits()), image.byteCount());
}

QDataStream &operator<<(QDataStream &out, const ImageContainer &container)
{
    const int extraDataSize =  20;
    static const bool dontUseSharedMemory = !qgetenv("DESIGNER_DONT_USE_SHARED_MEMORY").isEmpty();

    out << container.instanceId();
    out << container.keyNumber();

    const QImage image = container.image();

    if (dontUseSharedMemory) {
        out << qint32(0);
        writeStream(out, image);
    } else {
        SharedMemory *sharedMemory = createSharedMemory(container.keyNumber(), image.byteCount() + extraDataSize);

        out << qint32(sharedMemory != 0); // send if shared memory is used

        if (sharedMemory)
            writeSharedMemory(sharedMemory, image);
        else
            writeStream(out, image);
    }

    return out;
}

static void readSharedMemory(qint32 key, ImageContainer &container)
{
    SharedMemory sharedMemory(QString(imageKeyTemplateString).arg(key));

    bool canAttach = sharedMemory.attach(QSharedMemory::ReadOnly);

    if (canAttach && sharedMemory.size() >= 20)
    {
        sharedMemory.lock();
        qint32 headerData[5];
        std::memcpy(headerData, sharedMemory.constData(), 20);

        qint32 byteCount = headerData[0];
//        qint32 bytesPerLine = headerData[1];
        qint32 imageWidth = headerData[2];
        qint32 imageHeight = headerData[3];
        qint32 imageFormat = headerData[4];

        QImage image = QImage(imageWidth, imageHeight, QImage::Format(imageFormat));

        std::memcpy(image.bits(), reinterpret_cast<const qint32*>(sharedMemory.constData()) + 5, byteCount);

        container.setImage(image);

        sharedMemory.unlock();
        sharedMemory.detach();
    }
}

static void readStream(QDataStream &in, ImageContainer &container)
{
    qint32 byteCount;
    qint32 bytesPerLine;
    QSize imageSize;
    qint32 imageFormat;

    in >> bytesPerLine;
    in >> imageSize;
    in >> imageFormat;
    in >> byteCount;

    QImage image = QImage(imageSize, QImage::Format(imageFormat));

    in.readRawData(reinterpret_cast<char*>(image.bits()), byteCount);

    container.setImage(image);
}

QDataStream &operator>>(QDataStream &in, ImageContainer &container)
{
    qint32 sharedMemoryIsUsed;

    in >> container.m_instanceId;
    in >> container.m_keyNumber;
    in >> sharedMemoryIsUsed;

    if (sharedMemoryIsUsed) {
        readSharedMemory(container.keyNumber(), container);
    } else
        readStream(in, container);

    return in;
}

bool operator ==(const ImageContainer &first, const ImageContainer &second)
{
    return first.m_instanceId == second.m_instanceId
            && first.m_image == second.m_image;
}

bool operator <(const ImageContainer &first, const ImageContainer &second)
{
    return first.m_instanceId < second.m_instanceId;
}

QDebug operator <<(QDebug debug, const ImageContainer &container)
{
    return debug.nospace() << "ImageContainer("
                           << "instanceId: " << container.instanceId() << ", "
                           << "size: " << container.image().size() << ")";
}


} // namespace QmlDesigner
