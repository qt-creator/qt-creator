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

#include "modeleditorfactory.h"

#include "modeleditor_constants.h"
#include "actionhandler.h"
#include "modeleditor.h"

#include <QCoreApplication>

namespace ModelEditor {
namespace Internal {

class ModelEditorFactory::ModelEditorFactoryPrivate
{
public:
    UiController *uiController = nullptr;
    ActionHandler *actionHandler = nullptr;
};

ModelEditorFactory::ModelEditorFactory(UiController *uiController, QObject *parent)
    : Core::IEditorFactory(parent),
      d(new ModelEditorFactoryPrivate())
{
    setId(Constants::MODEL_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", Constants::MODEL_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::MIME_TYPE_MODEL);
    d->uiController = uiController;
    d->actionHandler = new ActionHandler(Core::Context(Constants::MODEL_EDITOR_ID), this);
}

ModelEditorFactory::~ModelEditorFactory()
{
    delete d->actionHandler;
    delete d;
}

Core::IEditor *ModelEditorFactory::createEditor()
{
    return new ModelEditor(d->uiController, d->actionHandler);
}

void ModelEditorFactory::extensionsInitialized()
{
    d->actionHandler->createActions();
}

} // namespace Internal
} // namespace ModelEditor
