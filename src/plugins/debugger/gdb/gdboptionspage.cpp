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

#include "gdboptionspage.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"

#include <coreplugin/icore.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

namespace Debugger {
namespace Internal {

GdbOptionsPage::GdbOptionsPage()
{
}

QString GdbOptionsPage::settingsId()
{
    return QLatin1String("M.Gdb");
}

QString GdbOptionsPage::displayName() const
{
    return tr("Gdb");
}

QString GdbOptionsPage::category() const
{
    return QLatin1String(Debugger::Constants::DEBUGGER_SETTINGS_CATEGORY);
}

QString GdbOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger", Debugger::Constants::DEBUGGER_SETTINGS_TR_CATEGORY);
}

QIcon GdbOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Debugger::Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

QWidget *GdbOptionsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_ui.gdbLocationChooser->setExpectedKind(Utils::PathChooser::Command);
    m_ui.gdbLocationChooser->setPromptDialogTitle(tr("Choose Gdb Location"));
    m_ui.scriptFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui.scriptFileChooser->setPromptDialogTitle(tr("Choose Location of Startup Script File"));

    m_group.clear();
    m_group.insert(theDebuggerAction(GdbLocation),
        m_ui.gdbLocationChooser);
    m_group.insert(theDebuggerAction(GdbScriptFile),
        m_ui.scriptFileChooser);
    m_group.insert(theDebuggerAction(GdbEnvironment),
        m_ui.environmentEdit);
    m_group.insert(theDebuggerAction(UsePreciseBreakpoints),
        m_ui.checkBoxUsePreciseBreakpoints);
    m_group.insert(theDebuggerAction(GdbWatchdogTimeout),
        m_ui.spinBoxGdbWatchdogTimeout);


#if 1
    m_ui.groupBoxPluginDebugging->hide();
#else // The related code (handleAqcuiredInferior()) is disabled as well.
    m_group.insert(theDebuggerAction(AllPluginBreakpoints),
        m_ui.radioButtonAllPluginBreakpoints);
    m_group.insert(theDebuggerAction(SelectedPluginBreakpoints),
        m_ui.radioButtonSelectedPluginBreakpoints);
    m_group.insert(theDebuggerAction(NoPluginBreakpoints),
        m_ui.radioButtonNoPluginBreakpoints);
    m_group.insert(theDebuggerAction(SelectedPluginBreakpointsPattern),
        m_ui.lineEditSelectedPluginBreakpointsPattern);
#endif

    m_ui.lineEditSelectedPluginBreakpointsPattern->
        setEnabled(theDebuggerAction(SelectedPluginBreakpoints)->value().toBool());
    connect(m_ui.radioButtonSelectedPluginBreakpoints, SIGNAL(toggled(bool)),
        m_ui.lineEditSelectedPluginBreakpointsPattern, SLOT(setEnabled(bool)));

    // FIXME
    m_ui.environmentEdit->hide();
    m_ui.labelEnvironment->hide();

    if (m_searchKeywords.isEmpty()) {
        // TODO: Add breakpoints, environment?
        QTextStream(&m_searchKeywords) << ' ' << m_ui.labelGdbLocation->text()
                << ' ' << m_ui.labelEnvironment->text()
                << ' ' << m_ui.labelGdbStartupScript->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
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
