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

#ifndef QMT_STYLEENGINE_H
#define QMT_STYLEENGINE_H

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
        TYPE_OTHER,
        TYPE_PACKAGE,
        TYPE_COMPONENT,
        TYPE_CLASS,
        TYPE_ITEM,
        TYPE_RELATION,
        TYPE_ANNOTATION,
        TYPE_BOUNDARY
    };

    class Parameters
    {
    public:
        virtual ~Parameters() { }
        virtual bool suppressGradients() const = 0;
    };

public:

    virtual ~StyleEngine() { }

public:

    virtual const Style *applyStyle(const Style *base_style, ElementType element_type, const Parameters *) = 0;

    virtual const Style *applyObjectStyle(const Style *base_style, ElementType element_type, const ObjectVisuals &object_visuals, const Parameters *parameters) = 0;

    virtual const Style *applyObjectStyle(const Style *base_style, const StyledObject &, const Parameters *) = 0;

    virtual const Style *applyRelationStyle(const Style *base_style, const StyledRelation &, const Parameters *) = 0;

    virtual const Style *applyAnnotationStyle(const Style *base_style, const DAnnotation *, const Parameters *) = 0;

    virtual const Style *applyBoundaryStyle(const Style *base_style, const DBoundary *, const Parameters *) = 0;
};

}

#endif // QMT_STYLEENGINE_H
