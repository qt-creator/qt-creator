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

#include <QObject>

#include "styleengine.h"
#include "qmt/diagram/dobject.h"

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
    Q_OBJECT
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
