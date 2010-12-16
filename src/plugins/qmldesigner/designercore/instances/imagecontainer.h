#ifndef IMAGECONTAINER_H
#define IMAGECONTAINER_H

#include <QMetaType>
#include <QImage>

namespace QmlDesigner {

class ImageContainer
{
    friend QDataStream &operator>>(QDataStream &in, ImageContainer &container);
public:
    ImageContainer();
    ImageContainer(qint32 instanceId, const QImage &image);

    qint32 instanceId() const;
    QImage image() const;

private:
    QImage m_image;
    qint32 m_instanceId;
};

QDataStream &operator<<(QDataStream &out, const ImageContainer &container);
QDataStream &operator>>(QDataStream &in, ImageContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ImageContainer)

#endif // IMAGECONTAINER_H
