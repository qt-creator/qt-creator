/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
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

#include "mercurialclient.h"
#include "mercurialsettings.h"
#include "mercurialplugin.h"
#include "ui_optionspage.h"
#include "ui_optionspage.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>
#include <vcsbase/vcsbaseconstants.h>

using namespace VcsBase;

namespace Mercurial {
namespace Internal  {

class OptionsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Mercurial::Internal::OptionsPageWidget)

public:
    OptionsPageWidget(const std::function<void()> &onApply, MercurialSettings *settings);
    void apply() final;

private:
    Ui::OptionsPage m_ui;
    std::function<void()> m_onApply;
    MercurialSettings *m_settings;
};

OptionsPageWidget::OptionsPageWidget(const std::function<void()> &onApply, MercurialSettings *settings)
    : m_onApply(onApply), m_settings(settings)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.commandChooser->setHistoryCompleter(QLatin1String("Mercurial.Command.History"));
    m_ui.commandChooser->setPromptDialogTitle(tr("Mercurial Command"));

    const VcsBaseClientSettings &s = *settings;

    m_ui.commandChooser->setPath(s.stringValue(MercurialSettings::binaryPathKey));
    m_ui.defaultUsernameLineEdit->setText(s.stringValue(MercurialSettings::userNameKey));
    m_ui.defaultEmailLineEdit->setText(s.stringValue(MercurialSettings::userEmailKey));
    m_ui.logEntriesCount->setValue(s.intValue(MercurialSettings::logCountKey));
    m_ui.timeout->setValue(s.intValue(MercurialSettings::timeoutKey));
}

void OptionsPageWidget::apply()
{
    MercurialSettings ms;
    ms.setValue(MercurialSettings::binaryPathKey, m_ui.commandChooser->rawPath());
    ms.setValue(MercurialSettings::userNameKey, m_ui.defaultUsernameLineEdit->text().trimmed());
    ms.setValue(MercurialSettings::userEmailKey, m_ui.defaultEmailLineEdit->text().trimmed());
    ms.setValue(MercurialSettings::logCountKey, m_ui.logEntriesCount->value());
    ms.setValue(MercurialSettings::timeoutKey, m_ui.timeout->value());

    if (*m_settings != ms) {
        *m_settings = ms;
        m_onApply();
    }
}

OptionsPage::OptionsPage(const std::function<void()> &onApply, MercurialSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_MERCURIAL);
    setDisplayName(OptionsPageWidget::tr("Mercurial"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([onApply, settings] { return new OptionsPageWidget(onApply, settings); });
}

} // namespace Internal
} // namespace Mercurial
