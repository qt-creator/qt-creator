#include "imagecontainer.h"

namespace QmlDesigner {

ImageContainer::ImageContainer()
    : m_instanceId(-1)
{
}

ImageContainer::ImageContainer(qint32 instanceId, const QImage &image)
    : m_image(image), m_instanceId(instanceId)
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

QDataStream &operator<<(QDataStream &out, const ImageContainer &container)
{
    out << container.instanceId();

    const QImage image = container.image();
    const QByteArray data(reinterpret_cast<const char*>(image.constBits()), image.byteCount());

    out << qint32(image.bytesPerLine());
    out << image.size();
    out << qint32(image.format());
    out << qint32(image.byteCount());
    out.writeRawData(reinterpret_cast<const char*>(image.constBits()), image.byteCount());

    return out;
}

QDataStream &operator>>(QDataStream &in, ImageContainer &container)
{

    qint32 byteSize;
    qint32 bytesPerLine;
    QSize imageSize;
    qint32 format;

    in >> container.m_instanceId;

    in >> bytesPerLine;
    in >> imageSize;
    in >> format;
    in >> byteSize;

    container.m_image = QImage(imageSize, QImage::Format(format));
    in.readRawData(reinterpret_cast<char*>(container.m_image.bits()), byteSize);

    return in;
}

} // namespace QmlDesigner
