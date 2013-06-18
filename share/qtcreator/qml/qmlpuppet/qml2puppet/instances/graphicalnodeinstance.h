#ifndef QMLDESIGNER_GRAPHICALNODEINSTANCE_H
#define QMLDESIGNER_GRAPHICALNODEINSTANCE_H

#include <QtGlobal>

#include "objectnodeinstance.h"

#include <designersupport.h>

namespace QmlDesigner {
namespace Internal {

class GraphicalNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<GraphicalNodeInstance> Pointer;
    typedef QWeakPointer<GraphicalNodeInstance> WeakPointer;

    ~GraphicalNodeInstance();

    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance);

    bool isGraphical() const;
    bool hasContent() const;

    QRectF boundingRect() const;
    QTransform customTransform() const;
    QTransform contentTransform() const Q_DECL_OVERRIDE;
    QTransform sceneTransform() const;
    double opacity() const;
    double rotation() const;
    double scale() const;
    QPointF transformOriginPoint() const;
    double zValue() const;
    QPointF position() const;
    QSizeF size() const;

    QImage renderImage() const;
    QImage renderPreviewImage(const QSize &previewImageSize) const;

    QList<ServerNodeInstance> childItems() const;

    void updateAllDirtyNodesRecursive();
    static void createEffectItem(bool createEffectItem);

    int penWidth() const;

    void setPropertyVariant(const PropertyName &name, const QVariant &value);
    void setPropertyBinding(const PropertyName &name, const QString &expression);

    QVariant property(const PropertyName &name) const;
    void resetProperty(const PropertyName &name) ;

    QList<ServerNodeInstance> stateInstances() const;

    bool isAnchoredByChildren() const;
    bool hasAnchor(const PropertyName &name) const;
    QPair<PropertyName, ServerNodeInstance> anchor(const PropertyName &name) const;

    void doComponentComplete();

protected:
    explicit GraphicalNodeInstance(QObject *object);
    void setHasContent(bool hasContent);
    DesignerSupport *designerSupport() const;
    Qt5NodeInstanceServer *qt5NodeInstanceServer() const;
    void updateDirtyNodesRecursive(QQuickItem *parentItem) const;
    void updateAllDirtyNodesRecursive(QQuickItem *parentItem) const;
    QRectF boundingRectWithStepChilds(QQuickItem *parentItem) const;
    void resetHorizontal();
    void resetVertical();
    QList<ServerNodeInstance> childItemsForChild(QQuickItem *childItem) const;
    void refresh();
    static bool anyItemHasContent(QQuickItem *quickItem);
    static bool childItemsHaveContent(QQuickItem *quickItem);

    double x() const;
    double y() const;

    virtual QQuickItem *quickItem() const;

private: // functions

private: // variables
    bool m_hasHeight;
    bool m_hasWidth;
    bool m_hasContent;
    double m_x;
    double m_y;
    double m_width;
    double m_height;
    static bool s_createEffectItem;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // QMLDESIGNER_GRAPHICALNODEINSTANCE_H
