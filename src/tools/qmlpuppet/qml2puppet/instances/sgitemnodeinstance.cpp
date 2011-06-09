/**************************************************************************

**

**  This  file  is  part  of  Qt  Creator

**

**  Copyright  (c)  2011  Nokia  Corporation  and/or  its  subsidiary(-ies).

**

**  Contact:  Nokia  Corporation  (qt-info@nokia.com)

**

**  No  Commercial  Usage

**

**  This  file  contains  pre-release  code  and  may  not  be  distributed.

**  You  may  use  this  file  in  accordance  with  the  terms  and  conditions

**  contained  in  the  Technology  Preview  License  Agreement  accompanying

**  this  package.

**

**  GNU  Lesser  General  Public  License  Usage

**

**  Alternatively,  this  file  may  be  used  under  the  terms  of  the  GNU  Lesser

**  General  Public  License  version  2.1  as  published  by  the  Free  Software

**  Foundation  and  appearing  in  the  file  LICENSE.LGPL  included  in  the

**  packaging  of  this  file.   Please  review  the  following  information  to

**  ensure  the  GNU  Lesser  General  Public  License  version  2.1  requirements

**  will  be  met:  http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.

**

**  In  addition,  as  a  special  exception,  Nokia  gives  you  certain  additional

**  rights.   These  rights  are  described  in  the  Nokia  Qt  LGPL  Exception

**  version  1.1,  included  in  the  file  LGPL_EXCEPTION.txt  in  this  package.

**

**  If  you  have  questions  regarding  the  use  of  this  file,  please  contact

**  Nokia  at  qt-info@nokia.com.

**

**************************************************************************/

#include "sgitemnodeinstance.h"

#if QT_VERSION >= 0x050000

#include "qt5nodeinstanceserver.h"

#include <QDeclarativeExpression>

#include <cmath>

#include <QHash>

