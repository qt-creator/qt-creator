/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimcodestylepreferencesfactory.h"
#include "nimcodestylepreferenceswidget.h"

#include "../nimconstants.h"
#include "../editor/nimindenter.h"

#include <coreplugin/id.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/snippets/snippetprovider.h>
#include <extensionsystem/pluginmanager.h>

#include <QWidget>
#include <QLayout>

namespace Nim {

NimCodeStylePreferencesFactory::NimCodeStylePreferencesFactory()
{
}

Core::Id NimCodeStylePreferencesFactory::languageId()
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

QWidget *NimCodeStylePreferencesFactory::createEditor(TextEditor::ICodeStylePreferences *preferences,
                                                      QWidget *parent) const
{
    auto result = new NimCodeStylePreferencesWidget(preferences, parent);
    result->layout()->setMargin(0);
    return result;
}

TextEditor::Indenter *NimCodeStylePreferencesFactory::createIndenter() const
{
    return new NimIndenter();
}

TextEditor::SnippetProvider *NimCodeStylePreferencesFactory::snippetProvider() const
{
    return ExtensionSystem::PluginManager::getObject<TextEditor::SnippetProvider>(
        [](TextEditor::SnippetProvider *provider) {
            return provider->groupId() == Nim::Constants::C_NIMSNIPPETSGROUP_ID;
        });
}

QString NimCodeStylePreferencesFactory::previewText() const
{
    return QLatin1String(Nim::Constants::C_NIMCODESTYLEPREVIEWSNIPPET);
}

}
