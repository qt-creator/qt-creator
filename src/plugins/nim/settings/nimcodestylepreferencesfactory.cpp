// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nimcodestylepreferencesfactory.h"
#include "nimcodestylepreferenceswidget.h"

#include "../nimconstants.h"
#include "../editor/nimindenter.h"

#include <utils/id.h>

#include <texteditor/simplecodestylepreferences.h>

using namespace TextEditor;

namespace Nim {

NimCodeStylePreferencesFactory::NimCodeStylePreferencesFactory()
{
}

Utils::Id NimCodeStylePreferencesFactory::languageId()
{
    return Constants::C_NIMLANGUAGE_ID;
}

QString NimCodeStylePreferencesFactory::displayName()
{
    return tr(Constants::C_NIMLANGUAGE_NAME);
}

TextEditor::ICodeStylePreferences *NimCodeStylePreferencesFactory::createCodeStyle() const
{
    return new TextEditor::SimpleCodeStylePreferences();
}

TextEditor::CodeStyleEditorWidget *NimCodeStylePreferencesFactory::createEditor(
    TextEditor::ICodeStylePreferences *preferences,
    ProjectExplorer::Project *project,
    QWidget *parent) const
{
    Q_UNUSED(project)
    auto result = new NimCodeStylePreferencesWidget(preferences, parent);
    return result;
}

TextEditor::Indenter *NimCodeStylePreferencesFactory::createIndenter(QTextDocument *doc) const
{
    return new NimIndenter(doc);
}

QString NimCodeStylePreferencesFactory::snippetProviderGroupId() const
{
    return Nim::Constants::C_NIMSNIPPETSGROUP_ID;
}

QString NimCodeStylePreferencesFactory::previewText() const
{
    return QLatin1String(Nim::Constants::C_NIMCODESTYLEPREVIEWSNIPPET);
}

}
