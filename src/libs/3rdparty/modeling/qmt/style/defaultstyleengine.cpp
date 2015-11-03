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

    ObjectStyleKey(StyleEngine::ElementType element_type, const ObjectVisuals &object_visuals)
        : m_elementType(element_type),
          m_objectVisuals(object_visuals)
    {
    }

    StyleEngine::ElementType m_elementType;
    ObjectVisuals m_objectVisuals;
};

uint qHash(const ObjectStyleKey &style_key)
{
    return ::qHash(style_key.m_elementType) ^ qHash(style_key.m_objectVisuals);
}

bool operator==(const ObjectStyleKey &lhs, const ObjectStyleKey &rhs)
{
    return lhs.m_elementType == rhs.m_elementType && lhs.m_objectVisuals == rhs.m_objectVisuals;
}


struct RelationStyleKey {
    RelationStyleKey(StyleEngine::ElementType element_type = StyleEngine::TYPE_OTHER,
                     DObject::VisualPrimaryRole visual_primary_role = DObject::PRIMARY_ROLE_NORMAL)
        : m_elementType(element_type),
          m_visualPrimaryRole(visual_primary_role)
    {
    }

    StyleEngine::ElementType m_elementType;
    DObject::VisualPrimaryRole m_visualPrimaryRole;
};

uint qHash(const RelationStyleKey &style_key) {
    return ::qHash(style_key.m_elementType) ^ ::qHash(style_key.m_visualPrimaryRole);
}

bool operator==(const RelationStyleKey &lhs, const RelationStyleKey &rhs)
{
    return lhs.m_elementType == rhs.m_elementType && lhs.m_visualPrimaryRole == rhs.m_visualPrimaryRole;
}


struct AnnotationStyleKey {
    AnnotationStyleKey(DAnnotation::VisualRole visual_role = DAnnotation::ROLE_NORMAL)
        : m_visualRole(visual_role)
    {
    }

    DAnnotation::VisualRole m_visualRole;
};

