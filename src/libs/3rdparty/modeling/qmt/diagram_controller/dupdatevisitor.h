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

#ifndef QMT_DUPDATEVISITOR_H
#define QMT_DUPDATEVISITOR_H

#include "qmt/model/mconstvisitor.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class DElement;

class QMT_EXPORT DUpdateVisitor : public MConstVisitor
{
public:
    DUpdateVisitor(DElement *target, const MDiagram *diagram, bool checkNeedsUpdate = false);

    bool isUpdateNeeded() const { return m_isUpdateNeeded; }
    void setCheckNeedsUpdate(bool checkNeedsUpdate);

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

private:
    bool isUpdating(bool valueChanged);

    DElement *m_target;
    const MDiagram *m_diagram;
    bool m_checkNeedsUpdate;
    bool m_isUpdateNeeded;
};

} // namespace qmt

#endif // QMT_DUPDATEVISITOR_H
