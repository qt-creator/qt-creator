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

#ifndef QMT_DEFAULTSTYLEENGINE_H
#define QMT_DEFAULTSTYLEENGINE_H

#include "styleengine.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dannotation.h"

#include <QHash>

QT_BEGIN_NAMESPACE
class QColor;
QT_END_NAMESPACE


namespace qmt {

struct ObjectStyleKey;
struct RelationStyleKey;
struct AnnotationStyleKey;
struct BoundaryStyleKey;


class QMT_EXPORT DefaultStyleEngine :
        public StyleEngine
{
    Q_DISABLE_COPY(DefaultStyleEngine)

public:

    DefaultStyleEngine();

    ~DefaultStyleEngine();

public:

    const Style *applyStyle(const Style *base_style, ElementType element_type, const Parameters *parameters);

    const Style *applyObjectStyle(const Style *base_style, ElementType element_type, const ObjectVisuals &object_visuals, const Parameters *parameters);

    const Style *applyObjectStyle(const Style *base_style, const StyledObject &styled_object, const Parameters *parameters);

    const Style *applyRelationStyle(const Style *base_style, const StyledRelation &styled_relation, const Parameters *parameters);

    const Style *applyAnnotationStyle(const Style *base_style, const DAnnotation *annotation, const Parameters *parameters);

    const Style *applyBoundaryStyle(const Style *base_style, const DBoundary *boundary, const Parameters *parameters);

private:

    const Style *applyAnnotationStyle(const Style *base_style, DAnnotation::VisualRole visual_role, const Parameters *parameters);

    const Style *applyBoundaryStyle(const Style *base_style, const Parameters *parameters);

    ElementType getObjectType(const DObject *object);

    bool areStackingRoles(DObject::VisualPrimaryRole rhs_primary_role, DObject::VisualSecondaryRole rhs_secondary_role, DObject::VisualPrimaryRole lhs_primary_role, DObject::VisualSecondaryRole lhs_secondary_rols);

    QColor getBaseColor(ElementType element_type, ObjectVisuals object_visuals);

    QColor getLineColor(ElementType element_type, const ObjectVisuals &object_visuals);

    QColor getFillColor(ElementType element_type, const ObjectVisuals &object_visuals);

    QColor getTextColor(const DObject *object, int depth);

    QColor getTextColor(ElementType element_type, const ObjectVisuals &object_visuals);

private:

    QHash<ObjectStyleKey, const Style *> _object_style_map;
    QHash<RelationStyleKey, const Style *> _relation_style_map;
    QHash<AnnotationStyleKey, const Style *> _annotation_style_map;
    QHash<BoundaryStyleKey, const Style *> _boundary_style_map;
};

}

#endif // QMT_DEFAULTSTYLEENGINE_H
