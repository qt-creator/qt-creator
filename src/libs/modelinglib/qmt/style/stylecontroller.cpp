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

#include "stylecontroller.h"

#include "defaultstyle.h"
#include "relationstarterstyle.h"
#include "defaultstyleengine.h"

#include <QHash>

namespace qmt {

class StyleController::Parameters : public StyleEngine::Parameters
{
public:
    explicit Parameters(StyleController *styleController)
        : m_styleController(styleController)
    {
    }

    bool suppressGradients() const
    {
        return m_styleController->suppressGradients();
    }

private:
    StyleController *m_styleController = nullptr;
};

StyleController::StyleController(QObject *parent)
    : QObject(parent),
      m_defaultStyle(new DefaultStyle),
      m_relationStarterStyle(new RelationStarterStyle),
      m_defaultStyleEngine(new DefaultStyleEngine)
{
}

StyleController::~StyleController()
{
}

void StyleController::setSuppressGradients(bool suppressGradients)
{
    m_suppressGradients = suppressGradients;
}

const Style *StyleController::adaptStyle(StyleEngine::ElementType elementType)
{
    Parameters parameters(this);
    return m_defaultStyleEngine->applyStyle(m_defaultStyle.data(), elementType, &parameters);
}

const Style *StyleController::adaptObjectStyle(StyleEngine::ElementType elementType, const ObjectVisuals &objectVisuals)
{
    Parameters parameters(this);
    return m_defaultStyleEngine->applyObjectStyle(m_defaultStyle.data(), elementType, objectVisuals, &parameters);
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

const Style *StyleController::adaptSwimlaneStyle(const DSwimlane *swimlane)
{
    Parameters parameters(this);
    return m_defaultStyleEngine->applySwimlaneStyle(m_defaultStyle.data(), swimlane, &parameters);
}

const Style *StyleController::relationStarterStyle()
{
    return m_relationStarterStyle.data();
}

} // namespace qmt
