// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimcodestylesettingspage.h"

#include "nimcodestylepreferencesfactory.h"
#include "nimsettings.h"
#include "../nimconstants.h"
#include "../nimtr.h"

#include <coreplugin/icore.h>

#include <texteditor/codestyleeditor.h>
#include <texteditor/codestylepool.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QVBoxLayout>

using namespace TextEditor;

namespace Nim {

static SimpleCodeStylePreferences *m_globalCodeStyle = nullptr;
static CodeStylePool *pool = nullptr;

SimpleCodeStylePreferences *globalCodeStyle()
{
    QTC_CHECK(m_globalCodeStyle);
    return m_globalCodeStyle;
}

static void createGlobalCodeStyle()
{
    auto factory = new NimCodeStylePreferencesFactory();
    TextEditorSettings::registerCodeStyleFactory(factory);

    // code style pool
    pool = new CodeStylePool(factory);
    TextEditorSettings::registerCodeStylePool(Nim::Constants::C_NIMLANGUAGE_ID, pool);

    m_globalCodeStyle = new SimpleCodeStylePreferences();
    m_globalCodeStyle->setDelegatingPool(pool);
    m_globalCodeStyle->setDisplayName(Tr::tr("Global", "Settings"));
    m_globalCodeStyle->setId(Nim::Constants::C_NIMGLOBALCODESTYLE_ID);
    pool->addCodeStyle(m_globalCodeStyle);
    TextEditorSettings::registerCodeStyle(Nim::Constants::C_NIMLANGUAGE_ID, m_globalCodeStyle);

    auto nimCodeStyle = new SimpleCodeStylePreferences();
    nimCodeStyle->setId("nim");
    nimCodeStyle->setDisplayName(Tr::tr("Nim"));
    nimCodeStyle->setReadOnly(true);

    TabSettings nimTabSettings;
    nimTabSettings.m_tabPolicy = TabSettings::SpacesOnlyTabPolicy;
    nimTabSettings.m_tabSize = 2;
    nimTabSettings.m_indentSize = 2;
    nimTabSettings.m_continuationAlignBehavior = TabSettings::ContinuationAlignWithIndent;
    nimCodeStyle->setTabSettings(nimTabSettings);

    pool->addCodeStyle(nimCodeStyle);

    m_globalCodeStyle->setCurrentDelegate(nimCodeStyle);

    pool->loadCustomCodeStyles();

    // load global settings (after built-in settings are added to the pool)
    m_globalCodeStyle->fromSettings(Nim::Constants::C_NIMLANGUAGE_ID);

    TextEditorSettings::registerMimeTypeForLanguageId(Nim::Constants::C_NIM_MIMETYPE,
                                                      Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Nim::Constants::C_NIM_SCRIPT_MIMETYPE,
                                                      Nim::Constants::C_NIMLANGUAGE_ID);
}

static void destroyGlobalCodeStyle()
{
    TextEditorSettings::unregisterCodeStyle(Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::unregisterCodeStylePool(Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::unregisterCodeStyleFactory(Nim::Constants::C_NIMLANGUAGE_ID);

    delete m_globalCodeStyle;
    m_globalCodeStyle = nullptr;

    delete pool;
    pool = nullptr;
}

class NimCodeStyleSettingsWidget : public Core::IOptionsPageWidget
{
public:
    NimCodeStyleSettingsWidget()
    {
        auto originalTabPreferences = globalCodeStyle();
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

    void apply() final
    {
        QTC_ASSERT(m_globalCodeStyle, return);
        m_globalCodeStyle->toSettings(Nim::Constants::C_NIMLANGUAGE_ID);
    }

private:
    TextEditor::SimpleCodeStylePreferences *m_nimCodeStylePreferences;
};

// NimCodeStyleSettingsPage

NimCodeStyleSettingsPage::NimCodeStyleSettingsPage()
{
    setId(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_ID);
    setDisplayName(Tr::tr("Code Style"));
    setCategory(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_CATEGORY);
    setDisplayCategory(Tr::tr("Nim"));
    setCategoryIconPath(":/nim/images/settingscategory_nim.png");
    setWidgetCreator([] { return new NimCodeStyleSettingsWidget; });

    createGlobalCodeStyle();
}

NimCodeStyleSettingsPage::~NimCodeStyleSettingsPage()
{
    destroyGlobalCodeStyle();
}

} // Nim
