/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#include "optionspage.h"
#include "bazaarclient.h"
#include "bazaarsettings.h"
#include "bazaarplugin.h"
#include "ui_optionspage.h"

#include <coreplugin/icore.h>
#include <vcsbase/vcsbaseconstants.h>

using namespace VcsBase;

namespace Bazaar {
namespace Internal {

class OptionsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Bazaar::Internal::OptionsPageWidget)

public:
    OptionsPageWidget(Core::IVersionControl *control, BazaarSettings *settings);

    void apply() final;

private:
    Ui::OptionsPage m_ui;
    Core::IVersionControl *m_control;
    BazaarSettings *m_settings;
};

void OptionsPageWidget::apply()
{
    BazaarSettings s = *m_settings;
    s.setValue(BazaarSettings::binaryPathKey, m_ui.commandChooser->rawPath());
    s.setValue(BazaarSettings::userNameKey, m_ui.defaultUsernameLineEdit->text().trimmed());
    s.setValue(BazaarSettings::userEmailKey, m_ui.defaultEmailLineEdit->text().trimmed());
    s.setValue(BazaarSettings::logCountKey, m_ui.logEntriesCount->value());
    s.setValue(BazaarSettings::timeoutKey, m_ui.timeout->value());

    if (*m_settings == s)
        return;

    *m_settings = s;
    emit m_control->configurationChanged();
}

OptionsPageWidget::OptionsPageWidget(Core::IVersionControl *control, BazaarSettings *settings)
    : m_control(control), m_settings(settings)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.commandChooser->setPromptDialogTitle(tr("Bazaar Command"));
    m_ui.commandChooser->setHistoryCompleter(QLatin1String("Bazaar.Command.History"));

    m_ui.commandChooser->setPath(m_settings->stringValue(BazaarSettings::binaryPathKey));
    m_ui.defaultUsernameLineEdit->setText(m_settings->stringValue(BazaarSettings::userNameKey));
    m_ui.defaultEmailLineEdit->setText(m_settings->stringValue(BazaarSettings::userEmailKey));
    m_ui.logEntriesCount->setValue(m_settings->intValue(BazaarSettings::logCountKey));
    m_ui.timeout->setValue(m_settings->intValue(BazaarSettings::timeoutKey));
}

OptionsPage::OptionsPage(Core::IVersionControl *control, BazaarSettings *settings, QObject *parent) :
    Core::IOptionsPage(parent)
{
    setId(VcsBase::Constants::VCS_ID_BAZAAR);
    setDisplayName(OptionsPageWidget::tr("Bazaar"));
    setWidgetCreator([control, settings] { return new OptionsPageWidget(control, settings); });
    setCategory(Constants::VCS_SETTINGS_CATEGORY);
}

} // Internal
} // Bazaar
