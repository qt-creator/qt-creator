/****************************************************************************
**
** Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
** Contact: https://www.qt.io/licensing/
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
