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

#include "nimcodestylesettingspage.h"
#include "nimcodestylepreferencesfactory.h"
#include "nimsettings.h"

#include "../nimconstants.h"

#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>

#include <QVBoxLayout>

using namespace TextEditor;

namespace Nim {

class NimCodeStyleSettingsWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Nim::CodeStyleSettings)

public:
    NimCodeStyleSettingsWidget()
    {
        auto originalTabPreferences = NimSettings::globalCodeStyle();
        m_nimCodeStylePreferences = new SimpleCodeStylePreferences(this);
        m_nimCodeStylePreferences->setDelegatingPool(originalTabPreferences->delegatingPool());
        m_nimCodeStylePreferences->setTabSettings(originalTabPreferences->tabSettings());
        m_nimCodeStylePreferences->setCurrentDelegate(originalTabPreferences->currentDelegate());
        m_nimCodeStylePreferences->setId(originalTabPreferences->id());

        auto factory = TextEditorSettings::codeStyleFactory(Nim::Constants::C_NIMLANGUAGE_ID);

        auto editor = new CodeStyleEditor(factory, m_nimCodeStylePreferences);

        auto layout = new QVBoxLayout(this);
        layout->addWidget(editor);
    }

private:
    void apply() final {}
    void finish() final {}

    TextEditor::SimpleCodeStylePreferences *m_nimCodeStylePreferences;
};

NimCodeStyleSettingsPage::NimCodeStyleSettingsPage()
{
    setId(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_ID);
    setDisplayName(tr(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_DISPLAY));
    setCategory(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_CATEGORY);
    setDisplayCategory(NimCodeStyleSettingsWidget::tr("Nim"));
    setCategoryIconPath(":/nim/images/settingscategory_nim.png");
    setWidgetCreator([] { return new NimCodeStyleSettingsWidget; });
}

} // Nim
