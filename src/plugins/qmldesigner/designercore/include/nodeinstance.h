#ifndef PROXYNODEINSTANCE_H
#define PROXYNODEINSTANCE_H

#include <QSharedPointer>
#include <QTransform>
#include <QPointF>
#include <QSizeF>
#include <QPair>

#include "commondefines.h"

namespace QmlDesigner {

class ModelNode;
class NodeInstanceView;
class ProxyNodeInstanceData;

class NodeInstance
{
    friend class NodeInstanceView;
public:
    static NodeInstance create(const ModelNode &node);
    NodeInstance();
    ~NodeInstance();
    NodeInstance(const NodeInstance &other);
    NodeInstance& operator=(const NodeInstance &other);

    ModelNode modelNode() const;
    bool isValid() const;
    void makeInvalid();
    QRectF boundingRect() const;
    bool hasContent() const;
    bool isAnchoredBySibling() const;
    bool isAnchoredByChildren() const;
    bool isMovable() const;
    bool isResizable() const;
    QTransform transform() const;
    QTransform sceneTransform() const;
    bool isInPositioner() const;
    QPointF position() const;
    QSizeF size() const;
    int penWidth() const;
    void paint(QPainter *painter);

    QVariant property(const QString &name) const;
    bool hasBindingForProperty(const QString &name) const;
    QPair<QString, qint32> anchor(const QString &name) const;
    bool hasAnchor(const QString &name) const;
    QString instanceType(const QString &name) const;

    qint32 parentId() const;

protected:
    void setProperty(const QString &name, const QVariant &value);
    void setInformation(InformationName name,
                        const QVariant &information,
                        const QVariant &secondInformation,
                        const QVariant &thirdInformation);

    void setParentId(qint32 instanceId);
    void setRenderImage(const QImage &image);
    NodeInstance(ProxyNodeInstanceData *d);
    qint32 instanceId() const;

private:
    QSharedPointer<ProxyNodeInstanceData> d;
};

}

#endif // PROXYNODEINSTANCE_H
