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

#pragma once

#include "styleengine.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dannotation.h"

#include <QHash>

QT_BEGIN_NAMESPACE
class QColor;
QT_END_NAMESPACE

namespace qmt {

class ObjectStyleKey;
class RelationStyleKey;
class AnnotationStyleKey;
class BoundaryStyleKey;
class SwimlaneStyleKey;

class QMT_EXPORT DefaultStyleEngine : public StyleEngine
{
    Q_DISABLE_COPY(DefaultStyleEngine)

public:
    DefaultStyleEngine();
    ~DefaultStyleEngine() override;

    const Style *applyStyle(const Style *baseStyle, ElementType elementType,
                            const Parameters *parameters) override;
    const Style *applyObjectStyle(const Style *baseStyle, ElementType elementType,
                                  const ObjectVisuals &objectVisuals,
                                  const Parameters *parameters) override;
    const Style *applyObjectStyle(const Style *baseStyle, const StyledObject &styledObject,
                                  const Parameters *parameters) override;
    const Style *applyRelationStyle(const Style *baseStyle, const StyledRelation &styledRelation,
                                    const Parameters *parameters) override;
    const Style *applyAnnotationStyle(const Style *baseStyle, const DAnnotation *annotation,
                                      const Parameters *parameters) override;
    const Style *applyBoundaryStyle(const Style *baseStyle, const DBoundary *boundary,
                                    const Parameters *parameters) override;
    const Style *applySwimlaneStyle(const Style *baseStyle, const DSwimlane *swimlane,
                                    const Parameters *parameters) override;

private:
    const Style *applyAnnotationStyle(const Style *baseStyle, DAnnotation::VisualRole visualRole,
                                      const Parameters *parameters);
    const Style *applyBoundaryStyle(const Style *baseStyle, const Parameters *parameters);
    const Style *applySwimlaneStyle(const Style *baseStyle, const Parameters *parameters);

    ElementType objectType(const DObject *object);

    bool areStackingRoles(DObject::VisualPrimaryRole rhsPrimaryRole,
                          DObject::VisualSecondaryRole rhsSecondaryRole,
                          DObject::VisualPrimaryRole lhsPrimaryRole,
                          DObject::VisualSecondaryRole lhsSecondaryRols);

    QColor baseColor(ElementType elementType, ObjectVisuals objectVisuals);
    QColor lineColor(ElementType elementType, const ObjectVisuals &objectVisuals);
    QColor fillColor(ElementType elementType, const ObjectVisuals &objectVisuals);
    QColor textColor(const DObject *object, int depth);
    QColor textColor(ElementType elementType, const ObjectVisuals &objectVisuals);

    QHash<ObjectStyleKey, const Style *> m_objectStyleMap;
    QHash<RelationStyleKey, const Style *> m_relationStyleMap;
    QHash<AnnotationStyleKey, const Style *> m_annotationStyleMap;
    QHash<BoundaryStyleKey, const Style *> m_boundaryStyleMap;
    QHash<SwimlaneStyleKey, const Style *> m_swimlaneStyleMap;
};

} // namespace qmt
