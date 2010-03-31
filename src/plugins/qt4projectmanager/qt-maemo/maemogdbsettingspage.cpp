/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "maemogdbsettingspage.h"
#include "ui_maemogdbwidget.h"

#include <coreplugin/icore.h>
#include <debugger/debuggerconstants.h>

#include <QtCore/QSettings>
#include <QtCore/QTextStream>

using namespace Qt4ProjectManager::Internal;

// -- MaemoGdbWidget

MaemoGdbWidget::MaemoGdbWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MaemoGdbWidget)
{
    ui->setupUi(this);

    QSettings *settings = Core::ICore::instance()->settings();
    ui->gdbChooser->setPath(settings->value(QLatin1String("MaemoDebugger/gdb"),
        QLatin1String("")).toString());
    const bool checked = settings->value(QLatin1String("MaemoDebugger/useMaddeGdb"),
        true).toBool();
    ui->useMaddeGdb->setChecked(checked);
    toogleGdbPathChooser(checked ? Qt::Checked : Qt::Unchecked);

    ui->gdbChooser->setExpectedKind(Utils::PathChooser::Command);
    connect(ui->useMaddeGdb, SIGNAL(stateChanged(int)), this,
        SLOT(toogleGdbPathChooser(int)));
}

MaemoGdbWidget::~MaemoGdbWidget()
{
    delete ui;
}

void MaemoGdbWidget::saveSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->setValue(QLatin1String("MaemoDebugger/useMaddeGdb"),
        ui->useMaddeGdb->isChecked());
    settings->setValue(QLatin1String("MaemoDebugger/gdb"), ui->gdbChooser->path());
}

QString MaemoGdbWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << ui->gdbLabel->text() << ui->useMaddeGdb->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

void MaemoGdbWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

void MaemoGdbWidget::toogleGdbPathChooser(int state)
{
    bool enable = false;
    if (state == Qt::Unchecked)
        enable = true;
    ui->gdbLabel->setEnabled(enable);
    ui->gdbChooser->setEnabled(enable);
}

// -- MaemoGdbSettingsPage

MaemoGdbSettingsPage::MaemoGdbSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
}

MaemoGdbSettingsPage::~MaemoGdbSettingsPage()
{
}

QString MaemoGdbSettingsPage::id() const
{
    return QLatin1String("ZZ.Maemo");
}

QString MaemoGdbSettingsPage::displayName() const
{
    return tr("Maemo");
}

QString MaemoGdbSettingsPage::category() const
{
    return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString MaemoGdbSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger",
        Debugger::Constants::DEBUGGER_SETTINGS_TR_CATEGORY);
}

QIcon MaemoGdbSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Debugger::Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

QWidget *MaemoGdbSettingsPage::createPage(QWidget *parent)
{
    m_widget = new MaemoGdbWidget(parent);
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void MaemoGdbSettingsPage::apply()
{
    m_widget->saveSettings();
    emit updateGdbLocation();
}

void MaemoGdbSettingsPage::finish()
{
}

bool MaemoGdbSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
