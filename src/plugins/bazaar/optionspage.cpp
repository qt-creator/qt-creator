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
    OptionsPageWidget(Core::IVersionControl *control);

    void apply() final;

private:
    Ui::OptionsPage m_ui;
    Core::IVersionControl *m_control;
    BazaarClient *m_client;
};

void OptionsPageWidget::apply()
{
    VcsBaseClientSettings s = BazaarPlugin::instance()->client()->settings();
    s.setValue(BazaarSettings::binaryPathKey, m_ui.commandChooser->rawPath());
    s.setValue(BazaarSettings::userNameKey, m_ui.defaultUsernameLineEdit->text().trimmed());
    s.setValue(BazaarSettings::userEmailKey, m_ui.defaultEmailLineEdit->text().trimmed());
    s.setValue(BazaarSettings::logCountKey, m_ui.logEntriesCount->value());
    s.setValue(BazaarSettings::timeoutKey, m_ui.timeout->value());

    const bool notify = m_client->settings() != s;
    m_client->settings() = s;
    if (notify)
        emit m_control->configurationChanged();
}

OptionsPageWidget::OptionsPageWidget(Core::IVersionControl *control)
    : m_control(control), m_client(BazaarPlugin::instance()->client())
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.commandChooser->setPromptDialogTitle(tr("Bazaar Command"));
    m_ui.commandChooser->setHistoryCompleter(QLatin1String("Bazaar.Command.History"));

    VcsBaseClientSettings s = m_client->settings();
    m_ui.commandChooser->setPath(s.stringValue(BazaarSettings::binaryPathKey));
    m_ui.defaultUsernameLineEdit->setText(s.stringValue(BazaarSettings::userNameKey));
    m_ui.defaultEmailLineEdit->setText(s.stringValue(BazaarSettings::userEmailKey));
    m_ui.logEntriesCount->setValue(s.intValue(BazaarSettings::logCountKey));
    m_ui.timeout->setValue(s.intValue(BazaarSettings::timeoutKey));
}

OptionsPage::OptionsPage(Core::IVersionControl *control, QObject *parent) :
    Core::IOptionsPage(parent)
{
    setId(VcsBase::Constants::VCS_ID_BAZAAR);
    setDisplayName(OptionsPageWidget::tr("Bazaar"));
    setWidgetCreator([control] { return new OptionsPageWidget(control); });
    setCategory(Constants::VCS_SETTINGS_CATEGORY);
}

} // Internal
} // Bazaar
