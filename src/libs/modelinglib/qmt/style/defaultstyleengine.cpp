/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "defaultstyleengine.h"

#include "defaultstyle.h"
#include "objectvisuals.h"
#include "styledobject.h"
#include "styledrelation.h"

#include "qmt/diagram/dclass.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram/dcomponent.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/dannotation.h"
#include "qmt/infrastructure/qmtassert.h"

#include <utils/algorithm.h>

#include <QSet>

namespace {

class DepthProperties
{
public:
    DepthProperties() = default;
    DepthProperties(qmt::DefaultStyleEngine::ElementType elementType,
                    qmt::DObject::VisualPrimaryRole visualPrimaryRole,
                    qmt::DObject::VisualSecondaryRole visualSecondaryRole)
        : m_elementType(elementType),
          m_visualPrimaryRole(visualPrimaryRole),
          m_visualSecondaryRole(visualSecondaryRole)
    {
    }

    qmt::DefaultStyleEngine::ElementType m_elementType = qmt::DefaultStyleEngine::TypeOther;
    qmt::DObject::VisualPrimaryRole m_visualPrimaryRole = qmt::DObject::PrimaryRoleNormal;
    qmt::DObject::VisualSecondaryRole m_visualSecondaryRole = qmt::DObject::SecondaryRoleNone;
};

} // namespace

namespace qmt {

// TODO use tuple instead of these 4 explicit key classes

class ObjectStyleKey
{
public:
    ObjectStyleKey() = default;

    ObjectStyleKey(StyleEngine::ElementType elementType, const ObjectVisuals &objectVisuals)
        : m_elementType(elementType),
          m_objectVisuals(objectVisuals)
    {
    }

    StyleEngine::ElementType m_elementType = StyleEngine::TypeOther;
    ObjectVisuals m_objectVisuals;
};

uint qHash(const ObjectStyleKey &styleKey)
{
    return ::qHash(styleKey.m_elementType) ^ qHash(styleKey.m_objectVisuals);
}

bool operator==(const ObjectStyleKey &lhs, const ObjectStyleKey &rhs)
{
    return lhs.m_elementType == rhs.m_elementType && lhs.m_objectVisuals == rhs.m_objectVisuals;
}

class RelationStyleKey
{
public:
    RelationStyleKey(StyleEngine::ElementType elementType = StyleEngine::TypeOther,
                     DObject::VisualPrimaryRole visualPrimaryRole = DObject::PrimaryRoleNormal)
        : m_elementType(elementType),
          m_visualPrimaryRole(visualPrimaryRole)
    {
    }

    StyleEngine::ElementType m_elementType = StyleEngine::TypeOther;
    DObject::VisualPrimaryRole m_visualPrimaryRole = DObject::PrimaryRoleNormal;
};

uint qHash(const RelationStyleKey &styleKey)
{
    return ::qHash(styleKey.m_elementType) ^ ::qHash(styleKey.m_visualPrimaryRole);
}

bool operator==(const RelationStyleKey &lhs, const RelationStyleKey &rhs)
{
    return lhs.m_elementType == rhs.m_elementType && lhs.m_visualPrimaryRole == rhs.m_visualPrimaryRole;
}

class AnnotationStyleKey
{
public:
    AnnotationStyleKey(DAnnotation::VisualRole visualRole = DAnnotation::RoleNormal)
        : m_visualRole(visualRole)
    {
    }

