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

#include "stylecontroller.h"

#include "defaultstyle.h"
#include "relationstarterstyle.h"
#include "defaultstyleengine.h"

#include <QHash>


namespace qmt {

class StyleController::Parameters :
        public StyleEngine::Parameters
{
public:
    Parameters(StyleController *style_controller)
        : m_styleController(style_controller)
    {
    }

    bool suppressGradients() const {
        return m_styleController->getSuppressGradients();
    }

private:

    StyleController *m_styleController;

};


StyleController::StyleController(QObject *parent)
    : QObject(parent),
      m_defaultStyle(new DefaultStyle),
      m_relationStarterStyle(new RelationStarterStyle),
      m_defaultStyleEngine(new DefaultStyleEngine),
      m_suppressGradients(false)
{
}

StyleController::~StyleController()
{
}

void StyleController::setSuppressGradients(bool suppress_gradients)
{
    m_suppressGradients = suppress_gradients;
}

const Style *StyleController::adaptStyle(StyleEngine::ElementType element_type)
{
    Parameters parameters(this);
    return m_defaultStyleEngine->applyStyle(m_defaultStyle.data(), element_type, &parameters);
}

const Style *StyleController::adaptObjectStyle(StyleEngine::ElementType element_type, const ObjectVisuals &object_visuals)
{
    Parameters parameters(this);
    return m_defaultStyleEngine->applyObjectStyle(m_defaultStyle.data(), element_type, object_visuals, &parameters);
}

const Style *StyleController::adaptObjectStyle(const StyledObject &object)
{
    Parameters parameters(this);
    return m_defaultStyleEngine->applyObjectStyle(m_defaultStyle.data(), object, &parameters);
}

const Style *StyleController::adaptRelationStyle(const StyledRelation &relation)
{
    Parameters parameters(this);
    return m_defaultStyleEngine->applyRelationStyle(m_defaultStyle.data(), relation, &parameters);
}

const Style *StyleController::adaptAnnotationStyle(const DAnnotation *annotation)
{
    Parameters parameters(this);
    return m_defaultStyleEngine->applyAnnotationStyle(m_defaultStyle.data(), annotation, &parameters);
}

const Style *StyleController::adaptBoundaryStyle(const DBoundary *boundary)
{
    Parameters parameters(this);
    return m_defaultStyleEngine->applyBoundaryStyle(m_defaultStyle.data(), boundary, &parameters);
}

const Style *StyleController::getRelationStarterStyle()
{
    return m_relationStarterStyle.data();
}

}
