/**************************************************************************
**
** Copyright (c) 2014 Hugues Delorme
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "optionspage.h"
#include "bazaarsettings.h"
#include "bazaarplugin.h"

#include <coreplugin/icore.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QTextStream>

using namespace Bazaar::Internal;
using namespace Bazaar;

OptionsPageWidget::OptionsPageWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.commandChooser->setPromptDialogTitle(tr("Bazaar Command"));
    m_ui.commandChooser->setHistoryCompleter(QLatin1String("Bazaar.Command.History"));
}

BazaarSettings OptionsPageWidget::settings() const
{
    BazaarSettings s = BazaarPlugin::instance()->settings();
    s.setValue(BazaarSettings::binaryPathKey, m_ui.commandChooser->rawPath());
    s.setValue(BazaarSettings::userNameKey, m_ui.defaultUsernameLineEdit->text().trimmed());
    s.setValue(BazaarSettings::userEmailKey, m_ui.defaultEmailLineEdit->text().trimmed());
    s.setValue(BazaarSettings::logCountKey, m_ui.logEntriesCount->value());
    s.setValue(BazaarSettings::timeoutKey, m_ui.timeout->value());
    return s;
}

void OptionsPageWidget::setSettings(const BazaarSettings &s)
{
    m_ui.commandChooser->setPath(s.stringValue(BazaarSettings::binaryPathKey));
    m_ui.defaultUsernameLineEdit->setText(s.stringValue(BazaarSettings::userNameKey));
    m_ui.defaultEmailLineEdit->setText(s.stringValue(BazaarSettings::userEmailKey));
    m_ui.logEntriesCount->setValue(s.intValue(BazaarSettings::logCountKey));
    m_ui.timeout->setValue(s.intValue(BazaarSettings::timeoutKey));
}

OptionsPage::OptionsPage()
{
    setId(VcsBase::Constants::VCS_ID_BAZAAR);
    setDisplayName(tr("Bazaar"));
}

QWidget *OptionsPage::widget()
{
    if (!m_optionsPageWidget)
        m_optionsPageWidget = new OptionsPageWidget;
    m_optionsPageWidget->setSettings(BazaarPlugin::instance()->settings());
    return m_optionsPageWidget;
}

void OptionsPage::apply()
{
    if (!m_optionsPageWidget)
        return;
    BazaarPlugin *plugin = BazaarPlugin::instance();
    const BazaarSettings newSettings = m_optionsPageWidget->settings();
    if (newSettings != plugin->settings()) {
        //assume success and emit signal that settings are changed;
        plugin->setSettings(newSettings);
        newSettings.writeSettings(Core::ICore::settings());
        emit settingsChanged();
    }
}
