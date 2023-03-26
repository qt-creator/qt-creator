// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
