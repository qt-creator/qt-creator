/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "settingspage.h"

#include "subversionclient.h"
#include "subversionplugin.h"
#include "subversionsettings.h"

#include "ui_settingspage.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <vcsbase/vcsbaseconstants.h>
#include <utils/pathchooser.h>

#include <QCoreApplication>

using namespace Utils;
using namespace VcsBase;

namespace Subversion {
namespace Internal {

class SubversionSettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Subversion::Internal::SettingsPageWidget)

public:
    SubversionSettingsPageWidget(const std::function<void()> &onApply, SubversionSettings *settings);

    void apply() final;

private:
    Ui::SettingsPage m_ui;
    std::function<void()> m_onApply;
    SubversionSettings *m_settings;
};

SubversionSettingsPageWidget::SubversionSettingsPageWidget(const std::function<void()> &onApply,
                                                           SubversionSettings *settings)
    : m_onApply(onApply), m_settings(settings)
{
    m_ui.setupUi(this);
    m_ui.pathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_ui.pathChooser->setHistoryCompleter(QLatin1String("Subversion.Command.History"));
    m_ui.pathChooser->setPromptDialogTitle(tr("Subversion Command"));

    SubversionSettings &s = *m_settings;
    m_ui.pathChooser->setFileName(s.binaryPath());
    m_ui.usernameLineEdit->setText(s.stringValue(SubversionSettings::userKey));
    m_ui.passwordLineEdit->setText(s.stringValue(SubversionSettings::passwordKey));
    m_ui.userGroupBox->setChecked(s.boolValue(SubversionSettings::useAuthenticationKey));
    m_ui.timeOutSpinBox->setValue(s.intValue(SubversionSettings::timeoutKey));
    m_ui.promptToSubmitCheckBox->setChecked(s.boolValue(SubversionSettings::promptOnSubmitKey));
    m_ui.spaceIgnorantAnnotationCheckBox->setChecked(
                s.boolValue(SubversionSettings::spaceIgnorantAnnotationKey));
    m_ui.logCountSpinBox->setValue(s.intValue(SubversionSettings::logCountKey));
}

void SubversionSettingsPageWidget::apply()
{
    SubversionSettings rc = *m_settings;
    rc.setValue(SubversionSettings::binaryPathKey, m_ui.pathChooser->rawPath());
    rc.setValue(SubversionSettings::useAuthenticationKey, m_ui.userGroupBox->isChecked());
    rc.setValue(SubversionSettings::userKey, m_ui.usernameLineEdit->text());
    rc.setValue(SubversionSettings::passwordKey, m_ui.passwordLineEdit->text());
    rc.setValue(SubversionSettings::timeoutKey, m_ui.timeOutSpinBox->value());
    if (rc.stringValue(SubversionSettings::userKey).isEmpty())
        rc.setValue(SubversionSettings::useAuthenticationKey, false);
    rc.setValue(SubversionSettings::promptOnSubmitKey, m_ui.promptToSubmitCheckBox->isChecked());
    rc.setValue(SubversionSettings::spaceIgnorantAnnotationKey,
                m_ui.spaceIgnorantAnnotationCheckBox->isChecked());
    rc.setValue(SubversionSettings::logCountKey, m_ui.logCountSpinBox->value());

    if (rc == *m_settings)
        return;

    *m_settings = rc;
    m_onApply();
}

SubversionSettingsPage::SubversionSettingsPage(const std::function<void()> &onApply, SubversionSettings *settings)
{
    setId(VcsBase::Constants::VCS_ID_SUBVERSION);
    setDisplayName(SubversionSettingsPageWidget::tr("Subversion"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([onApply, settings] { return new SubversionSettingsPageWidget(onApply, settings); });
}

} // Internal
} // Subversion
