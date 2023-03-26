// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimcodestylepreferencesfactory.h"
#include "nimcodestylepreferenceswidget.h"

#include "../editor/nimindenter.h"
#include "../nimconstants.h"
#include "../nimtr.h"

#include <utils/id.h>

#include <texteditor/simplecodestylepreferences.h>

#include <QWidget>

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
    return Tr::tr(Constants::C_NIMLANGUAGE_NAME);
}

ICodeStylePreferences *NimCodeStylePreferencesFactory::createCodeStyle() const
{
    return new SimpleCodeStylePreferences();
}

CodeStyleEditorWidget *NimCodeStylePreferencesFactory::createEditor(
    ICodeStylePreferences *preferences,
    ProjectExplorer::Project *project,
    QWidget *parent) const
{
    Q_UNUSED(project)
    auto result = new NimCodeStylePreferencesWidget(preferences, parent);
    return result;
}

Indenter *NimCodeStylePreferencesFactory::createIndenter(QTextDocument *doc) const
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

} // Nim