namespace QmlDesigner {
namespace Internal {

SGItemNodeInstance::SGItemNodeInstance(QSGItem *item)
   : ObjectNodeInstance(item),
     m_hasHeight(false),
     m_hasWidth(false),
     m_isResizable(true),
     m_hasContent(true),
     m_isMovable(true),
     m_x(0.0),
     m_y(0.0),
     m_width(0.0),
     m_height(0.0)
{
}

SGItemNodeInstance::~SGItemNodeInstance()
{
    if (sgItem())
        designerSupport()->derefFromEffectItem(sgItem());
}

bool SGItemNodeInstance::hasContent() const
{
    return m_hasContent;
}

QList<ServerNodeInstance> SGItemNodeInstance::childItems() const
{
    QList<ServerNodeInstance> instanceList;

    foreach (QSGItem *childItem, sgItem()->childItems())
    {
        if (childItem && nodeInstanceServer()->hasInstanceForObject(childItem)) {
            instanceList.append(nodeInstanceServer()->instanceForObject(childItem));
        } else { //there might be an item in between the parent instance
                 //and the child instance.
                 //Popular example is flickable which has a viewport item between
                 //the flickable item and the flickable children
            instanceList.append(childItemsForChild(childItem)); //In such a case we go deeper inside the item and
                                                           //search for child items with instances.
        }
    }

    return instanceList;
}

QList<ServerNodeInstance> SGItemNodeInstance::childItemsForChild(QSGItem *childItem) const
{
    QList<ServerNodeInstance> instanceList;

    if (childItem) {
        foreach (QSGItem *childItem, childItem->childItems())
        {
            if (childItem && nodeInstanceServer()->hasInstanceForObject(childItem)) {
                instanceList.append(nodeInstanceServer()->instanceForObject(childItem));
            } else {
                instanceList.append(childItemsForChild(childItem));
            }
        }
    }
    return instanceList;
}

void SGItemNodeInstance::setHasContent(bool hasContent)
{
    m_hasContent = hasContent;
}


bool anyItemHasContent(QSGItem *graphicsItem)
{
    if (graphicsItem->flags().testFlag(QSGItem::ItemHasContents))
        return true;

    foreach (QSGItem *childItem, graphicsItem->childItems()) {
        if (anyItemHasContent(childItem))
            return true;
    }

    return false;
}

QPointF SGItemNodeInstance::position() const
{
    return sgItem()->pos();
}

QTransform SGItemNodeInstance::transform() const
{
    return DesignerSupport::parentTransform(sgItem());
}

QTransform SGItemNodeInstance::customTransform() const
{
    return QTransform();
}

QTransform SGItemNodeInstance::sceneTransform() const
{
    return DesignerSupport::canvasTransform(sgItem());
}

double SGItemNodeInstance::rotation() const
{
    return sgItem()->rotation();
}

double SGItemNodeInstance::scale() const
{
    return sgItem()->scale();
}

QPointF SGItemNodeInstance::transformOriginPoint() const
{
    return sgItem()->transformOriginPoint();
}

double SGItemNodeInstance::zValue() const
{
    return sgItem()->z();
}

double SGItemNodeInstance::opacity() const
{
    return sgItem()->opacity();
}

QObject *SGItemNodeInstance::parent() const
{
    if (!sgItem() || !sgItem()->parentItem())
        return 0;

    return sgItem()->parentItem();
}

bool SGItemNodeInstance::equalSGItem(QSGItem *item) const
{
    return item == sgItem();
}

QImage SGItemNodeInstance::renderImage() const
{
    return designerSupport()->renderImageForItem(sgItem());
}

bool SGItemNodeInstance::isMovable() const
{
    return m_isMovable && sgItem() && sgItem()->parentItem();
}

void SGItemNodeInstance::setMovable(bool movable)
{
    m_isMovable = movable;
}

SGItemNodeInstance::Pointer SGItemNodeInstance::create(QObject *object)
{
    QSGItem *sgItem = dynamic_cast<QSGItem*>(object);

    Q_ASSERT(sgItem);

    Pointer instance(new SGItemNodeInstance(sgItem));

    instance->setHasContent(anyItemHasContent(sgItem));
    sgItem->setFlag(QSGItem::ItemHasContents, true);

    if (sgItem->inherits("QSGText"))
        instance->setResizable(false);

    static_cast<QDeclarativeParserStatus*>(sgItem)->classBegin();

    instance->populateResetValueHash();

    return instance;
}

void SGItemNodeInstance::initialize(const Pointer &objectNodeInstance)
{
    designerSupport()->refFromEffectItem(sgItem());
    ObjectNodeInstance::initialize(objectNodeInstance);
}

bool SGItemNodeInstance::isSGItem() const
{
    return true;
}

QSizeF SGItemNodeInstance::size() const
{
    if (isValid()) {
        double implicitWidth = sgItem()->implicitWidth();
        if (!m_hasWidth
            && implicitWidth // WORKAROUND
            && sgItem()->width() <= 0
            && implicitWidth != sgItem()->width()
            && !hasBindingForProperty("width")) {
            sgItem()->blockSignals(true);
            sgItem()->setWidth(implicitWidth);
            sgItem()->blockSignals(false);
        }

        double implicitHeight = sgItem()->implicitHeight();
        if (!m_hasHeight
            && implicitWidth // WORKAROUND
            && sgItem()->height() <= 0
            && implicitHeight != sgItem()->height()
            && !hasBindingForProperty("height")) {
            sgItem()->blockSignals(true);
            sgItem()->setHeight(implicitHeight);
            sgItem()->blockSignals(false);
        }

    }

    if (isRootNodeInstance()) {
        if (!m_hasWidth) {
            sgItem()->blockSignals(true);
            if (sgItem()->width() < 10.)
                sgItem()->setWidth(100.);
            sgItem()->blockSignals(false);
        }

        if (!m_hasHeight) {
            sgItem()->blockSignals(true);
            if (sgItem()->height() < 10.)
                sgItem()->setHeight(100.);
            sgItem()->blockSignals(false);
        }
    }

    return QSizeF(sgItem()->width(), sgItem()->height());
}

QRectF SGItemNodeInstance::boundingRect() const
{
    if (isValid()) {
        double implicitWidth = sgItem()->implicitWidth();
        if (!m_hasWidth
            && implicitWidth // WORKAROUND
            && sgItem()->width() <= 0
            && implicitWidth != sgItem()->width()
            && !hasBindingForProperty("width")) {
            sgItem()->blockSignals(true);
            sgItem()->setWidth(implicitWidth);
            sgItem()->blockSignals(false);
        }

        double implicitHeight = sgItem()->implicitHeight();
        if (!m_hasHeight
            && implicitWidth // WORKAROUND
            && sgItem()->height() <= 0
            && implicitHeight != sgItem()->height()
            && !hasBindingForProperty("height")) {
            sgItem()->blockSignals(true);
            sgItem()->setHeight(implicitHeight);
            sgItem()->blockSignals(false);
        }

    }

    if (isRootNodeInstance()) {
        if (!m_hasWidth) {
            sgItem()->blockSignals(true);
            if (sgItem()->width() < 10.)
                sgItem()->setWidth(100.);
            sgItem()->blockSignals(false);
        }

        if (!m_hasHeight) {
            sgItem()->blockSignals(true);
            if (sgItem()->height() < 10.)
                sgItem()->setHeight(100.);
            sgItem()->blockSignals(false);
        }
    }

    if (sgItem())
        return sgItem()->boundingRect();

    return QRectF();
}

void SGItemNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "state")
        return; // states are only set by us

