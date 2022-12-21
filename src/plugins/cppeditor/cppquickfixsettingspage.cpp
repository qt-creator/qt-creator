// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixsettingspage.h"

#include "cppeditorconstants.h"
#include "cppquickfixsettings.h"
#include "cppquickfixsettingswidget.h"

#include <QCoreApplication>
#include <QtDebug>

using namespace CppEditor::Internal;

CppQuickFixSettingsPage::CppQuickFixSettingsPage()
{
    setId(Constants::QUICK_FIX_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("CppEditor", Constants::QUICK_FIX_SETTINGS_DISPLAY_NAME));
    setCategory(Constants::CPP_SETTINGS_CATEGORY);
}

QWidget *CppQuickFixSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new CppQuickFixSettingsWidget;
        m_widget->loadSettings(CppQuickFixSettings::instance());
    }
    return m_widget;
}

void CppQuickFixSettingsPage::apply()
{
    const auto s = CppQuickFixSettings::instance();
    m_widget->saveSettings(s);
    s->saveAsGlobalSettings();
}

void CppEditor::Internal::CppQuickFixSettingsPage::finish()
{
    delete m_widget;
}
