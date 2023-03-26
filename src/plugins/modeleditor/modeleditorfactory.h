// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditorfactory.h>

namespace ModelEditor {
namespace Internal {

class ActionHandler;
class ModelEditor;
class UiController;

class ModelEditorFactory : public Core::IEditorFactory
{
public:
    ModelEditorFactory(UiController *uiController, ActionHandler *actionHandler);
};

} // namespace Internal
} // namespace ModelEditor