    if (name == "height") {
        m_height = value.toDouble();
       if (value.isValid())
           m_hasHeight = true;
       else
           m_hasHeight = false;
    }

    if (name == "width") {
       m_width = value.toDouble();
       if (value.isValid())
           m_hasWidth = true;
       else
           m_hasWidth = false;
    }

    if (name == "x")
        m_x = value.toDouble();

    if (name == "y")
        m_y = value.toDouble();

    ObjectNodeInstance::setPropertyVariant(name, value);

    refresh();
}

void SGItemNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    if (name == "state")
        return; // states are only set by us

    ObjectNodeInstance::setPropertyBinding(name, expression);
}

QVariant SGItemNodeInstance::property(const QString &name) const
{
   if (name == "width" && !hasBindingForProperty("width")) {
        double implicitWidth = sgItem()->implicitWidth();
        if (!m_hasWidth
            && implicitWidth // WORKAROUND
            && sgItem()->width() <= 0
            && implicitWidth != sgItem()->width()) {
                sgItem()->blockSignals(true);
                sgItem()->setWidth(implicitWidth);
                sgItem()->blockSignals(false);
        }
    }

    if (name == "height" && !hasBindingForProperty("height")) {
        double implicitHeight = sgItem()->implicitHeight();
        if (!m_hasHeight
            && implicitHeight // WORKAROUND
            && sgItem()->width() <= 0
            && implicitHeight != sgItem()->height()) {
                sgItem()->blockSignals(true);
                sgItem()->setHeight(implicitHeight);
                sgItem()->blockSignals(false);
            }
    }

    return ObjectNodeInstance::property(name);
}

void SGItemNodeInstance::resetHorizontal()
 {
    setPropertyVariant("x", m_x);
    if (m_width > 0.0) {
        setPropertyVariant("width", m_width);
    } else {
        setPropertyVariant("width", sgItem()->implicitWidth());
    }
}

void SGItemNodeInstance::resetVertical()
 {
    setPropertyVariant("y", m_y);
    if (m_height > 0.0) {
        setPropertyVariant("height", m_height);
    } else {
        setPropertyVariant("height", sgItem()->implicitWidth());
    }
}

static void repositioning(QSGItem *item)
{
    if (!item)
        return;

//    QDeclarativeBasePositioner *positioner = qobject_cast<QDeclarativeBasePositioner*>(item);
//    if (positioner)
//        positioner->rePositioning();

    if (item->parentItem())
        repositioning(item->parentItem());
}

void SGItemNodeInstance::refresh()
{
    repositioning(sgItem());
}

void SGItemNodeInstance::doComponentComplete()
{
    if (sgItem()) {
        if (DesignerSupport::isComponentComplete(sgItem()))
            return;
        static_cast<QDeclarativeParserStatus*>(sgItem())->componentComplete();
    }

    sgItem()->update();
}

bool SGItemNodeInstance::isResizable() const
{
    return m_isResizable && sgItem() && sgItem()->parentItem();
}

void SGItemNodeInstance::setResizable(bool resizeable)
{
    m_isResizable = resizeable;
}

int SGItemNodeInstance::penWidth() const
{
    return DesignerSupport::borderWidth(sgItem());
}

