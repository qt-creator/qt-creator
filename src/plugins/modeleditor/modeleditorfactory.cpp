// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modeleditorfactory.h"

#include "modeleditor_constants.h"
#include "actionhandler.h"
#include "modeleditor.h"

#include <coreplugin/coreplugintr.h>

#include <QCoreApplication>

namespace ModelEditor {
namespace Internal {

ModelEditorFactory::ModelEditorFactory(UiController *uiController, ActionHandler *actionHandler)
{
    setId(Constants::MODEL_EDITOR_ID);
    setDisplayName(::Core::Tr::tr("Model Editor"));
    addMimeType(Constants::MIME_TYPE_MODEL);
    setEditorCreator([uiController, actionHandler] { return new ModelEditor(uiController, actionHandler); });
}

} // namespace Internal
} // namespace ModelEditor
