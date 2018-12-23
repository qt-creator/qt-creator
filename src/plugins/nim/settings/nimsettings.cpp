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

#include "nimsettings.h"
#include "nimcodestylepreferencesfactory.h"

#include "../nimconstants.h"

#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/codestylepool.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/tabsettings.h>
#include <coreplugin/icore.h>

using namespace TextEditor;

namespace Nim {

static SimpleCodeStylePreferences *m_globalCodeStyle = nullptr;

NimSettings::NimSettings(QObject *parent)
    : QObject(parent)
{
    InitializeCodeStyleSettings();
    InitializeNimSuggestSettings();
}

NimSettings::~NimSettings()
{
    TerminateCodeStyleSettings();
}

QString NimSettings::nimSuggestPath() const
{
    return m_nimSuggestPath;
}

void NimSettings::setNimSuggestPath(const QString &path)
{
    if (m_nimSuggestPath == path)
        return;
    m_nimSuggestPath = path;
    emit nimSuggestPathChanged(path);
}

void NimSettings::save()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::C_NIM_SETTINGS_GROUP);
    s->beginGroup(Constants::C_NIM_SETTINGS_NIMSUGGEST_GROUP);
    s->setValue(QString::fromStdString(Constants::C_NIM_SETTINGS_COMMAND), nimSuggestPath());
    s->endGroup();
    s->endGroup();
    s->sync();
}

SimpleCodeStylePreferences *NimSettings::globalCodeStyle()
{
    return m_globalCodeStyle;
}

void NimSettings::InitializeCodeStyleSettings()
{
    // code style factory
    auto factory = new NimCodeStylePreferencesFactory();
    TextEditorSettings::registerCodeStyleFactory(factory);

    // code style pool
    auto pool = new CodeStylePool(factory, this);
    TextEditorSettings::registerCodeStylePool(Nim::Constants::C_NIMLANGUAGE_ID, pool);

    m_globalCodeStyle = new SimpleCodeStylePreferences();
    m_globalCodeStyle->setDelegatingPool(pool);
    m_globalCodeStyle->setDisplayName(tr("Global", "Settings"));
    m_globalCodeStyle->setId(Nim::Constants::C_NIMGLOBALCODESTYLE_ID);
    pool->addCodeStyle(m_globalCodeStyle);
    TextEditorSettings::registerCodeStyle(Nim::Constants::C_NIMLANGUAGE_ID, m_globalCodeStyle);

    auto nimCodeStyle = new SimpleCodeStylePreferences();
    nimCodeStyle->setId("nim");
    nimCodeStyle->setDisplayName(tr("Nim"));
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
}

void NimSettings::InitializeNimSuggestSettings()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::C_NIM_SETTINGS_GROUP);
    s->beginGroup(Constants::C_NIM_SETTINGS_NIMSUGGEST_GROUP);
    setNimSuggestPath(s->value(QString::fromStdString(Constants::C_NIM_SETTINGS_COMMAND),
                               QString()).toString());
    s->endGroup();
    s->endGroup();
}

void NimSettings::TerminateCodeStyleSettings()
{
    TextEditorSettings::unregisterCodeStyle(Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::unregisterCodeStylePool(Nim::Constants::C_NIMLANGUAGE_ID);
    TextEditorSettings::unregisterCodeStyleFactory(Nim::Constants::C_NIMLANGUAGE_ID);

    delete m_globalCodeStyle;
    m_globalCodeStyle = nullptr;
}

} // namespace Nim
