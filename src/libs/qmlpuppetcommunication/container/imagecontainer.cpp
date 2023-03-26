// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imagecontainer.h"

#include "sharedmemory.h"
#include <QCache>
#include <QDebug>
#include <QLoggingCategory>

#include <cstring>

#define QTC_ASSERT_STRINGIFY_HELPER(x) #x
#define QTC_ASSERT_STRINGIFY(x) QTC_ASSERT_STRINGIFY_HELPER(x)
#define QTC_ASSERT_STRING(cond) qDebug("SOFT ASSERT: \"" cond"\" in file " __FILE__ ", line " QTC_ASSERT_STRINGIFY(__LINE__))
#define QTC_ASSERT(cond, action) if (cond) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)

static Q_LOGGING_CATEGORY(imageContainerDebug, "qtc.imagecontainer.debug", QtDebugMsg)

namespace QmlDesigner {

// using cache as a container which deletes sharedmemory pointers at process exit
using GlobalSharedMemoryContainer = QCache<qint32, SharedMemory>;
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

QRectF ImageContainer::rect() const
{
    return m_rect;
}

void ImageContainer::setImage(const QImage &image)
{
    QTC_ASSERT(m_image.isNull(), /**/);

    m_image = image;
}

void ImageContainer::setRect(const QRectF &rectangle)
{
    m_rect = rectangle;
}

void ImageContainer::removeSharedMemorys(const QVector<qint32> &keyNumberVector)
{
    for (qint32 keyNumber : keyNumberVector) {
        SharedMemory *sharedMemory = globalSharedMemoryContainer()->take(keyNumber);
        delete sharedMemory;
    }
}

static const QLatin1String imageKeyTemplateString("Image-%1");

static SharedMemory *createSharedMemory(qint32 key, int byteCount)
{
    SharedMemory *sharedMemory = (*globalSharedMemoryContainer())[key];

    if (sharedMemory == nullptr) {
        sharedMemory = new SharedMemory(QString(imageKeyTemplateString).arg(key));
        bool sharedMemoryIsCreated = sharedMemory->create(byteCount);
        if (sharedMemoryIsCreated) {
            if (!globalSharedMemoryContainer()->insert(key, sharedMemory))
                return nullptr;
        } else {
            delete sharedMemory;
            sharedMemory = nullptr;
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
            sharedMemory = nullptr;
        }
    }

    return sharedMemory;
}

static void writeSharedMemory(SharedMemory *sharedMemory, const QImage &image)
{
    sharedMemory->lock();

    qint32 headerData[6];
    headerData[0] = qint32(image.sizeInBytes());
    headerData[1] = image.bytesPerLine();
    headerData[2] = image.size().width();
    headerData[3] = image.size().height();
    headerData[4] = image.format();
    headerData[5] = image.devicePixelRatio() * 100;

    std::memcpy(sharedMemory->data(), headerData, 24);
    std::memcpy(reinterpret_cast<char*>(sharedMemory->data()) + 24, image.constBits(), image.sizeInBytes());
    sharedMemory->unlock();
}

static void writeStream(QDataStream &out, const QImage &image)
{
    out << qint32(image.bytesPerLine());
    out << image.size();
    out << qint32(image.format());
    out << qint32(image.sizeInBytes());
    out << qint32(image.devicePixelRatio() * 100);
    out.writeRawData(reinterpret_cast<const char*>(image.constBits()), image.sizeInBytes());
}

QDataStream &operator<<(QDataStream &out, const ImageContainer &container)
{
    const int extraDataSize =  24;
    static const bool dontUseSharedMemory = qEnvironmentVariableIsSet("DESIGNER_DONT_USE_SHARED_MEMORY");

    out << container.instanceId();
    out << container.keyNumber();
    out << container.rect();

    const QImage image = container.image();

    if (dontUseSharedMemory) {
        out << qint32(0);
        writeStream(out, image);
    } else {
        const qint32 totalSize = qint32(image.sizeInBytes()) + extraDataSize;
        SharedMemory *sharedMemory = createSharedMemory(container.keyNumber(), totalSize);

        out << qint32(sharedMemory != nullptr); // send if shared memory is used

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

    if (canAttach && sharedMemory.size() >= 24)
    {
        sharedMemory.lock();
        qint32 headerData[6];
        std::memcpy(headerData, sharedMemory.constData(), 24);

        qint32 byteCount = headerData[0];
//        qint32 bytesPerLine = headerData[1];
        qint32 imageWidth = headerData[2];
        qint32 imageHeight = headerData[3];
        qint32 imageFormat = headerData[4];
        qreal pixelRatio = headerData[5] / 100.0;

        QImage image = QImage(imageWidth, imageHeight, QImage::Format(imageFormat));
        image.setDevicePixelRatio(pixelRatio);

        if (image.isNull())
            qCInfo(imageContainerDebug()) << Q_FUNC_INFO << "Not able to create image:" << imageWidth << imageHeight << imageFormat;
        else
            std::memcpy(image.bits(), reinterpret_cast<const qint32*>(sharedMemory.constData()) + 6, byteCount);

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
    qint32 pixelRatio;

    in >> bytesPerLine;
    in >> imageSize;
    in >> imageFormat;
    in >> byteCount;
    in >> pixelRatio;

    QImage image = QImage(imageSize, QImage::Format(imageFormat));

    in.readRawData(reinterpret_cast<char*>(image.bits()), byteCount);
    image.setDevicePixelRatio(pixelRatio / 100.0);

    container.setImage(image);
}

QDataStream &operator>>(QDataStream &in, ImageContainer &container)
{
    qint32 sharedMemoryIsUsed;

    in >> container.m_instanceId;
    in >> container.m_keyNumber;
    in >> container.m_rect;
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
