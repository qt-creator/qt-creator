/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "optionspage.h"
#include "mercurialsettings.h"
#include "mercurialplugin.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QtCore/QTextStream>

using namespace Mercurial::Internal;
using namespace Mercurial;

OptionsPageWidget::OptionsPageWidget(QWidget *parent) :
        QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
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

QString OptionsPageWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
            << sep << m_ui.configGroupBox->title()
            << sep << m_ui.mercurialCommandLabel->text()
            << sep << m_ui.userGroupBox->title()
            << sep << m_ui.defaultUsernameLabel->text()
            << sep << m_ui.defaultEmailLabel->text()
            << sep << m_ui.miscGroupBox->title()
            << sep << m_ui.showLogEntriesLabel->text()
            << sep << m_ui.timeoutSecondsLabel->text()
            << sep << m_ui.promptOnSubmitCheckBox->text()
               ;
    rc.remove(QLatin1Char('&'));
    return rc;
}

OptionsPage::OptionsPage()
{
}

QString OptionsPage::id() const
{
    return QLatin1String(VCSBase::Constants::VCS_ID_MERCURIAL);
}

QString OptionsPage::displayName() const
{
    return tr("Mercurial");
}

QWidget *OptionsPage::createPage(QWidget *parent)
{
    if (!optionsPageWidget)
        optionsPageWidget = new OptionsPageWidget(parent);
    optionsPageWidget->setSettings(MercurialPlugin::instance()->settings());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = optionsPageWidget->searchKeywords();
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

bool OptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
