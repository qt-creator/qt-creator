// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ielementtasks.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class QMT_EXPORT VoidElementTasks : public IElementTasks
{
public:
    VoidElementTasks();
    ~VoidElementTasks() override;

    void openElement(const MElement *) override;
    void openElement(const DElement *, const MDiagram *) override;

    bool hasClassDefinition(const MElement *) const override;
    bool hasClassDefinition(const DElement *, const MDiagram *) const override;
    void openClassDefinition(const MElement *) override;
    void openClassDefinition(const DElement *, const MDiagram *) override;

    bool hasHeaderFile(const MElement *) const override;
    bool hasHeaderFile(const DElement *, const MDiagram *) const override;
    bool hasSourceFile(const MElement *) const override;
    bool hasSourceFile(const DElement *, const MDiagram *) const override;
    void openHeaderFile(const MElement *) override;
    void openHeaderFile(const DElement *, const MDiagram *) override;
    void openSourceFile(const MElement *) override;
    void openSourceFile(const DElement *, const MDiagram *) override;

    bool hasFolder(const MElement *) const override;
    bool hasFolder(const DElement *, const MDiagram *) const override;
    void showFolder(const MElement *) override;
    void showFolder(const DElement *, const MDiagram *) override;

    bool hasDiagram(const MElement *) const override;
    bool hasDiagram(const DElement *, const MDiagram *) const override;
    void openDiagram(const MElement *) override;
    void openDiagram(const DElement *, const MDiagram *) override;

    bool hasParentDiagram(const MElement *) const override;
    bool hasParentDiagram(const DElement *, const MDiagram *) const override;
    void openParentDiagram(const MElement *) override;
    void openParentDiagram(const DElement *, const MElement *) override;

    bool mayCreateDiagram(const MElement *) const override;
    bool mayCreateDiagram(const DElement *, const MDiagram *) const override;
    void createAndOpenDiagram(const MElement *) override;
    void createAndOpenDiagram(const DElement *, const MDiagram *) override;

    bool extendContextMenu(const DElement *, const MDiagram *, QMenu *) override;
    bool handleContextMenuAction(const DElement *, const MDiagram *, const QString &) override;
};

} // namespace qmt
