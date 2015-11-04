/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

#include <QSet>
#include <QDebug>

namespace qmt {

// TODO use tuple instead of these 4 explicit key classes

struct ObjectStyleKey {
    ObjectStyleKey()
        : m_elementType(StyleEngine::TYPE_OTHER)
    {
    }

    ObjectStyleKey(StyleEngine::ElementType elementType, const ObjectVisuals &objectVisuals)
        : m_elementType(elementType),
          m_objectVisuals(objectVisuals)
    {
    }

    StyleEngine::ElementType m_elementType;
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


struct RelationStyleKey {
    RelationStyleKey(StyleEngine::ElementType elementType = StyleEngine::TYPE_OTHER,
                     DObject::VisualPrimaryRole visualPrimaryRole = DObject::PRIMARY_ROLE_NORMAL)
        : m_elementType(elementType),
          m_visualPrimaryRole(visualPrimaryRole)
    {
    }

    StyleEngine::ElementType m_elementType;
    DObject::VisualPrimaryRole m_visualPrimaryRole;
};

uint qHash(const RelationStyleKey &styleKey) {
    return ::qHash(styleKey.m_elementType) ^ ::qHash(styleKey.m_visualPrimaryRole);
}

bool operator==(const RelationStyleKey &lhs, const RelationStyleKey &rhs)
{
    return lhs.m_elementType == rhs.m_elementType && lhs.m_visualPrimaryRole == rhs.m_visualPrimaryRole;
}


struct AnnotationStyleKey {
    AnnotationStyleKey(DAnnotation::VisualRole visualRole = DAnnotation::ROLE_NORMAL)
        : m_visualRole(visualRole)
    {
    }

