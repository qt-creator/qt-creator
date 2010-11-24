#ifndef PIXMAPCHANGEDCOMMAND_H
#define PIXMAPCHANGEDCOMMAND_H

#include <QMetaType>
#include <QImage>

namespace QmlDesigner {

class PixmapChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command);
public:
    PixmapChangedCommand();
    PixmapChangedCommand(qint32 instanceId, const QImage &image);

    qint32 instanceId() const;
    QImage renderImage() const;

private:
    QImage m_image;
    qint32 m_instanceId;
};

QDataStream &operator<<(QDataStream &out, const PixmapChangedCommand &command);
QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PixmapChangedCommand);

#endif // PIXMAPCHANGEDCOMMAND_H
