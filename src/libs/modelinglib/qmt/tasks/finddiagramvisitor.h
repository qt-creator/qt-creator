// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/model_controller/mvoidvisitor.h"

namespace qmt {

class QMT_EXPORT FindDiagramVisitor : public MVoidConstVisitor
{
public:
    FindDiagramVisitor();
    ~FindDiagramVisitor() override;

    const MDiagram *diagram() const { return m_diagram; }

    void visitMObject(const MObject *object) override;
    void visitMDiagram(const MDiagram *diagram) override;

private:
    const MDiagram *m_diagram = nullptr;
};

} // namespace qmt