void SGItemNodeInstance::resetProperty(const QString &name)
{
    if (name == "height") {
        m_hasHeight = false;
        m_height = 0.0;
    }

    if (name == "width") {
        m_hasWidth = false;
        m_width = 0.0;
    }

    if (name == "x")
        m_x = 0.0;

    if (name == "y")
        m_y = 0.0;

    DesignerSupport::resetAnchor(sgItem(), name);

    if (name == "anchors.fill") {
        resetHorizontal();
        resetVertical();
    } else if (name == "anchors.centerIn") {
        resetHorizontal();
        resetVertical();
    } else if (name == "anchors.top") {
        resetVertical();
    } else if (name == "anchors.left") {
        resetHorizontal();
    } else if (name == "anchors.right") {
        resetHorizontal();
    } else if (name == "anchors.bottom") {
        resetVertical();
    } else if (name == "anchors.horizontalCenter") {
        resetHorizontal();
    } else if (name == "anchors.verticalCenter") {
        resetVertical();
    } else if (name == "anchors.baseline") {
        resetVertical();
    }

    ObjectNodeInstance::resetProperty(name);
}

void SGItemNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const QString &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const QString &newParentProperty)
{
    if (oldParentInstance && oldParentInstance->isPositioner()) {
        setInPositioner(false);
        setMovable(true);
    }

    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    if (newParentInstance && newParentInstance->isPositioner()) {
        setInPositioner(true);
        setMovable(false);
    }

    if (oldParentInstance && oldParentInstance->isPositioner() && !(newParentInstance && newParentInstance->isPositioner())) {
        if (!hasBindingForProperty("x"))
            setPropertyVariant("x", m_x);

        if (!hasBindingForProperty("y"))
            setPropertyVariant("y", m_y);
    }

    refresh();
}

static bool isValidAnchorName(const QString &name)
{
    static QStringList anchorNameList(QStringList() << "anchors.top"
                                                    << "anchors.left"
                                                    << "anchors.right"
                                                    << "anchors.bottom"
                                                    << "anchors.verticalCenter"
                                                    << "anchors.horizontalCenter"
                                                    << "anchors.fill"
                                                    << "anchors.centerIn"
                                                    << "anchors.baseline");

    return anchorNameList.contains(name);
}

bool SGItemNodeInstance::hasAnchor(const QString &name) const
{
    return DesignerSupport::hasAnchor(sgItem(), name);
}

QPair<QString, ServerNodeInstance> SGItemNodeInstance::anchor(const QString &name) const
{
    if (!isValidAnchorName(name) || !DesignerSupport::hasAnchor(sgItem(), name))
        return ObjectNodeInstance::anchor(name);

    QPair<QString, QObject*> nameObjectPair = DesignerSupport::anchorLineTarget(sgItem(), name, context());

    QObject *targetObject = nameObjectPair.second;
    QString targetName = nameObjectPair.first;

    if (targetObject && nodeInstanceServer()->hasInstanceForObject(targetObject)) {
        return qMakePair(targetName, nodeInstanceServer()->instanceForObject(targetObject));
    } else {
        return ObjectNodeInstance::anchor(name);
    }
}

QList<ServerNodeInstance> SGItemNodeInstance::stateInstances() const
{
    QList<ServerNodeInstance> instanceList;
    QList<QObject*> stateList = DesignerSupport::statesForItem(sgItem());
    foreach (QObject *state, stateList)
    {
        if (state && nodeInstanceServer()->hasInstanceForObject(state))
            instanceList.append(nodeInstanceServer()->instanceForObject(state));
    }

    return instanceList;
}

bool SGItemNodeInstance::isAnchoredBySibling() const
{
    if (sgItem()->parentItem()) {
        foreach (QSGItem *siblingItem, sgItem()->parentItem()->childItems()) { // search in siblings for a anchor to this item
            if (siblingItem) {
                if (DesignerSupport::isAnchoredTo(siblingItem, sgItem()))
                    return true;
            }
        }
    }

    return false;
}

bool SGItemNodeInstance::isAnchoredByChildren() const
{
    if (DesignerSupport::areChildrenAnchoredTo(sgItem(), sgItem())) // search in children for a anchor to this item
        return true;

    return false;
}

QSGItem *SGItemNodeInstance::sgItem() const
{
    if (object() == 0)
        return 0;

    Q_ASSERT(qobject_cast<QSGItem*>(object()));
    return static_cast<QSGItem*>(object());
}

DesignerSupport *SGItemNodeInstance::designerSupport() const
{
    return qt5NodeInstanceServer()->designerSupport();
}

Qt5NodeInstanceServer *SGItemNodeInstance::qt5NodeInstanceServer() const
{
    return qobject_cast<Qt5NodeInstanceServer*>(nodeInstanceServer());
}

} // namespace Internal
} // namespace QmlDesigner

#endif //QT_VERSION
