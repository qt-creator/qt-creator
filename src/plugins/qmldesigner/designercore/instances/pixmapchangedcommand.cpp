#include "pixmapchangedcommand.h"

#include <QtDebug>

#include <QVarLengthArray>

namespace QmlDesigner {

PixmapChangedCommand::PixmapChangedCommand()
{
}

PixmapChangedCommand::PixmapChangedCommand(const QVector<ImageContainer> &imageVector)
    : m_imageVector(imageVector)
{
}

QVector<ImageContainer> PixmapChangedCommand::images() const
{
    return m_imageVector;
}

QDataStream &operator<<(QDataStream &out, const PixmapChangedCommand &command)
{
    out << command.images();

    return out;
}

QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command)
{
    in >> command.m_imageVector;

    return in;
}

} // namespace QmlDesigner
