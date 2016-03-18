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

#include "qmt/tasks/ielementtasks.h"

namespace qmt { class DocumentController; }

namespace ModelEditor {
namespace Internal {

class ElementTasks :
        public qmt::IElementTasks
{
    class ElementTasksPrivate;

public:
    ElementTasks();
    ~ElementTasks();

    void setDocumentController(qmt::DocumentController *documentController);

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

private:
    ElementTasksPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
