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

#include "qmt/model/mvisitor.h"
#include "qmt/model/mconstvisitor.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class QMT_EXPORT MVoidVisitor : public MVisitor
{
public:
    void visitMElement(MElement *element) override;
    void visitMObject(MObject *object) override;
    void visitMPackage(MPackage *package) override;
    void visitMClass(MClass *klass) override;
    void visitMComponent(MComponent *component) override;
    void visitMDiagram(MDiagram *diagram) override;
    void visitMCanvasDiagram(MCanvasDiagram *diagram) override;
    void visitMItem(MItem *item) override;
    void visitMRelation(MRelation *relation) override;
    void visitMDependency(MDependency *dependency) override;
    void visitMInheritance(MInheritance *inheritance) override;
    void visitMAssociation(MAssociation *association) override;
    void visitMConnection(MConnection *connection) override;
};

class QMT_EXPORT MVoidConstVisitor : public MConstVisitor
{
public:
    void visitMElement(const MElement *element) override;
    void visitMObject(const MObject *object) override;
    void visitMPackage(const MPackage *package) override;
    void visitMClass(const MClass *klass) override;
    void visitMComponent(const MComponent *component) override;
    void visitMDiagram(const MDiagram *diagram) override;
    void visitMCanvasDiagram(const MCanvasDiagram *diagram) override;
    void visitMItem(const MItem *item) override;
    void visitMRelation(const MRelation *relation) override;
    void visitMDependency(const MDependency *dependency) override;
    void visitMInheritance(const MInheritance *inheritance) override;
    void visitMAssociation(const MAssociation *association) override;
    void visitMConnection(const MConnection *connection) override;
};

} // namespace qmt
