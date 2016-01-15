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

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <vcsbase/vcsbaseconstants.h>
#include <utils/pathchooser.h>

#include <QCoreApplication>
#include <QTextStream>
#include <QFileDialog>

using namespace Subversion::Internal;
using namespace Utils;
using namespace VcsBase;

SettingsPageWidget::SettingsPageWidget(QWidget *parent) : VcsClientOptionsPageWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.pathChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_ui.pathChooser->setHistoryCompleter(QLatin1String("Subversion.Command.History"));
    m_ui.pathChooser->setPromptDialogTitle(tr("Subversion Command"));
}

VcsBase::VcsBaseClientSettings SettingsPageWidget::settings() const
{
    SubversionSettings rc;
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
    return rc;
}

void SettingsPageWidget::setSettings(const VcsBaseClientSettings &s)
{
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

SettingsPage::SettingsPage(Core::IVersionControl *control) :
    VcsClientOptionsPage(control, SubversionPlugin::instance()->client())
{
    setId(VcsBase::Constants::VCS_ID_SUBVERSION);
    setDisplayName(tr("Subversion"));
    setWidgetFactory([]() { return new SettingsPageWidget; });
}
