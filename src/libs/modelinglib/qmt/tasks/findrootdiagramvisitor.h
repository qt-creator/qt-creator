// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/model_controller/mvoidvisitor.h"

namespace qmt {

class MDiagram;

class QMT_EXPORT FindRootDiagramVisitor : public MVoidVisitor
{
public:
    FindRootDiagramVisitor();
    ~FindRootDiagramVisitor() override;

    MDiagram *diagram() const { return m_diagram; }

    void visitMObject(MObject *object) override;

private:
    MDiagram *m_diagram = nullptr;
};

} // namespace qmt
