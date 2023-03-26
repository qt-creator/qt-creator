// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include "qmt/tasks/ielementtasks.h"

namespace qmt { class DocumentController; }

namespace ModelEditor {
namespace Internal {

class ComponentViewController;

class ElementTasks :
        public QObject, public qmt::IElementTasks
{
    Q_OBJECT

    class ElementTasksPrivate;

public:
    ElementTasks(QObject *parent = nullptr);
    ~ElementTasks();

    void setDocumentController(qmt::DocumentController *documentController);
    void setComponentViewController(ComponentViewController *componentViewController);

    void openElement(const qmt::MElement *element) override;
    void openElement(const qmt::DElement *element, const qmt::MDiagram *diagram) override;

    bool hasClassDefinition(const qmt::MElement *element) const override;
    bool hasClassDefinition(const qmt::DElement *element,
                            const qmt::MDiagram *diagram) const override;
    void openClassDefinition(const qmt::MElement *element) override;
    void openClassDefinition(const qmt::DElement *element, const qmt::MDiagram *diagram) override;

    bool hasHeaderFile(const qmt::MElement *element) const override;
    bool hasHeaderFile(const qmt::DElement *element, const qmt::MDiagram *diagram) const override;
    bool hasSourceFile(const qmt::MElement *element) const override;
    bool hasSourceFile(const qmt::DElement *element, const qmt::MDiagram *diagram) const override;
    void openHeaderFile(const qmt::MElement *element) override;
    void openHeaderFile(const qmt::DElement *element, const qmt::MDiagram *diagram) override;
    void openSourceFile(const qmt::MElement *element) override;
    void openSourceFile(const qmt::DElement *element, const qmt::MDiagram *diagram) override;

    bool hasFolder(const qmt::MElement *element) const override;
    bool hasFolder(const qmt::DElement *element, const qmt::MDiagram *diagram) const override;
    void showFolder(const qmt::MElement *element) override;
    void showFolder(const qmt::DElement *element, const qmt::MDiagram *diagram) override;

    bool hasDiagram(const qmt::MElement *element) const override;
    bool hasDiagram(const qmt::DElement *element, const qmt::MDiagram *diagram) const override;
    void openDiagram(const qmt::MElement *element) override;
    void openDiagram(const qmt::DElement *element, const qmt::MDiagram *diagram) override;

    bool hasParentDiagram(const qmt::MElement *element) const override;
    bool hasParentDiagram(const qmt::DElement *element, const qmt::MDiagram *diagram) const override;
    void openParentDiagram(const qmt::MElement *element) override;
    void openParentDiagram(const qmt::DElement *element, const qmt::MElement *diagram) override;

    bool mayCreateDiagram(const qmt::MElement *element) const override;
    bool mayCreateDiagram(const qmt::DElement *element,
                          const qmt::MDiagram *diagram) const override;
    void createAndOpenDiagram(const qmt::MElement *element) override;
    void createAndOpenDiagram(const qmt::DElement *element, const qmt::MDiagram *diagram) override;

    bool extendContextMenu(const qmt::DElement *delement, const qmt::MDiagram *, QMenu *menu) override;
    bool handleContextMenuAction(const qmt::DElement *element, const qmt::MDiagram *, const QString &id) override;

private:
    ElementTasksPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
