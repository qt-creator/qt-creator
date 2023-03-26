// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    bool suppressGradients() const final
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
