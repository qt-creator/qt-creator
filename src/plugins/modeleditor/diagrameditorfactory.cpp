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

#include "diagrameditorfactory.h"

#include "modeleditor_constants.h"
#include "actionhandler.h"
#include "diagrameditor.h"

#include <QApplication>

namespace ModelEditor {
namespace Internal {

class DiagramEditorFactory::DiagramEditorFactoryPrivate
{
public:
    UiController *uiController = 0;
    ActionHandler *actionHandler = 0;
};

DiagramEditorFactory::DiagramEditorFactory(UiController *uiController, QObject *parent)
    : Core::IEditorFactory(parent),
      d(new DiagramEditorFactoryPrivate)
{
    setId(Constants::DIAGRAM_EDITOR_ID);
    setDisplayName(qApp->translate("OpenWith::Editors", Constants::DIAGRAM_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::MIME_TYPE_DIAGRAM_REFERENCE);
    d->uiController = uiController;
    d->actionHandler = new ActionHandler(Core::Context(Constants::DIAGRAM_EDITOR_ID), this);
}

DiagramEditorFactory::~DiagramEditorFactory()
{
    delete d;
}

Core::IEditor *DiagramEditorFactory::createEditor()
{
    return new DiagramEditor(d->uiController, d->actionHandler);
}

void DiagramEditorFactory::extensionsInitialized()
{
    d->actionHandler->createActions();
    d->actionHandler->createEditPropertiesShortcut(
                Constants::SHORTCUT_DIAGRAM_EDITOR_EDIT_PROPERTIES);
}

} // namespace Internal
} // namespace ModelEditor
