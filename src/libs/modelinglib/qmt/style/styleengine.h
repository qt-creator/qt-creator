// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"

#include <QString>
#include <QList>
#include <typeinfo>

namespace qmt {

class Style;
class ObjectVisuals;
class StyledObject;
class StyledRelation;

class DAnnotation;
class DBoundary;
class DSwimlane;

class QMT_EXPORT StyleEngine
{
public:
    enum ElementType {
        TypeOther,
        TypePackage,
        TypeComponent,
        TypeClass,
        TypeItem,
        TypeRelation,
        TypeAnnotation,
        TypeBoundary,
        TypeSwimlane
    };

    class Parameters
    {
    public:
        virtual ~Parameters() { }
        virtual bool suppressGradients() const = 0;
    };

    virtual ~StyleEngine() { }

    virtual const Style *applyStyle(const Style *baseStyle, ElementType elementType,
                                    const Parameters *) = 0;
    virtual const Style *applyObjectStyle(const Style *baseStyle, ElementType elementType,
                                          const ObjectVisuals &objectVisuals,
                                          const Parameters *parameters) = 0;
    virtual const Style *applyObjectStyle(const Style *baseStyle, const StyledObject &,
                                          const Parameters *) = 0;
    virtual const Style *applyRelationStyle(const Style *baseStyle, const StyledRelation &,
                                            const Parameters *) = 0;
    virtual const Style *applyAnnotationStyle(const Style *baseStyle, const DAnnotation *,
                                              const Parameters *) = 0;
    virtual const Style *applyBoundaryStyle(const Style *baseStyle, const DBoundary *,
                                            const Parameters *) = 0;
    virtual const Style *applySwimlaneStyle(const Style *baseStyle, const DSwimlane *,
                                            const Parameters *) = 0;
};

} // namespace qmt
