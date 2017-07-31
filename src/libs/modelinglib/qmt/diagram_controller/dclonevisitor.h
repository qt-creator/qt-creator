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

#include "qmt/diagram/dconstvisitor.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class QMT_EXPORT DCloneVisitor : public DConstVisitor
{
public:
    DCloneVisitor();

    DElement *cloned() const { return m_cloned; }

    void visitDElement(const DElement *element) override;
    void visitDObject(const DObject *object) override;
    void visitDPackage(const DPackage *package) override;
    void visitDClass(const DClass *klass) override;
    void visitDComponent(const DComponent *component) override;
    void visitDDiagram(const DDiagram *diagram) override;
    void visitDItem(const DItem *item) override;
    void visitDRelation(const DRelation *relation) override;
    void visitDInheritance(const DInheritance *inheritance) override;
    void visitDDependency(const DDependency *dependency) override;
    void visitDAssociation(const DAssociation *association) override;
    void visitDConnection(const DConnection *connection) override;
    void visitDAnnotation(const DAnnotation *annotation) override;
    void visitDBoundary(const DBoundary *boundary) override;
    void visitDSwimlane(const DSwimlane *swimlane) override;

private:
    DElement *m_cloned = nullptr;
};

class QMT_EXPORT DCloneDeepVisitor : public DConstVisitor
{
public:
    DCloneDeepVisitor();

    DElement *cloned() const { return m_cloned; }

    void visitDElement(const DElement *element) override;
    void visitDObject(const DObject *object) override;
    void visitDPackage(const DPackage *package) override;
    void visitDClass(const DClass *klass) override;
    void visitDComponent(const DComponent *component) override;
    void visitDDiagram(const DDiagram *diagram) override;
    void visitDItem(const DItem *item) override;
    void visitDRelation(const DRelation *relation) override;
    void visitDInheritance(const DInheritance *inheritance) override;
    void visitDDependency(const DDependency *dependency) override;
    void visitDAssociation(const DAssociation *association) override;
    void visitDConnection(const DConnection *connection) override;
    void visitDAnnotation(const DAnnotation *annotation) override;
    void visitDBoundary(const DBoundary *boundary) override;
    void visitDSwimlane(const DSwimlane *swimlane) override;

private:
    DElement *m_cloned = nullptr;
};

} // namespace qmt
