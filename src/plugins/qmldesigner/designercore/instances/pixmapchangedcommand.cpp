#include "pixmapchangedcommand.h"

#include <QtDebug>

#include <QVarLengthArray>

namespace QmlDesigner {

PixmapChangedCommand::PixmapChangedCommand()
    : m_instanceId(-1)
{
}

PixmapChangedCommand::PixmapChangedCommand(qint32 instanceId, const QImage &image)
    : m_image(image), m_instanceId(instanceId)
{
}

qint32 PixmapChangedCommand::instanceId() const
{
    return m_instanceId;
}

QImage PixmapChangedCommand::renderImage() const
{
    return m_image;
}

QDataStream &operator<<(QDataStream &out, const PixmapChangedCommand &command)
{
    out << command.instanceId();

    const QImage image = command.renderImage();
    const QByteArray data(reinterpret_cast<const char*>(image.constBits()), image.byteCount());

    out << qint32(image.bytesPerLine());
    out << image.size();
    out << qint32(image.format());
    out << qint32(image.byteCount());
    out.writeRawData(reinterpret_cast<const char*>(image.constBits()), image.byteCount());

    return out;
}

QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command)
{

    qint32 byteSize;
    qint32 bytesPerLine;
    QSize imageSize;
    qint32 format;

    in >> command.m_instanceId;

    in >> bytesPerLine;
    in >> imageSize;
    in >> format;
    in >> byteSize;

    command.m_image = QImage(imageSize, QImage::Format(format));
    in.readRawData(reinterpret_cast<char*>(command.m_image.bits()), byteSize);

    return in;
}

} // namespace QmlDesigner
