// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "styleengine.h"
#include "qmt/diagram/dobject.h"

#include <QObject>
#include <QScopedPointer>

namespace qmt {

class Style;
class ObjectVisuals;
class StyledObject;
class StyledRelation;
class DAnnotation;
class DBoundary;

class QMT_EXPORT StyleController : public QObject
{
    class Parameters;

public:
    explicit StyleController(QObject *parent = nullptr);
    ~StyleController() override;

    bool suppressGradients() const { return m_suppressGradients; }
    void setSuppressGradients(bool suppressGradients);

    const Style *adaptStyle(StyleEngine::ElementType elementType);
    const Style *adaptObjectStyle(StyleEngine::ElementType elementType,
                                  const ObjectVisuals &objectVisuals);
    const Style *adaptObjectStyle(const StyledObject &object);
    const Style *adaptRelationStyle(const StyledRelation &relation);
    const Style *adaptAnnotationStyle(const DAnnotation *annotation);
    const Style *adaptBoundaryStyle(const DBoundary *boundary);
    const Style *adaptSwimlaneStyle(const DSwimlane *swimlane);
    const Style *relationStarterStyle();

private:
    QScopedPointer<Style> m_defaultStyle;
    QScopedPointer<Style> m_relationStarterStyle;
    QScopedPointer<StyleEngine> m_defaultStyleEngine;
    bool m_suppressGradients = false;
};

} // namespace qmt
