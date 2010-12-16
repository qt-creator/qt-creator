#ifndef PIXMAPCHANGEDCOMMAND_H
#define PIXMAPCHANGEDCOMMAND_H

#include <QMetaType>
#include "imagecontainer.h"

namespace QmlDesigner {

class PixmapChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command);
public:
    PixmapChangedCommand();
    PixmapChangedCommand(const QVector<ImageContainer> &imageVector);

    QVector<ImageContainer> images() const;

private:
    QVector<ImageContainer> m_imageVector;
};

QDataStream &operator<<(QDataStream &out, const PixmapChangedCommand &command);
QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PixmapChangedCommand)

#endif // PIXMAPCHANGEDCOMMAND_H
