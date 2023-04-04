// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "icodestylepreferencesfactory.h"

#include "codestyleeditor.h"

using namespace TextEditor;

ICodeStylePreferencesFactory::ICodeStylePreferencesFactory()
{
}

CodeStyleEditorWidget *ICodeStylePreferencesFactory::createCodeStyleEditor(
    ICodeStylePreferences *codeStyle, ProjectExplorer::Project *project, QWidget *parent)
{
    return new CodeStyleEditor(this, codeStyle, project, parent);
}

CodeStyleEditorWidget *ICodeStylePreferencesFactory::createAdditionalGlobalSettings(
    ICodeStylePreferences *, ProjectExplorer::Project *, QWidget *)
{
    return nullptr;
}
