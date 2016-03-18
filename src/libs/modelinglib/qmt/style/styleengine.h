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
        TypeBoundary
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
};

} // namespace qmt
