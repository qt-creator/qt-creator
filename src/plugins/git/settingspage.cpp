/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "settingspage.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QSettings>
#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>
#include <QtCore/QDebug>

using namespace Git::Internal;

static const char *groupC = "Git";
static const char *sysEnvKeyC = "SysEnv";
static const char *pathKeyC = "Path";
static const char *logCountKeyC = "LogCount";

SettingsPage::SettingsPage()
{
    Core::ICore *coreIFace = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    if (coreIFace)
        m_settings = coreIFace->settings();

    if (m_settings) {
        m_settings->beginGroup(QLatin1String(groupC));
        m_adopt = m_settings->value(QLatin1String(sysEnvKeyC), true).toBool();
        m_path = m_settings->value(QLatin1String(pathKeyC), QString()).toString();
        m_logCount = m_settings->value(QLatin1String(logCountKeyC), 10).toInt();
        m_settings->endGroup();
    }
}

QString SettingsPage::name() const
{
    return tr("General");
}

QString SettingsPage::category() const
{
    return QLatin1String("Git");
}

QString SettingsPage::trCategory() const
{
    return tr("Git");
}

QWidget *SettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_ui.adoptCheckBox->setChecked(m_adopt);
    m_ui.pathLineEdit->setText(m_path);
    m_ui.logLineEdit->setText(QString::number(m_logCount));

    connect(m_ui.adoptButton, SIGNAL(clicked()), this, SLOT(setSystemPath()));
    return w;
}

void SettingsPage::finished(bool accepted)
{
    if (!accepted)
        return;

    m_adopt = m_ui.adoptCheckBox->isChecked();
    m_path = m_ui.pathLineEdit->text();
    m_logCount = m_ui.logLineEdit->text().toInt();

    if (!m_settings)
        return;

    m_settings->beginGroup(QLatin1String(groupC));
    m_settings->setValue(QLatin1String(sysEnvKeyC), m_adopt);
    m_settings->setValue(QLatin1String(pathKeyC), m_path);
    m_settings->setValue(QLatin1String(logCountKeyC), m_logCount);
    m_settings->endGroup();
}

void SettingsPage::setSystemPath()
{
    m_path = qgetenv("PATH");
    m_ui.pathLineEdit->setText(m_path);
}
