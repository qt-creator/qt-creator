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

#include "gdboptionpage.h"

#include "gdbengine.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>

using namespace Debugger::Internal;

GdbOptionPage::GdbOptionPage(GdbSettings *settings)
{
    m_pm = ExtensionSystem::PluginManager::instance();
    m_settings = settings;

    Core::ICore *coreIFace = m_pm->getObject<Core::ICore>();
    if (!coreIFace || !coreIFace->settings())
        return;
    QSettings *s = coreIFace->settings();
    s->beginGroup("GdbOptions");
    QString defaultCommand("gdb");
#if defined(Q_OS_WIN32)
    defaultCommand.append(".exe");
#endif
    m_settings->m_gdbCmd   = s->value("Location", defaultCommand).toString();
    m_settings->m_gdbEnv   = s->value("Environment", "").toString();
    m_settings->m_autoRun  = s->value("AutoRun", true).toBool();
    m_settings->m_autoQuit = s->value("AutoQuit", true).toBool();
    s->endGroup();
}

QString GdbOptionPage::name() const
{
    return tr("Gdb");
}

QString GdbOptionPage::category() const
{
    return "Debugger|Gdb";
}

QString GdbOptionPage::trCategory() const
{
    return tr("Debugger|Gdb");
}

QWidget *GdbOptionPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_ui.gdbEdit->setText(m_settings->m_gdbCmd);
    m_ui.envEdit->setText(m_settings->m_gdbEnv);
    m_ui.autoStartBox->setChecked(m_settings->m_autoRun);
    m_ui.autoQuitBox->setChecked(m_settings->m_autoQuit);
    connect(m_ui.pushButtonBrowse, SIGNAL(clicked()),
        this, SLOT(browse()));

    return w;
}

void GdbOptionPage::browse()
{
    QString fileName = QFileDialog::getOpenFileName(m_ui.pushButtonBrowse,
            "Browse for gdb executable");
    if (fileName.isEmpty())
        return;
    m_settings->m_gdbCmd = fileName;
    m_ui.gdbEdit->setText(fileName);
}

void GdbOptionPage::finished(bool accepted)
{
    if (!accepted)
        return;

    m_settings->m_gdbCmd   = m_ui.gdbEdit->text();
    m_settings->m_gdbEnv   = m_ui.envEdit->text();
    m_settings->m_autoRun  = m_ui.autoStartBox->isChecked();
    m_settings->m_autoQuit = m_ui.autoQuitBox->isChecked();

    Core::ICore *coreIFace = m_pm->getObject<Core::ICore>();
    if (!coreIFace || !coreIFace->settings())
        return;

    QSettings *s = coreIFace->settings();

    s->beginGroup("GdbOptions");
    s->setValue("Location", m_settings->m_gdbCmd);
    s->setValue("Environment", m_settings->m_gdbEnv);
    s->setValue("AutoRun", m_settings->m_autoRun);
    s->setValue("AutoQuit", m_settings->m_autoQuit);
    s->endGroup();
}
