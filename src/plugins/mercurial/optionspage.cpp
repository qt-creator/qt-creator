/**************************************************************************
**
** Copyright (c) 2014 Brian McGillion
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
#include "mercurialsettings.h"
#include "mercurialplugin.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>
#include <vcsbase/vcsbaseconstants.h>

#include <QTextStream>

using namespace Mercurial::Internal;
using namespace Mercurial;

OptionsPageWidget::OptionsPageWidget(QWidget *parent) :
        QWidget(parent)
{
    m_ui.setupUi(this);
    m_ui.commandChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui.commandChooser->setHistoryCompleter(QLatin1String("Mercurial.Command.History"));
    m_ui.commandChooser->setPromptDialogTitle(tr("Mercurial Command"));
}

MercurialSettings OptionsPageWidget::settings() const
{
    MercurialSettings s = MercurialPlugin::settings();
    s.setValue(MercurialSettings::binaryPathKey, m_ui.commandChooser->rawPath());
    s.setValue(MercurialSettings::userNameKey, m_ui.defaultUsernameLineEdit->text().trimmed());
    s.setValue(MercurialSettings::userEmailKey, m_ui.defaultEmailLineEdit->text().trimmed());
    s.setValue(MercurialSettings::logCountKey, m_ui.logEntriesCount->value());
    s.setValue(MercurialSettings::timeoutKey, m_ui.timeout->value());
    return s;
}

void OptionsPageWidget::setSettings(const MercurialSettings &s)
{
    m_ui.commandChooser->setPath(s.stringValue(MercurialSettings::binaryPathKey));
    m_ui.defaultUsernameLineEdit->setText(s.stringValue(MercurialSettings::userNameKey));
    m_ui.defaultEmailLineEdit->setText(s.stringValue(MercurialSettings::userEmailKey));
    m_ui.logEntriesCount->setValue(s.intValue(MercurialSettings::logCountKey));
    m_ui.timeout->setValue(s.intValue(MercurialSettings::timeoutKey));
}

OptionsPage::OptionsPage()
{
    setId(VcsBase::Constants::VCS_ID_MERCURIAL);
    setDisplayName(tr("Mercurial"));
}

QWidget *OptionsPage::widget()
{
    if (!optionsPageWidget)
        optionsPageWidget = new OptionsPageWidget;
    optionsPageWidget->setSettings(MercurialPlugin::settings());
    return optionsPageWidget;
}

void OptionsPage::apply()
{
    if (!optionsPageWidget)
        return;
    const MercurialSettings newSettings = optionsPageWidget->settings();
    if (newSettings != MercurialPlugin::settings()) {
        //assume success and emit signal that settings are changed;
        MercurialPlugin::setSettings(newSettings);
        newSettings.writeSettings(Core::ICore::settings());
        emit settingsChanged();
    }
}

void OptionsPage::finish()
{
    delete optionsPageWidget;
}