    DAnnotation::VisualRole m_visualRole = DAnnotation::RoleNormal;
};

uint qHash(const AnnotationStyleKey &styleKey)
{
    return ::qHash(styleKey.m_visualRole);
}

bool operator==(const AnnotationStyleKey &lhs, const AnnotationStyleKey &rhs)
{
    return lhs.m_visualRole == rhs.m_visualRole;
}

// TODO remove class if no attributes needed even with future extensions
class BoundaryStyleKey
{
};

uint qHash(const BoundaryStyleKey &styleKey)
{
    Q_UNUSED(styleKey);

    return 1;
}

bool operator==(const BoundaryStyleKey &lhs, const BoundaryStyleKey &rhs)
{
    Q_UNUSED(lhs);
    Q_UNUSED(rhs);

    return true;
}

// TODO remove class if no attributes needed even with future extensions
class SwimlaneStyleKey
{
};

uint qHash(const SwimlaneStyleKey &styleKey)
{
    Q_UNUSED(styleKey);

    return 1;
}

bool operator==(const SwimlaneStyleKey &lhs, const SwimlaneStyleKey &rhs)
{
    Q_UNUSED(lhs);
    Q_UNUSED(rhs);

    return true;
}

DefaultStyleEngine::DefaultStyleEngine()
{
}

DefaultStyleEngine::~DefaultStyleEngine()
{
    qDeleteAll(m_objectStyleMap);
    qDeleteAll(m_relationStyleMap);
    qDeleteAll(m_annotationStyleMap);
    qDeleteAll(m_boundaryStyleMap);
}

const Style *DefaultStyleEngine::applyStyle(const Style *baseStyle, StyleEngine::ElementType elementType,
                                            const StyleEngine::Parameters *parameters)
{
    switch (elementType) {
    case TypeAnnotation:
        return applyAnnotationStyle(baseStyle, DAnnotation::RoleNormal, parameters);
    case TypeBoundary:
        return applyBoundaryStyle(baseStyle, parameters);
    case TypeRelation:
        break;
    case TypeClass:
    case TypeComponent:
    case TypeItem:
    case TypePackage:
        return applyObjectStyle(
                    baseStyle, elementType,
                    ObjectVisuals(DObject::PrimaryRoleNormal, DObject::SecondaryRoleNone, false, QColor(), 0),
                    parameters);
    case TypeOther:
        break;
    case TypeSwimlane:
        return applySwimlaneStyle(baseStyle, parameters);
    }
    return baseStyle;
}

const Style *DefaultStyleEngine::applyObjectStyle(const Style *baseStyle, StyleEngine::ElementType elementType,
                                                  const ObjectVisuals &objectVisuals,
                                                  const StyleEngine::Parameters *parameters)
{
    ObjectStyleKey key(elementType, objectVisuals);
    const Style *derivedStyle = m_objectStyleMap.value(key);
    if (!derivedStyle) {
        int lineWidth = 1;

        QColor fillColor = DefaultStyleEngine::fillColor(elementType, objectVisuals);
        QColor lineColor = DefaultStyleEngine::lineColor(elementType, objectVisuals);
        QColor textColor = DefaultStyleEngine::textColor(elementType, objectVisuals);

        QFont normalFont = baseStyle->normalFont();
        QFont headerFont = baseStyle->normalFont();
        if (objectVisuals.isEmphasized()) {
            lineWidth = 2;
            headerFont.setBold(true);
        }

        auto style = new Style(baseStyle->type());
        QPen linePen = baseStyle->linePen();
        linePen.setColor(lineColor);
        linePen.setWidth(lineWidth);
        style->setLinePen(linePen);
        style->setInnerLinePen(linePen);
        style->setOuterLinePen(linePen);
        style->setExtraLinePen(linePen);
        style->setTextBrush(QBrush(textColor));
        if (objectVisuals.visualSecondaryRole() == DObject::SecondaryRoleOutline) {
            style->setFillBrush(QBrush(Qt::white));
        } else {
            if (!parameters->suppressGradients()) {
                QLinearGradient fillGradient(0.0, 0.0, 0.0, 1.0);
                fillGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
                fillGradient.setColorAt(0.0, fillColor.lighter(110));
                fillGradient.setColorAt(1.0, fillColor.darker(110));
                style->setFillBrush(QBrush(fillGradient));
            } else {
                style->setFillBrush(QBrush(fillColor));
            }
        }
        if (objectVisuals.visualSecondaryRole() == DObject::SecondaryRoleOutline)
            style->setExtraFillBrush(QBrush(Qt::white));
        else
            style->setExtraFillBrush(QBrush(fillColor.darker(120)));
        style->setNormalFont(normalFont);
        style->setSmallFont(baseStyle->smallFont());
        style->setHeaderFont(headerFont);
        m_objectStyleMap.insert(key, style);
        derivedStyle = style;
    }

    return derivedStyle;
}

const Style *DefaultStyleEngine::applyObjectStyle(const Style *baseStyle, const StyledObject &styledObject,
                                                  const Parameters *parameters)
{
    ElementType elementType = objectType(styledObject.object());

    // find colliding elements which best match visual appearance of styled object
    DObject::VisualPrimaryRole styledVisualPrimaryRole = styledObject.objectVisuals().visualPrimaryRole();
    DObject::VisualSecondaryRole styledVisualSecondaryRole = styledObject.objectVisuals().visualSecondaryRole();
    QHash<int, DepthProperties> depths;
    foreach (const DObject *collidingObject, styledObject.collidingObjects()) {
        int collidingDepth = collidingObject->depth();
        if (collidingDepth < styledObject.object()->depth()) {
            ElementType collidingElementType = objectType(collidingObject);
            DObject::VisualPrimaryRole collidingVisualPrimaryRole = collidingObject->visualPrimaryRole();
            DObject::VisualSecondaryRole collidingVisualSecondaryRole = collidingObject->visualSecondaryRole();
            if (!depths.contains(collidingDepth)) {
                depths.insert(collidingDepth, DepthProperties(collidingElementType, collidingVisualPrimaryRole,
                                                              collidingVisualSecondaryRole));
            } else {
                bool updateProperties = false;
                DepthProperties properties = depths.value(collidingDepth);
                if (properties.m_elementType != elementType && collidingElementType == elementType) {
                    properties.m_elementType = collidingElementType;
                    properties.m_visualPrimaryRole = collidingVisualPrimaryRole;
                    properties.m_visualSecondaryRole = collidingVisualSecondaryRole;
                    updateProperties = true;
                } else if (properties.m_elementType == elementType && collidingElementType == elementType) {
                    if ((properties.m_visualPrimaryRole != styledVisualPrimaryRole
                         || properties.m_visualSecondaryRole != styledVisualSecondaryRole)
                            && collidingVisualPrimaryRole == styledVisualPrimaryRole
                            && collidingVisualSecondaryRole == styledVisualSecondaryRole) {
                        properties.m_visualPrimaryRole = collidingVisualPrimaryRole;
                        properties.m_visualSecondaryRole = collidingVisualSecondaryRole;
                        updateProperties = true;
                    }
                }
                if (updateProperties)
                    depths.insert(collidingDepth, properties);
            }
        }
    }
    int depth = 0;
    if (!depths.isEmpty()) {
        QList<int> keys = depths.keys();
        Utils::sort(keys);
        foreach (int d, keys) {
            DepthProperties properties = depths.value(d);
            if (properties.m_elementType == elementType
                    && areStackingRoles(properties.m_visualPrimaryRole, properties.m_visualSecondaryRole,
                                        styledVisualPrimaryRole, styledVisualSecondaryRole)) {
                ++depth;
            } else {
                depth = 0;
            }
        }
    }

    return applyObjectStyle(baseStyle, elementType,
                            ObjectVisuals(styledVisualPrimaryRole,
                                          styledVisualSecondaryRole,
                                          styledObject.objectVisuals().isEmphasized(),
                                          styledObject.objectVisuals().baseColor(),
                                          depth),
                            parameters);
}

const Style *DefaultStyleEngine::applyRelationStyle(const Style *baseStyle, const StyledRelation &styledRelation,
                                                    const Parameters *parameters)
{
    Q_UNUSED(parameters);

    ElementType elementType = objectType(styledRelation.endA());
    RelationStyleKey key(elementType, styledRelation.endA() ? styledRelation.endA()->visualPrimaryRole() : DObject::PrimaryRoleNormal);
    const Style *derivedStyle = m_relationStyleMap.value(key);
    if (!derivedStyle) {
        auto style = new Style(baseStyle->type());

        const DObject *object = styledRelation.endA();
        ObjectVisuals objectVisuals(object ? object->visualPrimaryRole() : DObject::PrimaryRoleNormal,
                                     object ? object->visualSecondaryRole() : DObject::SecondaryRoleNone,
                                     object ? object->isVisualEmphasized() : false,
                                     Qt::black, // TODO STyledRelation should get an EndAObjectVisuals
                                     object ? object->depth() : 0);
        QColor lineColor = DefaultStyleEngine::lineColor(objectType(object), objectVisuals);
        QColor fillColor = lineColor;

        QPen linePen = baseStyle->linePen();
        linePen.setWidth(1);
        linePen.setColor(lineColor);
        style->setLinePen(linePen);
        QBrush textBrush = baseStyle->textBrush();
        textBrush.setColor(QColor("black"));
        style->setTextBrush(textBrush);
        QBrush brush = baseStyle->fillBrush();
        brush.setColor(fillColor);
        brush.setStyle(Qt::SolidPattern);
        style->setFillBrush(brush);
        style->setNormalFont(baseStyle->normalFont());
        style->setSmallFont(baseStyle->smallFont());
        style->setHeaderFont(baseStyle->headerFont());
        m_relationStyleMap.insert(key, style);
        derivedStyle = style;
    }
    return derivedStyle;
}

const Style *DefaultStyleEngine::applyAnnotationStyle(const Style *baseStyle, const DAnnotation *annotation,
                                                      const Parameters *parameters)
{
    DAnnotation::VisualRole visualRole = annotation ? annotation->visualRole() : DAnnotation::RoleNormal;
    return applyAnnotationStyle(baseStyle, visualRole, parameters);
}

const Style *DefaultStyleEngine::applyBoundaryStyle(const Style *baseStyle, const DBoundary *boundary,
                                                    const Parameters *parameters)
{
    Q_UNUSED(boundary);

    return applyBoundaryStyle(baseStyle, parameters);
}

const Style *DefaultStyleEngine::applySwimlaneStyle(const Style *baseStyle, const DSwimlane *swimlane, const StyleEngine::Parameters *parameters)
{
    Q_UNUSED(swimlane);

    return applySwimlaneStyle(baseStyle, parameters);
}

const Style *DefaultStyleEngine::applyAnnotationStyle(const Style *baseStyle, DAnnotation::VisualRole visualRole,
                                                      const StyleEngine::Parameters *parameters)
{
    Q_UNUSED(parameters);

    AnnotationStyleKey key(visualRole);
    const Style *derivedStyle = m_annotationStyleMap.value(key);
    if (!derivedStyle) {
        auto style = new Style(baseStyle->type());
        QFont normalFont;
        QBrush textBrush = baseStyle->textBrush();
        switch (visualRole) {
        case DAnnotation::RoleNormal:
            normalFont = baseStyle->normalFont();
            break;
        case DAnnotation::RoleTitle:
            normalFont = baseStyle->headerFont();
            break;
        case DAnnotation::RoleSubtitle:
            normalFont = baseStyle->normalFont();
            normalFont.setItalic(true);
            break;
        case DAnnotation::RoleEmphasized:
            normalFont = baseStyle->normalFont();
            normalFont.setBold(true);
            break;
        case DAnnotation::RoleSoften:
            normalFont = baseStyle->normalFont();
            textBrush.setColor(Qt::gray);
            break;
        case DAnnotation::RoleFootnote:
            normalFont = baseStyle->smallFont();
            break;
        }
        style->setNormalFont(normalFont);
        style->setTextBrush(textBrush);
        m_annotationStyleMap.insert(key, style);
        derivedStyle = style;
    }
    return derivedStyle;
}

const Style *DefaultStyleEngine::applyBoundaryStyle(const Style *baseStyle, const StyleEngine::Parameters *parameters)
{
    Q_UNUSED(parameters);

    BoundaryStyleKey key;
    const Style *derivedStyle = m_boundaryStyleMap.value(key);
    if (!derivedStyle) {
        auto style = new Style(baseStyle->type());
        style->setNormalFont(baseStyle->normalFont());
        style->setTextBrush(baseStyle->textBrush());
        m_boundaryStyleMap.insert(key, style);
        derivedStyle = style;
    }
    return derivedStyle;
}

const Style *DefaultStyleEngine::applySwimlaneStyle(const Style *baseStyle, const StyleEngine::Parameters *parameters)
{
    Q_UNUSED(parameters);

    SwimlaneStyleKey key;
    const Style *derivedStyle = m_swimlaneStyleMap.value(key);
    if (!derivedStyle) {
        auto style = new Style(baseStyle->type());
        style->setNormalFont(baseStyle->normalFont());
        style->setTextBrush(baseStyle->textBrush());
        m_swimlaneStyleMap.insert(key, style);
        derivedStyle = style;
    }
    return derivedStyle;
}

DefaultStyleEngine::ElementType DefaultStyleEngine::objectType(const DObject *object)
{
    ElementType elementType;
    if (dynamic_cast<const DPackage *>(object))
        elementType = TypePackage;
    else if (dynamic_cast<const DComponent *>(object))
        elementType = TypeComponent;
    else if (dynamic_cast<const DClass *>(object))
        elementType = TypeClass;
    else if (dynamic_cast<const DItem *>(object))
        elementType = TypeItem;
    else
        elementType = TypeOther;
    return elementType;
}

bool DefaultStyleEngine::areStackingRoles(DObject::VisualPrimaryRole rhsPrimaryRole,
                                          DObject::VisualSecondaryRole rhsSecondaryRole,
                                          DObject::VisualPrimaryRole lhsPrimaryRole,
                                          DObject::VisualSecondaryRole lhsSecondaryRols)
{
    switch (rhsSecondaryRole) {
    case DObject::SecondaryRoleNone:
    case DObject::SecondaryRoleLighter:
    case DObject::SecondaryRoleDarker:
        switch (lhsSecondaryRols) {
        case DObject::SecondaryRoleNone:
        case DObject::SecondaryRoleLighter:
        case DObject::SecondaryRoleDarker:
            return lhsPrimaryRole == rhsPrimaryRole;
        case DObject::SecondaryRoleSoften:
        case DObject::SecondaryRoleOutline:
            return false;
        }
    case DObject::SecondaryRoleSoften:
    case DObject::SecondaryRoleOutline:
        return false;
    }
    return true;
}

QColor DefaultStyleEngine::baseColor(ElementType elementType, ObjectVisuals objectVisuals)
{
    if (objectVisuals.visualSecondaryRole() == DObject::SecondaryRoleOutline)
        return QColor("#FFFFFF");

    QColor baseColor;

    if (objectVisuals.visualPrimaryRole() == DObject::PrimaryRoleNormal) {
        if (objectVisuals.baseColor().isValid()) {
            baseColor = objectVisuals.baseColor();
        } else {
            switch (elementType) {
            case TypePackage:
                baseColor = QColor("#7C98AD");
                break;
            case TypeComponent:
                baseColor = QColor("#A0A891");
                break;
            case TypeClass:
                baseColor = QColor("#E5A858");
                break;
            case TypeItem:
                baseColor = QColor("#B995C6");
                break;
            case TypeRelation:
            case TypeAnnotation:
            case TypeBoundary:
            case TypeSwimlane:
            case TypeOther:
                baseColor = QColor("#BF7D65");
                break;
            }
        }
    } else {
        static QColor customColors[] = {
            QColor("#EE8E99").darker(110),  // ROLE_CUSTOM1,
            QColor("#80AF47").lighter(130), // ROLE_CUSTOM2,
            QColor("#FFA15B").lighter(100), // ROLE_CUSTOM3,
            QColor("#55C4CF").lighter(120), // ROLE_CUSTOM4,
            QColor("#FFE14B")               // ROLE_CUSTOM5,
        };

        int index = static_cast<int>(objectVisuals.visualPrimaryRole()) - static_cast<int>(DObject::PrimaryRoleCustom1);
        QMT_ASSERT(index >= 0 && index <= 4, return baseColor);
        baseColor = customColors[index];
    }

    switch (objectVisuals.visualSecondaryRole()) {
    case DObject::SecondaryRoleNone:
        break;
    case DObject::SecondaryRoleLighter:
        baseColor = baseColor.lighter(110);
        break;
    case DObject::SecondaryRoleDarker:
        baseColor = baseColor.darker(120);
        break;
    case DObject::SecondaryRoleSoften:
        baseColor = baseColor.lighter(300);
        break;
    case DObject::SecondaryRoleOutline:
        QMT_CHECK(false);
        break;
    }

    return baseColor;
}

QColor DefaultStyleEngine::lineColor(ElementType elementType, const ObjectVisuals &objectVisuals)
{
    QColor lineColor;
    if (objectVisuals.visualSecondaryRole() == DObject::SecondaryRoleOutline)
        lineColor = Qt::black;
    else if (objectVisuals.visualSecondaryRole() == DObject::SecondaryRoleSoften)
        lineColor = Qt::gray;
    else
        lineColor = baseColor(elementType, objectVisuals).darker(200).lighter(150).darker(100 + objectVisuals.depth() * 10);
    return lineColor;
}

QColor DefaultStyleEngine::fillColor(ElementType elementType, const ObjectVisuals &objectVisuals)
{
    QColor fillColor;
    if (objectVisuals.visualSecondaryRole() == DObject::SecondaryRoleOutline)
        fillColor = Qt::white;
    else
        fillColor = baseColor(elementType, objectVisuals).lighter(150).darker(100 + objectVisuals.depth() * 10);
    return fillColor;
}

QColor DefaultStyleEngine::textColor(const DObject *object, int depth)
{
    Q_UNUSED(depth);

    QColor textColor;
    DObject::VisualPrimaryRole visualRole = object ? object->visualPrimaryRole() : DObject::PrimaryRoleNormal;
    if (visualRole == DObject::DeprecatedPrimaryRoleSoften)
        textColor = Qt::gray;
    else
        textColor = Qt::black;
    return textColor;
}

QColor DefaultStyleEngine::textColor(ElementType elementType, const ObjectVisuals &objectVisuals)
{
    Q_UNUSED(elementType);

    QColor textColor;
    if (objectVisuals.visualSecondaryRole() == DObject::SecondaryRoleSoften)
        textColor = Qt::gray;
    else
        textColor = Qt::black;
    return textColor;
}

} // namespace qmt
