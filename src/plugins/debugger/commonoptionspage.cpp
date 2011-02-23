/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "commonoptionspage.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/manhattanstyle.h>

#include <projectexplorer/projectexplorer.h>

#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

using namespace Core;
using namespace Debugger::Constants;
using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// CommonOptionsPage
//
///////////////////////////////////////////////////////////////////////

CommonOptionsPage::CommonOptionsPage()
{
}

QString CommonOptionsPage::id() const
{
    return _(DEBUGGER_COMMON_SETTINGS_ID);
}

QString CommonOptionsPage::displayName() const
{
    return QCoreApplication::translate("Debugger", DEBUGGER_COMMON_SETTINGS_NAME);}

QString CommonOptionsPage::category() const
{
    return _(DEBUGGER_SETTINGS_CATEGORY);
}

QString CommonOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger", DEBUGGER_SETTINGS_TR_CATEGORY);}

QIcon CommonOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

void CommonOptionsPage::apply()
{
    m_group.apply(ICore::instance()->settings());
}

void CommonOptionsPage::finish()
{
    m_group.finish();
}

QWidget *CommonOptionsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);
    m_group.clear();

    m_group.insert(debuggerCore()->action(ListSourceFiles),
        m_ui.checkBoxListSourceFiles);
    m_group.insert(debuggerCore()->action(UseAlternatingRowColors),
        m_ui.checkBoxUseAlternatingRowColors);
    m_group.insert(debuggerCore()->action(UseToolTipsInMainEditor),
        m_ui.checkBoxUseToolTipsInMainEditor);
    m_group.insert(debuggerCore()->action(CloseBuffersOnExit),
        m_ui.checkBoxCloseBuffersOnExit);
    m_group.insert(debuggerCore()->action(SwitchModeOnExit),
        m_ui.checkBoxSwitchModeOnExit);
    m_group.insert(debuggerCore()->action(AutoDerefPointers), 0);
    m_group.insert(debuggerCore()->action(UseToolTipsInLocalsView), 0);
    m_group.insert(debuggerCore()->action(UseToolTipsInBreakpointsView), 0);
    m_group.insert(debuggerCore()->action(UseAddressInBreakpointsView), 0);
    m_group.insert(debuggerCore()->action(UseAddressInStackView), 0);
    m_group.insert(debuggerCore()->action(MaximalStackDepth),
        m_ui.spinBoxMaximalStackDepth);
    m_group.insert(debuggerCore()->action(ShowStdNamespace), 0);
    m_group.insert(debuggerCore()->action(ShowQtNamespace), 0);
    m_group.insert(debuggerCore()->action(SortStructMembers), 0);
    m_group.insert(debuggerCore()->action(LogTimeStamps), 0);
    m_group.insert(debuggerCore()->action(VerboseLog), 0);
    m_group.insert(debuggerCore()->action(BreakOnThrow), 0);
    m_group.insert(debuggerCore()->action(BreakOnCatch), 0);
    m_group.insert(debuggerCore()->action(QtSourcesLocation),
        m_ui.qtSourcesChooser);
#ifdef Q_OS_WIN
    Utils::SavedAction *registerAction = debuggerCore()->action(RegisterForPostMortem);
    m_group.insert(registerAction,
        m_ui.checkBoxRegisterForPostMortem);
    connect(registerAction, SIGNAL(toggled(bool)),
            m_ui.checkBoxRegisterForPostMortem, SLOT(setChecked(bool)));
#endif

    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << sep << m_ui.checkBoxUseAlternatingRowColors->text()
                << sep << m_ui.checkBoxUseToolTipsInMainEditor->text()
                << sep << m_ui.checkBoxListSourceFiles->text()
#ifdef Q_OS_WIN
                << sep << m_ui.checkBoxRegisterForPostMortem->text()
#endif
                << sep << m_ui.checkBoxCloseBuffersOnExit->text()
                << sep << m_ui.checkBoxSwitchModeOnExit->text()
                << sep << m_ui.labelMaximalStackDepth->text()
                   ;
        m_searchKeywords.remove(QLatin1Char('&'));
    }
#ifndef Q_OS_WIN
    m_ui.checkBoxRegisterForPostMortem->setVisible(false);
#endif
    return w;
}

bool CommonOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

///////////////////////////////////////////////////////////////////////
//
// DebuggingHelperOptionPage
//
///////////////////////////////////////////////////////////////////////

static bool oxygenStyle()
{
    const ManhattanStyle *ms = qobject_cast<const ManhattanStyle *>(qApp->style());
    return ms && !qstrcmp("OxygenStyle", ms->baseStyle()->metaObject()->className());
}

QString DebuggingHelperOptionPage::id() const
{
    return _("Z.DebuggingHelper");
}

QString DebuggingHelperOptionPage::displayName() const
{
    return tr("Debugging Helper");
}

QString DebuggingHelperOptionPage::category() const
{
    return _(DEBUGGER_SETTINGS_CATEGORY);
}

QString DebuggingHelperOptionPage::displayCategory() const
{
    return QCoreApplication::translate("Debugger", DEBUGGER_SETTINGS_TR_CATEGORY);
}

QIcon DebuggingHelperOptionPage::categoryIcon() const
{
    return QIcon(QLatin1String(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

void DebuggingHelperOptionPage::apply()
{
    m_group.apply(ICore::instance()->settings());
}

void DebuggingHelperOptionPage::finish()
{
    m_group.finish();
}

QWidget *DebuggingHelperOptionPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);

    m_ui.dumperLocationChooser->setExpectedKind(Utils::PathChooser::Command);
    m_ui.dumperLocationChooser->setPromptDialogTitle(tr("Choose DebuggingHelper Location"));
    m_ui.dumperLocationChooser->setInitialBrowsePathBackup(
        ICore::instance()->resourcePath() + "../../lib");

    m_group.clear();
    m_group.insert(debuggerCore()->action(UseDebuggingHelpers),
        m_ui.debuggingHelperGroupBox);
    m_group.insert(debuggerCore()->action(UseCustomDebuggingHelperLocation),
        m_ui.customLocationGroupBox);
    // Suppress Oxygen style's giving flat group boxes bold titles.
    if (oxygenStyle())
        m_ui.customLocationGroupBox->setStyleSheet(_("QGroupBox::title { font: ; }"));

    m_group.insert(debuggerCore()->action(CustomDebuggingHelperLocation),
        m_ui.dumperLocationChooser);

    m_group.insert(debuggerCore()->action(UseCodeModel),
        m_ui.checkBoxUseCodeModel);
    m_group.insert(debuggerCore()->action(ShowThreadNames),
        m_ui.checkBoxShowThreadNames);


#ifndef QT_DEBUG
#if 0
    cmd = am->registerAction(m_dumpLogAction,
        DUMP_LOG, globalcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+L")));
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F11")));
    mdebug->addAction(cmd);
#endif
#endif

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords)
                << ' ' << m_ui.debuggingHelperGroupBox->title()
                << ' ' << m_ui.customLocationGroupBox->title()
                << ' ' << m_ui.dumperLocationLabel->text()
                << ' ' << m_ui.checkBoxUseCodeModel->text()
                << ' ' << m_ui.checkBoxShowThreadNames->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

bool DebuggingHelperOptionPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}


} // namespace Internal
} // namespace Debugger
