/****************************************************************************
**
** Copyright (c) 2018 Artur Shepilko
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
#include "constants.h"
#include "fossilclient.h"
#include "fossilsettings.h"
#include "fossilplugin.h"
#include "ui_optionspage.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>
#include <vcsbase/vcsbaseconstants.h>

namespace Fossil {
namespace Internal {

class OptionsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Fossil::Internal::OptionsPageWidget)

public:
    OptionsPageWidget(Core::IVersionControl *control, FossilSettings *settings);
    void apply() final;

private:
    Ui::OptionsPage m_ui;
    Core::IVersionControl *m_control;
    FossilSettings *m_settings;
};

void OptionsPageWidget::apply()
{
    FossilSettings s = *m_settings;
    s.setValue(FossilSettings::binaryPathKey, m_ui.commandChooser->rawPath());
    s.setValue(FossilSettings::defaultRepoPathKey, m_ui.defaultRepoPathChooser->path());
    s.setValue(FossilSettings::userNameKey, m_ui.defaultUsernameLineEdit->text().trimmed());
    s.setValue(FossilSettings::sslIdentityFileKey, m_ui.sslIdentityFilePathChooser->path());
    s.setValue(FossilSettings::logCountKey, m_ui.logEntriesCount->value());
    s.setValue(FossilSettings::timelineWidthKey, m_ui.logEntriesWidth->value());
    s.setValue(FossilSettings::timeoutKey, m_ui.timeout->value());
    s.setValue(FossilSettings::disableAutosyncKey, m_ui.disableAutosyncCheckBox->isChecked());
    if (*m_settings == s)
        return;

    *m_settings = s;
    emit m_control->configurationChanged();
}

OptionsPageWidget::OptionsPageWidget(Core::IVersionControl *control, FossilSettings *settings) :
    m_control(control),
    m_settings(settings)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.commandChooser->setPromptDialogTitle(tr("Fossil Command"));
    m_ui.commandChooser->setHistoryCompleter("Fossil.Command.History");
    m_ui.commandChooser->setPath(m_settings->stringValue(FossilSettings::binaryPathKey));
    m_ui.defaultRepoPathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_ui.defaultRepoPathChooser->setPromptDialogTitle(tr("Fossil Repositories"));
    m_ui.defaultRepoPathChooser->setPath(m_settings->stringValue(FossilSettings::defaultRepoPathKey));
    m_ui.sslIdentityFilePathChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui.sslIdentityFilePathChooser->setPromptDialogTitle(tr("SSL/TLS Identity Key"));
    m_ui.sslIdentityFilePathChooser->setPath(m_settings->stringValue(FossilSettings::sslIdentityFileKey));
    m_ui.defaultUsernameLineEdit->setText(m_settings->stringValue(FossilSettings::userNameKey));
    m_ui.logEntriesCount->setValue(m_settings->intValue(FossilSettings::logCountKey));
    m_ui.logEntriesWidth->setValue(m_settings->intValue(FossilSettings::timelineWidthKey));
    m_ui.timeout->setValue(m_settings->intValue(FossilSettings::timeoutKey));
    m_ui.disableAutosyncCheckBox->setChecked(m_settings->boolValue(FossilSettings::disableAutosyncKey));
}

OptionsPage::OptionsPage(Core::IVersionControl *control, FossilSettings *settings, QObject *parent) :
    Core::IOptionsPage(parent)
{
    setId(Constants::VCS_ID_FOSSIL);
    setDisplayName(OptionsPageWidget::tr("Fossil"));
    setWidgetCreator([control, settings]() { return new OptionsPageWidget(control, settings); });
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
}

} // Internal
} // Fossil
