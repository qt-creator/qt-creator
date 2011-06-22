/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "gdboptionspage.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerinternalconstants.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>


namespace Debugger {
namespace Internal {

GdbOptionsPage::GdbOptionsPage()
    : m_ui(0)
{ }

QString GdbOptionsPage::settingsId()
{
    return QLatin1String("M.Gdb");
}

QString GdbOptionsPage::displayName() const
{
    return tr("GDB");
}

QString GdbOptionsPage::category() const
{
    return QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString GdbOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger", Constants::DEBUGGER_SETTINGS_TR_CATEGORY);
}

QIcon GdbOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

QWidget *GdbOptionsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui = new Ui::GdbOptionsPage;
    m_ui->setupUi(w);

    m_ui->scriptFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->scriptFileChooser->setPromptDialogTitle(
        tr("Choose Location of Startup Script File"));

    m_group.clear();
    m_group.insert(debuggerCore()->action(GdbScriptFile),
        m_ui->scriptFileChooser);
    m_group.insert(debuggerCore()->action(LoadGdbInit),
        m_ui->checkBoxLoadGdbInit);
    m_group.insert(debuggerCore()->action(TargetAsync),
        m_ui->checkBoxTargetAsync);
    m_group.insert(debuggerCore()->action(AdjustBreakpointLocations),
        m_ui->checkBoxAdjustBreakpointLocations);
    m_group.insert(debuggerCore()->action(BreakOnWarning),
        m_ui->checkBoxBreakOnWarning);
    m_group.insert(debuggerCore()->action(GdbWatchdogTimeout),
        m_ui->spinBoxGdbWatchdogTimeout);

    m_group.insert(debuggerCore()->action(UseMessageBoxForSignals),
        m_ui->checkBoxUseMessageBoxForSignals);
    m_group.insert(debuggerCore()->action(SkipKnownFrames),
        m_ui->checkBoxSkipKnownFrames);
    m_group.insert(debuggerCore()->action(EnableReverseDebugging),
        m_ui->checkBoxEnableReverseDebugging);
    m_group.insert(debuggerCore()->action(GdbWatchdogTimeout), 0);

    m_ui->groupBoxPluginDebugging->hide();

    m_ui->lineEditSelectedPluginBreakpointsPattern->
        setEnabled(debuggerCore()->action(SelectedPluginBreakpoints)->value().toBool());
    connect(m_ui->radioButtonSelectedPluginBreakpoints, SIGNAL(toggled(bool)),
        m_ui->lineEditSelectedPluginBreakpointsPattern, SLOT(setEnabled(bool)));

    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << sep << m_ui->groupBoxLocations->title()
                << sep << m_ui->checkBoxLoadGdbInit->text()
                << sep << m_ui->checkBoxTargetAsync->text()
                << sep << m_ui->labelGdbStartupScript->text()
                << sep << m_ui->labelGdbWatchdogTimeout->text()
                << sep << m_ui->checkBoxEnableReverseDebugging->text()
                << sep << m_ui->checkBoxSkipKnownFrames->text()
                << sep << m_ui->checkBoxUseMessageBoxForSignals->text()
                << sep << m_ui->checkBoxAdjustBreakpointLocations->text()
                << sep << m_ui->groupBoxPluginDebugging->title()
                << sep << m_ui->radioButtonAllPluginBreakpoints->text()
                << sep << m_ui->radioButtonSelectedPluginBreakpoints->text()
                << sep << m_ui->labelSelectedPluginBreakpoints->text()
                << sep << m_ui->radioButtonNoPluginBreakpoints->text()
                   ;
        m_searchKeywords.remove(QLatin1Char('&'));
    }

    // FIXME: Not fully working on the gdb side yet.
    m_ui->checkBoxTargetAsync->hide();

    return w;
}

void GdbOptionsPage::apply()
{
    m_group.apply(Core::ICore::instance()->settings());
}

void GdbOptionsPage::finish()
{
    m_group.finish();
}

bool GdbOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace Debugger
