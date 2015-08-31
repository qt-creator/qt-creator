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

#include "diagrameditor.h"

#include "modeleditor_constants.h"
#include "diagramdocument.h"
#include "extdocumentcontroller.h"

#include "qmt/model/mdiagram.h"
#include "qmt/model_controller/modelcontroller.h"
#include "qmt/project/project.h"
#include "qmt/project_controller/projectcontroller.h"

#include <utils/qtcassert.h>

#include <QFileInfo>

namespace ModelEditor {
namespace Internal {

class DiagramEditor::DiagramEditorPrivate
{
public:
    DiagramDocument *document = 0;
};

DiagramEditor::DiagramEditor(UiController *uiController, ActionHandler *actionHandler,
                             QWidget *parent)
    : AbstractEditor(uiController, actionHandler, parent),
      d(new DiagramEditorPrivate)
{
    setContext(Core::Context(Constants::DIAGRAM_EDITOR_ID));
    d->document = new DiagramDocument(this);
    connect(d->document, &DiagramDocument::contentSet, this, &DiagramEditor::onContentSet);
    init(parent);
}

DiagramEditor::~DiagramEditor()
{
    delete d;
}

Core::IDocument *DiagramEditor::document()
{
    return d->document;
}

void DiagramEditor::onContentSet()
{
    setDocument(d->document);

    // open diagram
    qmt::MDiagram *diagram = d->document->documentController()->getModelController()->findObject<qmt::MDiagram>(d->document->diagramUid());
    QTC_ASSERT(diagram, return);
    showDiagram(diagram);

    expandModelTreeToDepth(0);
}

} // namespace Internal
} // namespace ModelEditor
