/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "optionspage.h"
#include "mercurialsettings.h"
#include "mercurialplugin.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>
#include <vcsbase/vcsbaseconstants.h>

using namespace Mercurial::Internal;
using namespace Mercurial;

OptionsPageWidget::OptionsPageWidget(QWidget *parent) :
        QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::Command);
    m_ui.commandChooser->setPromptDialogTitle(tr("Mercurial Command"));
}

MercurialSettings OptionsPageWidget::settings() const
{
    MercurialSettings rc;
    rc.setBinary(m_ui.commandChooser->path());
    rc.setUserName(m_ui.defaultUsernameLineEdit->text().trimmed());
    rc.setEmail(m_ui.defaultEmailLineEdit->text().trimmed());
    rc.setLogCount(m_ui.logEntriesCount->value());
    rc.setTimeoutSeconds(m_ui.timeout->value());
    rc.setPrompt(m_ui.promptOnSubmitCheckBox->isChecked());
    return rc;
}

void OptionsPageWidget::setSettings(const MercurialSettings &s)
{
    m_ui.commandChooser->setPath(s.binary());
    m_ui.defaultUsernameLineEdit->setText(s.userName());
    m_ui.defaultEmailLineEdit->setText(s.email());
    m_ui.logEntriesCount->setValue(s.logCount());
    m_ui.timeout->setValue(s.timeoutSeconds());
    m_ui.promptOnSubmitCheckBox->setChecked(s.prompt());
}

OptionsPage::OptionsPage()
{
}

QString OptionsPage::id() const
{
    return QLatin1String("Mercurial");
}

QString OptionsPage::trName() const
{
    return tr("Mercurial");
}

QString OptionsPage::category() const
{
    return QLatin1String(VCSBase::Constants::VCS_SETTINGS_CATEGORY);
}

QString OptionsPage::trCategory() const
{
    return QCoreApplication::translate("VCSBase", VCSBase::Constants::VCS_SETTINGS_CATEGORY);
}

QWidget *OptionsPage::createPage(QWidget *parent)
{
    if (!optionsPageWidget)
        optionsPageWidget = new OptionsPageWidget(parent);
    optionsPageWidget->setSettings(MercurialPlugin::instance()->settings());
    return optionsPageWidget;
}

void OptionsPage::apply()
{
    if (!optionsPageWidget)
        return;
    MercurialPlugin *plugin = MercurialPlugin::instance();
    const MercurialSettings newSettings = optionsPageWidget->settings();
    if (newSettings != plugin->settings()) {
        //assume success and emit signal that settings are changed;
        plugin->setSettings(newSettings);
        newSettings.writeSettings(Core::ICore::instance()->settings());
        emit settingsChanged();
    }
}