    DAnnotation::VisualRole m_visualRole;
};

uint qHash(const AnnotationStyleKey &styleKey) {
    return ::qHash(styleKey.m_visualRole);
}

bool operator==(const AnnotationStyleKey &lhs, const AnnotationStyleKey &rhs)
{
    return lhs.m_visualRole == rhs.m_visualRole;
}


struct BoundaryStyleKey {
    BoundaryStyleKey()
    {
    }
};

uint qHash(const BoundaryStyleKey &styleKey) {
    Q_UNUSED(styleKey);

    return 1;
}

bool operator==(const BoundaryStyleKey &lhs, const BoundaryStyleKey &rhs)
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

const Style *DefaultStyleEngine::applyStyle(const Style *baseStyle, StyleEngine::ElementType elementType, const StyleEngine::Parameters *parameters)
{
    switch (elementType) {
    case TYPE_ANNOTATION:
        return applyAnnotationStyle(baseStyle, DAnnotation::ROLE_NORMAL, parameters);
    case TYPE_BOUNDARY:
        return applyBoundaryStyle(baseStyle, parameters);
    case TYPE_RELATION:
        break;
    case TYPE_CLASS:
    case TYPE_COMPONENT:
    case TYPE_ITEM:
    case TYPE_PACKAGE:
        return applyObjectStyle(baseStyle, elementType, ObjectVisuals(DObject::PRIMARY_ROLE_NORMAL, DObject::SECONDARY_ROLE_NONE, false, QColor(), 0), parameters);
    case TYPE_OTHER:
        break;
    }
    return baseStyle;
}

const Style *DefaultStyleEngine::applyObjectStyle(const Style *baseStyle, StyleEngine::ElementType elementType, const ObjectVisuals &objectVisuals, const StyleEngine::Parameters *parameters)
{
    ObjectStyleKey key(elementType, objectVisuals);
    const Style *derivedStyle = m_objectStyleMap.value(key);
    if (!derivedStyle) {
        int lineWidth = 1;

        QColor fillColor = getFillColor(elementType, objectVisuals);
        QColor lineColor = getLineColor(elementType, objectVisuals);
        QColor textColor = getTextColor(elementType, objectVisuals);

        QFont normalFont = baseStyle->getNormalFont();
        QFont headerFont = baseStyle->getNormalFont();
        if (objectVisuals.isEmphasized()) {
            lineWidth = 2;
            headerFont.setBold(true);
        }

        Style *style = new Style(baseStyle->getType());
        QPen linePen = baseStyle->getLinePen();
        linePen.setColor(lineColor);
        linePen.setWidth(lineWidth);
        style->setLinePen(linePen);
        style->setInnerLinePen(linePen);
        style->setOuterLinePen(linePen);
        style->setExtraLinePen(linePen);
        style->setTextBrush(QBrush(textColor));
        if (objectVisuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
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
        if (objectVisuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
            style->setExtraFillBrush(QBrush(Qt::white));
        } else {
            style->setExtraFillBrush(QBrush(fillColor.darker(120)));
        }
        style->setNormalFont(normalFont);
        style->setSmallFont(baseStyle->getSmallFont());
        style->setHeaderFont(headerFont);
        m_objectStyleMap.insert(key, style);
        derivedStyle = style;
    }

    return derivedStyle;
}

const Style *DefaultStyleEngine::applyObjectStyle(const Style *baseStyle, const StyledObject &styledObject, const Parameters *parameters)
{
    ElementType elementType = getObjectType(styledObject.getObject());

    struct DepthProperties {
        DepthProperties()
            : m_elementType(TYPE_OTHER),
              m_visualPrimaryRole(DObject::PRIMARY_ROLE_NORMAL),
              m_visualSecondaryRole(DObject::SECONDARY_ROLE_NONE)
        {
        }

        DepthProperties(ElementType elementType, DObject::VisualPrimaryRole visualPrimaryRole, DObject::VisualSecondaryRole visualSecondaryRole)
            : m_elementType(elementType),
              m_visualPrimaryRole(visualPrimaryRole),
              m_visualSecondaryRole(visualSecondaryRole)
        {
        }

        ElementType m_elementType;
        DObject::VisualPrimaryRole m_visualPrimaryRole;
        DObject::VisualSecondaryRole m_visualSecondaryRole;
    };

    // find colliding elements which best match visual appearance of styled object
    DObject::VisualPrimaryRole styledVisualPrimaryRole = styledObject.getObjectVisuals().getVisualPrimaryRole();
    DObject::VisualSecondaryRole styledVisualSecondaryRole = styledObject.getObjectVisuals().getVisualSecondaryRole();
    QHash<int, DepthProperties> depths;
    foreach (const DObject *collidingObject, styledObject.getCollidingObjects()) {
        int collidingDepth = collidingObject->getDepth();
        if (collidingDepth < styledObject.getObject()->getDepth()) {
            ElementType collidingElementType = getObjectType(collidingObject);
            DObject::VisualPrimaryRole collidingVisualPrimaryRole = collidingObject->getVisualPrimaryRole();
            DObject::VisualSecondaryRole collidingVisualSecondaryRole = collidingObject->getVisualSecondaryRole();
            if (!depths.contains(collidingDepth)) {
                depths.insert(collidingDepth, DepthProperties(collidingElementType, collidingVisualPrimaryRole, collidingVisualSecondaryRole));
            } else {
                bool updateProperties = false;
                DepthProperties properties = depths.value(collidingDepth);
                if (properties.m_elementType != elementType && collidingElementType == elementType) {
                    properties.m_elementType = collidingElementType;
                    properties.m_visualPrimaryRole = collidingVisualPrimaryRole;
                    properties.m_visualSecondaryRole = collidingVisualSecondaryRole;
                    updateProperties = true;
                } else if (properties.m_elementType == elementType && collidingElementType == elementType) {
                    if ((properties.m_visualPrimaryRole != styledVisualPrimaryRole || properties.m_visualSecondaryRole != styledVisualSecondaryRole)
                            && collidingVisualPrimaryRole == styledVisualPrimaryRole && collidingVisualSecondaryRole == styledVisualSecondaryRole) {
                        properties.m_visualPrimaryRole = collidingVisualPrimaryRole;
                        properties.m_visualSecondaryRole = collidingVisualSecondaryRole;
                        updateProperties = true;
                    }
                }
                if (updateProperties) {
                    depths.insert(collidingDepth, properties);
                }
            }
        }
    }
    int depth = 0;
    if (!depths.isEmpty()) {
        QList<int> keys = depths.keys();
        qSort(keys);
        foreach (int d, keys) {
            DepthProperties properties = depths.value(d);
            if (properties.m_elementType == elementType && areStackingRoles(properties.m_visualPrimaryRole, properties.m_visualSecondaryRole, styledVisualPrimaryRole, styledVisualSecondaryRole)) {
                ++depth;
            } else {
                depth = 0;
            }
        }
    }

    return applyObjectStyle(baseStyle, elementType,
                            ObjectVisuals(styledVisualPrimaryRole,
                                          styledVisualSecondaryRole,
                                          styledObject.getObjectVisuals().isEmphasized(),
                                          styledObject.getObjectVisuals().getBaseColor(),
                                          depth),
                            parameters);
}

const Style *DefaultStyleEngine::applyRelationStyle(const Style *baseStyle, const StyledRelation &styledRelation, const Parameters *parameters)
{
    Q_UNUSED(parameters);

    ElementType elementType = getObjectType(styledRelation.getEndA());
    RelationStyleKey key(elementType, styledRelation.getEndA() ? styledRelation.getEndA()->getVisualPrimaryRole() : DObject::PRIMARY_ROLE_NORMAL);
    const Style *derivedStyle = m_relationStyleMap.value(key);
    if (!derivedStyle) {
        Style *style = new Style(baseStyle->getType());

        const DObject *object = styledRelation.getEndA();
        ObjectVisuals objectVisuals(object ? object->getVisualPrimaryRole() : DObject::PRIMARY_ROLE_NORMAL,
                                     object ? object->getVisualSecondaryRole() : DObject::SECONDARY_ROLE_NONE,
                                     object ? object->isVisualEmphasized() : false,
                                     Qt::black, // TODO STyledRelation should get an EndAObjectVisuals
                                     object ? object->getDepth() : 0);
        QColor lineColor = getLineColor(getObjectType(object), objectVisuals);
        QColor fillColor = lineColor;

        QPen linePen = baseStyle->getLinePen();
        linePen.setWidth(1);
        linePen.setColor(lineColor);
        style->setLinePen(linePen);
        QBrush textBrush = baseStyle->getTextBrush();
        textBrush.setColor(QColor("black"));
        style->setTextBrush(textBrush);
        QBrush brush = baseStyle->getFillBrush();
        brush.setColor(fillColor);
        brush.setStyle(Qt::SolidPattern);
        style->setFillBrush(brush);
        style->setNormalFont(baseStyle->getNormalFont());
        style->setSmallFont(baseStyle->getSmallFont());
        style->setHeaderFont(baseStyle->getHeaderFont());
        m_relationStyleMap.insert(key, style);
        derivedStyle = style;
    }
    return derivedStyle;
}

const Style *DefaultStyleEngine::applyAnnotationStyle(const Style *baseStyle, const DAnnotation *annotation, const Parameters *parameters)
{
    DAnnotation::VisualRole visualRole = annotation ? annotation->getVisualRole() : DAnnotation::ROLE_NORMAL;
    return applyAnnotationStyle(baseStyle, visualRole, parameters);
}

const Style *DefaultStyleEngine::applyBoundaryStyle(const Style *baseStyle, const DBoundary *boundary, const Parameters *parameters)
{
    Q_UNUSED(boundary);

    return applyBoundaryStyle(baseStyle, parameters);
}

const Style *DefaultStyleEngine::applyAnnotationStyle(const Style *baseStyle, DAnnotation::VisualRole visualRole, const StyleEngine::Parameters *parameters)
{
    Q_UNUSED(parameters);

    AnnotationStyleKey key(visualRole);
    const Style *derivedStyle = m_annotationStyleMap.value(key);
    if (!derivedStyle) {
        Style *style = new Style(baseStyle->getType());
        QFont normalFont;
        QBrush textBrush = baseStyle->getTextBrush();
        switch (visualRole) {
        case DAnnotation::ROLE_NORMAL:
            normalFont = baseStyle->getNormalFont();
            break;
        case DAnnotation::ROLE_TITLE:
            normalFont = baseStyle->getHeaderFont();
            break;
        case DAnnotation::ROLE_SUBTITLE:
            normalFont = baseStyle->getNormalFont();
            normalFont.setItalic(true);
            break;
        case DAnnotation::ROLE_EMPHASIZED:
            normalFont = baseStyle->getNormalFont();
            normalFont.setBold(true);
            break;
        case DAnnotation::ROLE_SOFTEN:
            normalFont = baseStyle->getNormalFont();
            textBrush.setColor(Qt::gray);
            break;
        case DAnnotation::ROLE_FOOTNOTE:
            normalFont = baseStyle->getSmallFont();
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
        Style *style = new Style(baseStyle->getType());
        style->setNormalFont(baseStyle->getNormalFont());
        style->setTextBrush(baseStyle->getTextBrush());
        m_boundaryStyleMap.insert(key, style);
        derivedStyle = style;
    }
    return derivedStyle;
}

DefaultStyleEngine::ElementType DefaultStyleEngine::getObjectType(const DObject *object)
{
    ElementType elementType;
    if (dynamic_cast<const DPackage *>(object)) {
        elementType = TYPE_PACKAGE;
    } else if (dynamic_cast<const DComponent *>(object)) {
        elementType = TYPE_COMPONENT;
    } else if (dynamic_cast<const DClass *>(object)) {
        elementType = TYPE_CLASS;
    } else if (dynamic_cast<const DItem *>(object)) {
        elementType = TYPE_ITEM;
    } else {
        elementType = TYPE_OTHER;
    }
    return elementType;
}

bool DefaultStyleEngine::areStackingRoles(DObject::VisualPrimaryRole rhsPrimaryRole, DObject::VisualSecondaryRole rhsSecondaryRole,
                                          DObject::VisualPrimaryRole lhsPrimaryRole, DObject::VisualSecondaryRole lhsSecondaryRols)
{
    switch (rhsSecondaryRole) {
    case DObject::SECONDARY_ROLE_NONE:
    case DObject::SECONDARY_ROLE_LIGHTER:
    case DObject::SECONDARY_ROLE_DARKER:
        switch (lhsSecondaryRols) {
        case DObject::SECONDARY_ROLE_NONE:
        case DObject::SECONDARY_ROLE_LIGHTER:
        case DObject::SECONDARY_ROLE_DARKER:
            return lhsPrimaryRole == rhsPrimaryRole;
            break;
        case DObject::SECONDARY_ROLE_SOFTEN:
        case DObject::SECONDARY_ROLE_OUTLINE:
            return false;
        }
    case DObject::SECONDARY_ROLE_SOFTEN:
    case DObject::SECONDARY_ROLE_OUTLINE:
        return false;
    }
    return true;
}

QColor DefaultStyleEngine::getBaseColor(ElementType elementType, ObjectVisuals objectVisuals)
{
    if (objectVisuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
        return QColor("#FFFFFF");
    }

    QColor baseColor;

    if (objectVisuals.getVisualPrimaryRole() == DObject::PRIMARY_ROLE_NORMAL) {
        if (objectVisuals.getBaseColor().isValid()) {
            baseColor = objectVisuals.getBaseColor();
        } else {
            switch (elementType) {
            case TYPE_PACKAGE:
                baseColor = QColor("#7C98AD");
                break;
            case TYPE_COMPONENT:
                baseColor = QColor("#A0A891");
                break;
            case TYPE_CLASS:
                baseColor = QColor("#E5A858");
                break;
            case TYPE_ITEM:
                baseColor = QColor("#B995C6");
                break;
            default:
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

        int index = (int) objectVisuals.getVisualPrimaryRole() - (int) DObject::PRIMARY_ROLE_CUSTOM1;
        QMT_CHECK(index >= 0 && index <= 4);
        baseColor = customColors[index];
    }

    switch (objectVisuals.getVisualSecondaryRole()) {
    case DObject::SECONDARY_ROLE_NONE:
        break;
    case DObject::SECONDARY_ROLE_LIGHTER:
        baseColor = baseColor.lighter(110);
        break;
    case DObject::SECONDARY_ROLE_DARKER:
        baseColor = baseColor.darker(120);
        break;
    case DObject::SECONDARY_ROLE_SOFTEN:
        baseColor = baseColor.lighter(300);
        break;
    case DObject::SECONDARY_ROLE_OUTLINE:
        QMT_CHECK(false);
        break;
    }

    return baseColor;
}

QColor DefaultStyleEngine::getLineColor(ElementType elementType, const ObjectVisuals &objectVisuals)
{
    QColor lineColor;
    if (objectVisuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
        lineColor = Qt::black;
    } else if (objectVisuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_SOFTEN) {
        lineColor = Qt::gray;
    } else {
        lineColor = getBaseColor(elementType, objectVisuals).darker(200).lighter(150).darker(100 + objectVisuals.getDepth() * 10);
    }
    return lineColor;
}

QColor DefaultStyleEngine::getFillColor(ElementType elementType, const ObjectVisuals &objectVisuals)
{
    QColor fillColor;
    if (objectVisuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
        fillColor = Qt::white;
    } else {
        fillColor = getBaseColor(elementType, objectVisuals).lighter(150).darker(100 + objectVisuals.getDepth() * 10);
    }
    return fillColor;
}

QColor DefaultStyleEngine::getTextColor(const DObject *object, int depth)
{
    Q_UNUSED(depth);

    QColor textColor;
    DObject::VisualPrimaryRole visualRole = object ? object->getVisualPrimaryRole() : DObject::PRIMARY_ROLE_NORMAL;
    if (visualRole == DObject::DEPRECATED_PRIMARY_ROLE_SOFTEN) {
        textColor = Qt::gray;
    } else {
        textColor = Qt::black;
    }
    return textColor;
}

QColor DefaultStyleEngine::getTextColor(ElementType elementType, const ObjectVisuals &objectVisuals)
{
    Q_UNUSED(elementType);

    QColor textColor;
    if (objectVisuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_SOFTEN) {
        textColor = Qt::gray;
    } else {
        textColor = Qt::black;
    }
    return textColor;
}

}
