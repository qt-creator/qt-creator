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

#include <extensionsystem/pluginmanager.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>
#include <utils/qtcassert.h>

#include <QWidget>

using namespace TextEditor;

namespace Nim {

NimCodeStyleSettingsPage::NimCodeStyleSettingsPage(QWidget *parent)
    : Core::IOptionsPage(parent)
    , m_nimCodeStylePreferences(nullptr)
    , m_widget(nullptr)
{
    setId(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_ID);
    setDisplayName(tr(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_DISPLAY));
    setCategory(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_CATEGORY);
    setDisplayCategory(tr(Nim::Constants::C_NIMCODESTYLESETTINGSPAGE_CATEGORY_DISPLAY));
    setCategoryIcon(Utils::Icon(Nim::Constants::C_NIM_ICON_PATH));
}

NimCodeStyleSettingsPage::~NimCodeStyleSettingsPage()
{
    deleteWidget();
}

QWidget *NimCodeStyleSettingsPage::widget()
{
    if (!m_widget) {
        auto originalTabPreferences = qobject_cast<SimpleCodeStylePreferences *>(NimSettings::globalCodeStyle());
        m_nimCodeStylePreferences = new SimpleCodeStylePreferences(m_widget);
        m_nimCodeStylePreferences->setDelegatingPool(originalTabPreferences->delegatingPool());
        m_nimCodeStylePreferences->setTabSettings(originalTabPreferences->tabSettings());
        m_nimCodeStylePreferences->setCurrentDelegate(originalTabPreferences->currentDelegate());
        m_nimCodeStylePreferences->setId(originalTabPreferences->id());
        auto factory = TextEditorSettings::codeStyleFactory(Nim::Constants::C_NIMLANGUAGE_ID);
        m_widget = new CodeStyleEditor(factory, m_nimCodeStylePreferences);
    }
    return m_widget;
}

void NimCodeStyleSettingsPage::apply()
{

}

void NimCodeStyleSettingsPage::finish()
{
    deleteWidget();
}

void NimCodeStyleSettingsPage::deleteWidget()
{
    if (m_widget) {
        delete m_widget;
        m_widget = nullptr;
    }
}

}
