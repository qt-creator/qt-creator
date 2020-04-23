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

#include "nimtoolssettingspage.h"
#include "nimconstants.h"
#include "nimsettings.h"
#include "ui_nimtoolssettingswidget.h"

#include <coreplugin/icore.h>

namespace Nim {

NimToolsSettingsWidget::NimToolsSettingsWidget() : ui(new Ui::NimToolsSettingsWidget)
{
    ui->setupUi(this);
    ui->pathWidget->setExpectedKind(Utils::PathChooser::ExistingCommand);
}

NimToolsSettingsWidget::~NimToolsSettingsWidget()
{
    delete ui;
}

QString NimToolsSettingsWidget::command() const
{
    return ui->pathWidget->filePath().toString();
}

void NimToolsSettingsWidget::setCommand(const QString &filename)
{
    ui->pathWidget->setPath(filename);
}

NimToolsSettingsPage::NimToolsSettingsPage(NimSettings *settings)
    : m_settings(settings)
{
    setId(Nim::Constants::C_NIMTOOLSSETTINGSPAGE_ID);
    setDisplayName(NimToolsSettingsWidget::tr(Nim::Constants::C_NIMTOOLSSETTINGSPAGE_DISPLAY));
    setCategory(Nim::Constants::C_NIMTOOLSSETTINGSPAGE_CATEGORY);
    setDisplayCategory(NimToolsSettingsWidget::tr("Nim"));
    setCategoryIconPath(":/nim/images/settingscategory_nim.png");
}

NimToolsSettingsPage::~NimToolsSettingsPage() = default;

QWidget *NimToolsSettingsPage::widget()
{
    if (!m_widget)
        m_widget.reset(new NimToolsSettingsWidget);
    m_widget->setCommand(m_settings->nimSuggestPath());
    return m_widget.get();
}

void NimToolsSettingsPage::apply()
{
    m_settings->setNimSuggestPath(m_widget->command());
    m_settings->save();
}

void NimToolsSettingsPage::finish()
{
    m_widget.reset();
}

}
