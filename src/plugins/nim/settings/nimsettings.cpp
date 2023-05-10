// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimsettings.h"

#include "../nimconstants.h"
#include "../nimtr.h"
#include "nimcodestylepreferencesfactory.h"

#include <coreplugin/icore.h>

#include <texteditor/codestylepool.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/layoutbuilder.h>

using namespace TextEditor;
using namespace Utils;

namespace Nim {

static SimpleCodeStylePreferences *m_globalCodeStyle = nullptr;

NimSettings::NimSettings()
{
    setSettingsGroups("Nim", "NimSuggest");
    setId(Nim::Constants::C_NIMTOOLSSETTINGSPAGE_ID);
    setDisplayName(Tr::tr("Tools"));
    setCategory(Nim::Constants::C_NIMTOOLSSETTINGSPAGE_CATEGORY);
    setDisplayCategory(Tr::tr("Nim"));
    setCategoryIconPath(":/nim/images/settingscategory_nim.png");

    setLayouter([this](QWidget *widget) {
        using namespace Layouting;
        Column {
            Group {
                title("Nimsuggest"),
                Column { nimSuggestPath }
            },
            st
        }.attachTo(widget);
    });

    // code style factory
    auto factory = new NimCodeStylePreferencesFactory();
    TextEditorSettings::registerCodeStyleFactory(factory);

    // code style pool
    auto pool = new CodeStylePool(factory, this);
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
    QSettings *s = Core::ICore::settings();
    m_globalCodeStyle->fromSettings(QLatin1String(Nim::Constants::C_NIMLANGUAGE_ID), s);

    TextEditorSettings::registerMimeTypeForLanguageId(Nim::Constants::C_NIM_MIMETYPE,
                                                      Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Nim::Constants::C_NIM_SCRIPT_MIMETYPE,
                                                      Nim::Constants::C_NIMLANGUAGE_ID);

    registerAspect(&nimSuggestPath);
    nimSuggestPath.setSettingsKey("Command");
    nimSuggestPath.setDisplayStyle(StringAspect::PathChooserDisplay);
    nimSuggestPath.setExpectedKind(PathChooser::ExistingCommand);
    nimSuggestPath.setLabelText(Tr::tr("Path:"));

    readSettings(Core::ICore::settings());
}

NimSettings::~NimSettings()
{
    TextEditorSettings::unregisterCodeStyle(Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::unregisterCodeStylePool(Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::unregisterCodeStyleFactory(Nim::Constants::C_NIMLANGUAGE_ID);

    delete m_globalCodeStyle;
    m_globalCodeStyle = nullptr;
}

SimpleCodeStylePreferences *NimSettings::globalCodeStyle()
{
    return m_globalCodeStyle;
}

} // namespace Nim