uint qHash(const AnnotationStyleKey &style_key) {
    return ::qHash(style_key.m_visualRole);
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

uint qHash(const BoundaryStyleKey &style_key) {
    Q_UNUSED(style_key);

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

const Style *DefaultStyleEngine::applyStyle(const Style *base_style, StyleEngine::ElementType element_type, const StyleEngine::Parameters *parameters)
{
    switch (element_type) {
    case TYPE_ANNOTATION:
        return applyAnnotationStyle(base_style, DAnnotation::ROLE_NORMAL, parameters);
    case TYPE_BOUNDARY:
        return applyBoundaryStyle(base_style, parameters);
    case TYPE_RELATION:
        break;
    case TYPE_CLASS:
    case TYPE_COMPONENT:
    case TYPE_ITEM:
    case TYPE_PACKAGE:
        return applyObjectStyle(base_style, element_type, ObjectVisuals(DObject::PRIMARY_ROLE_NORMAL, DObject::SECONDARY_ROLE_NONE, false, QColor(), 0), parameters);
    case TYPE_OTHER:
        break;
    }
    return base_style;
}

const Style *DefaultStyleEngine::applyObjectStyle(const Style *base_style, StyleEngine::ElementType element_type, const ObjectVisuals &object_visuals, const StyleEngine::Parameters *parameters)
{
    ObjectStyleKey key(element_type, object_visuals);
    const Style *derived_style = m_objectStyleMap.value(key);
    if (!derived_style) {
        int line_width = 1;

        QColor fill_color = getFillColor(element_type, object_visuals);
        QColor line_color = getLineColor(element_type, object_visuals);
        QColor text_color = getTextColor(element_type, object_visuals);

        QFont normal_font = base_style->getNormalFont();
        QFont header_font = base_style->getNormalFont();
        if (object_visuals.isEmphasized()) {
            line_width = 2;
            header_font.setBold(true);
        }

        Style *style = new Style(base_style->getType());
        QPen line_pen = base_style->getLinePen();
        line_pen.setColor(line_color);
        line_pen.setWidth(line_width);
        style->setLinePen(line_pen);
        style->setInnerLinePen(line_pen);
        style->setOuterLinePen(line_pen);
        style->setExtraLinePen(line_pen);
        style->setTextBrush(QBrush(text_color));
        if (object_visuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
            style->setFillBrush(QBrush(Qt::white));
        } else {
            if (!parameters->suppressGradients()) {
                QLinearGradient fill_gradient(0.0, 0.0, 0.0, 1.0);
                fill_gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
                fill_gradient.setColorAt(0.0, fill_color.lighter(110));
                fill_gradient.setColorAt(1.0, fill_color.darker(110));
                style->setFillBrush(QBrush(fill_gradient));
            } else {
                style->setFillBrush(QBrush(fill_color));
            }
        }
        if (object_visuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
            style->setExtraFillBrush(QBrush(Qt::white));
        } else {
            style->setExtraFillBrush(QBrush(fill_color.darker(120)));
        }
        style->setNormalFont(normal_font);
        style->setSmallFont(base_style->getSmallFont());
        style->setHeaderFont(header_font);
        m_objectStyleMap.insert(key, style);
        derived_style = style;
    }

    return derived_style;
}

const Style *DefaultStyleEngine::applyObjectStyle(const Style *base_style, const StyledObject &styled_object, const Parameters *parameters)
{
    ElementType element_type = getObjectType(styled_object.getObject());

    struct DepthProperties {
        DepthProperties()
            : m_elementType(TYPE_OTHER),
              m_visualPrimaryRole(DObject::PRIMARY_ROLE_NORMAL),
              m_visualSecondaryRole(DObject::SECONDARY_ROLE_NONE)
        {
        }

        DepthProperties(ElementType element_type, DObject::VisualPrimaryRole visual_primary_role, DObject::VisualSecondaryRole visual_secondary_role)
            : m_elementType(element_type),
              m_visualPrimaryRole(visual_primary_role),
              m_visualSecondaryRole(visual_secondary_role)
        {
        }

        ElementType m_elementType;
        DObject::VisualPrimaryRole m_visualPrimaryRole;
        DObject::VisualSecondaryRole m_visualSecondaryRole;
    };

    // find colliding elements which best match visual appearance of styled object
    DObject::VisualPrimaryRole styled_visual_primary_role = styled_object.getObjectVisuals().getVisualPrimaryRole();
    DObject::VisualSecondaryRole styled_visual_secondary_role = styled_object.getObjectVisuals().getVisualSecondaryRole();
    QHash<int, DepthProperties> depths;
    foreach (const DObject *colliding_object, styled_object.getCollidingObjects()) {
        int colliding_depth = colliding_object->getDepth();
        if (colliding_depth < styled_object.getObject()->getDepth()) {
            ElementType colliding_element_type = getObjectType(colliding_object);
            DObject::VisualPrimaryRole colliding_visual_primary_role = colliding_object->getVisualPrimaryRole();
            DObject::VisualSecondaryRole colliding_visual_secondary_role = colliding_object->getVisualSecondaryRole();
            if (!depths.contains(colliding_depth)) {
                depths.insert(colliding_depth, DepthProperties(colliding_element_type, colliding_visual_primary_role, colliding_visual_secondary_role));
            } else {
                bool update_properties = false;
                DepthProperties properties = depths.value(colliding_depth);
                if (properties.m_elementType != element_type && colliding_element_type == element_type) {
                    properties.m_elementType = colliding_element_type;
                    properties.m_visualPrimaryRole = colliding_visual_primary_role;
                    properties.m_visualSecondaryRole = colliding_visual_secondary_role;
                    update_properties = true;
                } else if (properties.m_elementType == element_type && colliding_element_type == element_type) {
                    if ((properties.m_visualPrimaryRole != styled_visual_primary_role || properties.m_visualSecondaryRole != styled_visual_secondary_role)
                            && colliding_visual_primary_role == styled_visual_primary_role && colliding_visual_secondary_role == styled_visual_secondary_role) {
                        properties.m_visualPrimaryRole = colliding_visual_primary_role;
                        properties.m_visualSecondaryRole = colliding_visual_secondary_role;
                        update_properties = true;
                    }
                }
                if (update_properties) {
                    depths.insert(colliding_depth, properties);
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
            if (properties.m_elementType == element_type && areStackingRoles(properties.m_visualPrimaryRole, properties.m_visualSecondaryRole, styled_visual_primary_role, styled_visual_secondary_role)) {
                ++depth;
            } else {
                depth = 0;
            }
        }
    }

    return applyObjectStyle(base_style, element_type,
                            ObjectVisuals(styled_visual_primary_role,
                                          styled_visual_secondary_role,
                                          styled_object.getObjectVisuals().isEmphasized(),
                                          styled_object.getObjectVisuals().getBaseColor(),
                                          depth),
                            parameters);
}

const Style *DefaultStyleEngine::applyRelationStyle(const Style *base_style, const StyledRelation &styled_relation, const Parameters *parameters)
{
    Q_UNUSED(parameters);

    ElementType element_type = getObjectType(styled_relation.getEndA());
    RelationStyleKey key(element_type, styled_relation.getEndA() ? styled_relation.getEndA()->getVisualPrimaryRole() : DObject::PRIMARY_ROLE_NORMAL);
    const Style *derived_style = m_relationStyleMap.value(key);
    if (!derived_style) {
        Style *style = new Style(base_style->getType());

        const DObject *object = styled_relation.getEndA();
        ObjectVisuals object_visuals(object ? object->getVisualPrimaryRole() : DObject::PRIMARY_ROLE_NORMAL,
                                     object ? object->getVisualSecondaryRole() : DObject::SECONDARY_ROLE_NONE,
                                     object ? object->isVisualEmphasized() : false,
                                     Qt::black, // TODO STyledRelation should get an EndAObjectVisuals
                                     object ? object->getDepth() : 0);
        QColor line_color = getLineColor(getObjectType(object), object_visuals);
        QColor fill_color = line_color;

        QPen line_pen = base_style->getLinePen();
        line_pen.setWidth(1);
        line_pen.setColor(line_color);
        style->setLinePen(line_pen);
        QBrush text_brush = base_style->getTextBrush();
        text_brush.setColor(QColor("black"));
        style->setTextBrush(text_brush);
        QBrush brush = base_style->getFillBrush();
        brush.setColor(fill_color);
        brush.setStyle(Qt::SolidPattern);
        style->setFillBrush(brush);
        style->setNormalFont(base_style->getNormalFont());
        style->setSmallFont(base_style->getSmallFont());
        style->setHeaderFont(base_style->getHeaderFont());
        m_relationStyleMap.insert(key, style);
        derived_style = style;
    }
    return derived_style;
}

const Style *DefaultStyleEngine::applyAnnotationStyle(const Style *base_style, const DAnnotation *annotation, const Parameters *parameters)
{
    DAnnotation::VisualRole visual_role = annotation ? annotation->getVisualRole() : DAnnotation::ROLE_NORMAL;
    return applyAnnotationStyle(base_style, visual_role, parameters);
}

const Style *DefaultStyleEngine::applyBoundaryStyle(const Style *base_style, const DBoundary *boundary, const Parameters *parameters)
{
    Q_UNUSED(boundary);

    return applyBoundaryStyle(base_style, parameters);
}

const Style *DefaultStyleEngine::applyAnnotationStyle(const Style *base_style, DAnnotation::VisualRole visual_role, const StyleEngine::Parameters *parameters)
{
    Q_UNUSED(parameters);

    AnnotationStyleKey key(visual_role);
    const Style *derived_style = m_annotationStyleMap.value(key);
    if (!derived_style) {
        Style *style = new Style(base_style->getType());
        QFont normal_font;
        QBrush text_brush = base_style->getTextBrush();
        switch (visual_role) {
        case DAnnotation::ROLE_NORMAL:
            normal_font = base_style->getNormalFont();
            break;
        case DAnnotation::ROLE_TITLE:
            normal_font = base_style->getHeaderFont();
            break;
        case DAnnotation::ROLE_SUBTITLE:
            normal_font = base_style->getNormalFont();
            normal_font.setItalic(true);
            break;
        case DAnnotation::ROLE_EMPHASIZED:
            normal_font = base_style->getNormalFont();
            normal_font.setBold(true);
            break;
        case DAnnotation::ROLE_SOFTEN:
            normal_font = base_style->getNormalFont();
            text_brush.setColor(Qt::gray);
            break;
        case DAnnotation::ROLE_FOOTNOTE:
            normal_font = base_style->getSmallFont();
            break;
        }
        style->setNormalFont(normal_font);
        style->setTextBrush(text_brush);
        m_annotationStyleMap.insert(key, style);
        derived_style = style;
    }
    return derived_style;
}

const Style *DefaultStyleEngine::applyBoundaryStyle(const Style *base_style, const StyleEngine::Parameters *parameters)
{
    Q_UNUSED(parameters);

    BoundaryStyleKey key;
    const Style *derived_style = m_boundaryStyleMap.value(key);
    if (!derived_style) {
        Style *style = new Style(base_style->getType());
        style->setNormalFont(base_style->getNormalFont());
        style->setTextBrush(base_style->getTextBrush());
        m_boundaryStyleMap.insert(key, style);
        derived_style = style;
    }
    return derived_style;
}

DefaultStyleEngine::ElementType DefaultStyleEngine::getObjectType(const DObject *object)
{
    ElementType element_type;
    if (dynamic_cast<const DPackage *>(object)) {
        element_type = TYPE_PACKAGE;
    } else if (dynamic_cast<const DComponent *>(object)) {
        element_type = TYPE_COMPONENT;
    } else if (dynamic_cast<const DClass *>(object)) {
        element_type = TYPE_CLASS;
    } else if (dynamic_cast<const DItem *>(object)) {
        element_type = TYPE_ITEM;
    } else {
        element_type = TYPE_OTHER;
    }
    return element_type;
}

bool DefaultStyleEngine::areStackingRoles(DObject::VisualPrimaryRole rhs_primary_role, DObject::VisualSecondaryRole rhs_secondary_role,
                                          DObject::VisualPrimaryRole lhs_primary_role, DObject::VisualSecondaryRole lhs_secondary_rols)
{
    switch (rhs_secondary_role) {
    case DObject::SECONDARY_ROLE_NONE:
    case DObject::SECONDARY_ROLE_LIGHTER:
    case DObject::SECONDARY_ROLE_DARKER:
        switch (lhs_secondary_rols) {
        case DObject::SECONDARY_ROLE_NONE:
        case DObject::SECONDARY_ROLE_LIGHTER:
        case DObject::SECONDARY_ROLE_DARKER:
            return lhs_primary_role == rhs_primary_role;
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

QColor DefaultStyleEngine::getBaseColor(ElementType element_type, ObjectVisuals object_visuals)
{
    if (object_visuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
        return QColor("#FFFFFF");
    }

    QColor base_color;

    if (object_visuals.getVisualPrimaryRole() == DObject::PRIMARY_ROLE_NORMAL) {
        if (object_visuals.getBaseColor().isValid()) {
            base_color = object_visuals.getBaseColor();
        } else {
            switch (element_type) {
            case TYPE_PACKAGE:
                base_color = QColor("#7C98AD");
                break;
            case TYPE_COMPONENT:
                base_color = QColor("#A0A891");
                break;
            case TYPE_CLASS:
                base_color = QColor("#E5A858");
                break;
            case TYPE_ITEM:
                base_color = QColor("#B995C6");
                break;
            default:
                base_color = QColor("#BF7D65");
                break;
            }
        }
    } else {
        static QColor custom_colors[] = {
            QColor("#EE8E99").darker(110),  // ROLE_CUSTOM1,
            QColor("#80AF47").lighter(130), // ROLE_CUSTOM2,
            QColor("#FFA15B").lighter(100), // ROLE_CUSTOM3,
            QColor("#55C4CF").lighter(120), // ROLE_CUSTOM4,
            QColor("#FFE14B")               // ROLE_CUSTOM5,
        };

        int index = (int) object_visuals.getVisualPrimaryRole() - (int) DObject::PRIMARY_ROLE_CUSTOM1;
        QMT_CHECK(index >= 0 && index <= 4);
        base_color = custom_colors[index];
    }

    switch (object_visuals.getVisualSecondaryRole()) {
    case DObject::SECONDARY_ROLE_NONE:
        break;
    case DObject::SECONDARY_ROLE_LIGHTER:
        base_color = base_color.lighter(110);
        break;
    case DObject::SECONDARY_ROLE_DARKER:
        base_color = base_color.darker(120);
        break;
    case DObject::SECONDARY_ROLE_SOFTEN:
        base_color = base_color.lighter(300);
        break;
    case DObject::SECONDARY_ROLE_OUTLINE:
        QMT_CHECK(false);
        break;
    }

    return base_color;
}

QColor DefaultStyleEngine::getLineColor(ElementType element_type, const ObjectVisuals &object_visuals)
{
    QColor line_color;
    if (object_visuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
        line_color = Qt::black;
    } else if (object_visuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_SOFTEN) {
        line_color = Qt::gray;
    } else {
        line_color = getBaseColor(element_type, object_visuals).darker(200).lighter(150).darker(100 + object_visuals.getDepth() * 10);
    }
    return line_color;
}

QColor DefaultStyleEngine::getFillColor(ElementType element_type, const ObjectVisuals &object_visuals)
{
    QColor fill_color;
    if (object_visuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_OUTLINE) {
        fill_color = Qt::white;
    } else {
        fill_color = getBaseColor(element_type, object_visuals).lighter(150).darker(100 + object_visuals.getDepth() * 10);
    }
    return fill_color;
}

QColor DefaultStyleEngine::getTextColor(const DObject *object, int depth)
{
    Q_UNUSED(depth);

    QColor text_color;
    DObject::VisualPrimaryRole visual_role = object ? object->getVisualPrimaryRole() : DObject::PRIMARY_ROLE_NORMAL;
    if (visual_role == DObject::DEPRECATED_PRIMARY_ROLE_SOFTEN) {
        text_color = Qt::gray;
    } else {
        text_color = Qt::black;
    }
    return text_color;
}

QColor DefaultStyleEngine::getTextColor(ElementType element_type, const ObjectVisuals &object_visuals)
{
    Q_UNUSED(element_type);

    QColor text_color;
    if (object_visuals.getVisualSecondaryRole() == DObject::SECONDARY_ROLE_SOFTEN) {
        text_color = Qt::gray;
    } else {
        text_color = Qt::black;
    }
    return text_color;
}

}
