#ifndef COMMONDEFINES_H
#define COMMONDEFINES_H

#include <QMetaType>

namespace QmlDesigner {

enum InformationName
{
    NoName,
    Size,
    BoundingRect,
    Transform,
    HasAnchor,
    Anchor,
    InstanceTypeForProperty,
    PenWidth,
    Position,
    IsInPositioner,
    SceneTransform,
    IsResizable,
    IsMovable,
    IsAnchoredByChildren,
    IsAnchoredBySibling,
    HasContent,
    HasBindingForProperty
};

}

#endif // COMMONDEFINES_H
