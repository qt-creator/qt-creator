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

        QColor fillColor = DefaultStyleEngine::fillColor(elementType, objectVisuals);
        QColor lineColor = DefaultStyleEngine::lineColor(elementType, objectVisuals);
        QColor textColor = DefaultStyleEngine::textColor(elementType, objectVisuals);

        QFont normalFont = baseStyle->normalFont();
        QFont headerFont = baseStyle->normalFont();
        if (objectVisuals.isEmphasized()) {
            lineWidth = 2;
            headerFont.setBold(true);
        }

        Style *style = new Style(baseStyle->type());
        QPen linePen = baseStyle->linePen();
        linePen.setColor(lineColor);
        linePen.setWidth(lineWidth);
        style->setLinePen(linePen);
        style->setInnerLinePen(linePen);
        style->setOuterLinePen(linePen);
        style->setExtraLinePen(linePen);
        style->setTextBrush(QBrush(textColor));
        if (objectVisuals.visualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
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
        if (objectVisuals.visualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
            style->setExtraFillBrush(QBrush(Qt::white));
        } else {
            style->setExtraFillBrush(QBrush(fillColor.darker(120)));
        }
        style->setNormalFont(normalFont);
        style->setSmallFont(baseStyle->smallFont());
        style->setHeaderFont(headerFont);
        m_objectStyleMap.insert(key, style);
        derivedStyle = style;
    }

    return derivedStyle;
}

const Style *DefaultStyleEngine::applyObjectStyle(const Style *baseStyle, const StyledObject &styledObject, const Parameters *parameters)
{
    ElementType elementType = objectType(styledObject.object());

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
                                          styledObject.objectVisuals().isEmphasized(),
                                          styledObject.objectVisuals().baseColor(),
                                          depth),
                            parameters);
}

const Style *DefaultStyleEngine::applyRelationStyle(const Style *baseStyle, const StyledRelation &styledRelation, const Parameters *parameters)
{
    Q_UNUSED(parameters);

    ElementType elementType = objectType(styledRelation.endA());
    RelationStyleKey key(elementType, styledRelation.endA() ? styledRelation.endA()->visualPrimaryRole() : DObject::PRIMARY_ROLE_NORMAL);
    const Style *derivedStyle = m_relationStyleMap.value(key);
    if (!derivedStyle) {
        Style *style = new Style(baseStyle->type());

        const DObject *object = styledRelation.endA();
        ObjectVisuals objectVisuals(object ? object->visualPrimaryRole() : DObject::PRIMARY_ROLE_NORMAL,
                                     object ? object->visualSecondaryRole() : DObject::SECONDARY_ROLE_NONE,
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

const Style *DefaultStyleEngine::applyAnnotationStyle(const Style *baseStyle, const DAnnotation *annotation, const Parameters *parameters)
{
    DAnnotation::VisualRole visualRole = annotation ? annotation->visualRole() : DAnnotation::ROLE_NORMAL;
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
        Style *style = new Style(baseStyle->type());
        QFont normalFont;
        QBrush textBrush = baseStyle->textBrush();
        switch (visualRole) {
        case DAnnotation::ROLE_NORMAL:
            normalFont = baseStyle->normalFont();
            break;
        case DAnnotation::ROLE_TITLE:
            normalFont = baseStyle->headerFont();
            break;
        case DAnnotation::ROLE_SUBTITLE:
            normalFont = baseStyle->normalFont();
            normalFont.setItalic(true);
            break;
        case DAnnotation::ROLE_EMPHASIZED:
            normalFont = baseStyle->normalFont();
            normalFont.setBold(true);
            break;
        case DAnnotation::ROLE_SOFTEN:
            normalFont = baseStyle->normalFont();
            textBrush.setColor(Qt::gray);
            break;
        case DAnnotation::ROLE_FOOTNOTE:
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
        Style *style = new Style(baseStyle->type());
        style->setNormalFont(baseStyle->normalFont());
        style->setTextBrush(baseStyle->textBrush());
        m_boundaryStyleMap.insert(key, style);
        derivedStyle = style;
    }
    return derivedStyle;
}

DefaultStyleEngine::ElementType DefaultStyleEngine::objectType(const DObject *object)
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

QColor DefaultStyleEngine::baseColor(ElementType elementType, ObjectVisuals objectVisuals)
{
    if (objectVisuals.visualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
        return QColor("#FFFFFF");
    }

    QColor baseColor;

    if (objectVisuals.visualPrimaryRole() == DObject::PRIMARY_ROLE_NORMAL) {
        if (objectVisuals.baseColor().isValid()) {
            baseColor = objectVisuals.baseColor();
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

        int index = (int) objectVisuals.visualPrimaryRole() - (int) DObject::PRIMARY_ROLE_CUSTOM1;
        QMT_CHECK(index >= 0 && index <= 4);
        baseColor = customColors[index];
    }

    switch (objectVisuals.visualSecondaryRole()) {
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

QColor DefaultStyleEngine::lineColor(ElementType elementType, const ObjectVisuals &objectVisuals)
{
    QColor lineColor;
    if (objectVisuals.visualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
        lineColor = Qt::black;
    } else if (objectVisuals.visualSecondaryRole() == DObject::SECONDARY_ROLE_SOFTEN) {
        lineColor = Qt::gray;
    } else {
        lineColor = baseColor(elementType, objectVisuals).darker(200).lighter(150).darker(100 + objectVisuals.depth() * 10);
    }
    return lineColor;
}

QColor DefaultStyleEngine::fillColor(ElementType elementType, const ObjectVisuals &objectVisuals)
{
    QColor fillColor;
    if (objectVisuals.visualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
        fillColor = Qt::white;
    } else {
        fillColor = baseColor(elementType, objectVisuals).lighter(150).darker(100 + objectVisuals.depth() * 10);
    }
    return fillColor;
}

QColor DefaultStyleEngine::textColor(const DObject *object, int depth)
{
    Q_UNUSED(depth);

    QColor textColor;
    DObject::VisualPrimaryRole visualRole = object ? object->visualPrimaryRole() : DObject::PRIMARY_ROLE_NORMAL;
    if (visualRole == DObject::DEPRECATED_PRIMARY_ROLE_SOFTEN) {
        textColor = Qt::gray;
    } else {
        textColor = Qt::black;
    }
    return textColor;
}

QColor DefaultStyleEngine::textColor(ElementType elementType, const ObjectVisuals &objectVisuals)
{
    Q_UNUSED(elementType);

    QColor textColor;
    if (objectVisuals.visualSecondaryRole() == DObject::SECONDARY_ROLE_SOFTEN) {
        textColor = Qt::gray;
    } else {
        textColor = Qt::black;
    }
    return textColor;
}

}
