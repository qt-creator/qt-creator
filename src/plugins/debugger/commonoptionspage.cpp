/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "commonoptionspage.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/manhattanstyle.h>
#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>
#include <QTextStream>

using namespace Core;
using namespace Debugger::Constants;
using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

CommonOptionsPageWidget::CommonOptionsPageWidget
    (const QSharedPointer<Utils::SavedActionSet> &group, QWidget *parent)
  : QWidget(parent), m_group(group)
{
    m_ui.setupUi(this);

    DebuggerCore *dc = debuggerCore();
    m_group->clear();

    m_group->insert(dc->action(ListSourceFiles),
        m_ui.checkBoxListSourceFiles);
    m_group->insert(dc->action(UseAlternatingRowColors),
        m_ui.checkBoxUseAlternatingRowColors);
    m_group->insert(dc->action(UseToolTipsInMainEditor),
        m_ui.checkBoxUseToolTipsInMainEditor);
    m_group->insert(dc->action(CloseBuffersOnExit),
        m_ui.checkBoxCloseBuffersOnExit);
    m_group->insert(dc->action(SwitchModeOnExit),
        m_ui.checkBoxSwitchModeOnExit);
    m_group->insert(dc->action(BreakpointsFullPathByDefault),
        m_ui.checkBoxBreakpointsFullPath);
    m_group->insert(dc->action(RaiseOnInterrupt),
        m_ui.checkBoxBringToForegroundOnInterrrupt);
    m_group->insert(dc->action(ShowQmlObjectTree),
        m_ui.checkBoxShowQmlObjectTree);
    m_group->insert(dc->action(FontSizeFollowsEditor),
        m_ui.checkBoxFontSizeFollowsEditor);
    m_group->insert(dc->action(AutoDerefPointers), 0);
    m_group->insert(dc->action(UseToolTipsInLocalsView), 0);
    m_group->insert(dc->action(AlwaysAdjustLocalsColumnWidths), 0);
    m_group->insert(dc->action(AlwaysAdjustThreadsColumnWidths), 0);
    m_group->insert(dc->action(AlwaysAdjustSnapshotsColumnWidths), 0);
    m_group->insert(dc->action(AlwaysAdjustBreakpointsColumnWidths), 0);
    m_group->insert(dc->action(AlwaysAdjustModulesColumnWidths), 0);
    m_group->insert(dc->action(UseToolTipsInBreakpointsView), 0);
    m_group->insert(dc->action(UseAddressInBreakpointsView), 0);
    m_group->insert(dc->action(UseAddressInStackView), 0);
    m_group->insert(dc->action(AlwaysAdjustStackColumnWidths), 0);
    m_group->insert(dc->action(MaximalStackDepth),
        m_ui.spinBoxMaximalStackDepth);
    m_group->insert(dc->action(ShowStdNamespace), 0);
    m_group->insert(dc->action(ShowQtNamespace), 0);
    m_group->insert(dc->action(SortStructMembers), 0);
    m_group->insert(dc->action(LogTimeStamps), 0);
    m_group->insert(dc->action(VerboseLog), 0);
    m_group->insert(dc->action(BreakOnThrow), 0);
    m_group->insert(dc->action(BreakOnCatch), 0);
#ifdef Q_OS_WIN
    Utils::SavedAction *registerAction = dc->action(RegisterForPostMortem);
    m_group->insert(registerAction,
        m_ui.checkBoxRegisterForPostMortem);
    connect(registerAction, SIGNAL(toggled(bool)),
            m_ui.checkBoxRegisterForPostMortem, SLOT(setChecked(bool)));
#else
    m_ui.checkBoxRegisterForPostMortem->setVisible(false);
#endif
}

QString CommonOptionsPageWidget::searchKeyWords() const
{
    QString rc;
    const QLatin1Char sep(' ');
    QTextStream(&rc)
            << sep << m_ui.checkBoxUseAlternatingRowColors->text()
            << sep << m_ui.checkBoxFontSizeFollowsEditor->text()
            << sep << m_ui.checkBoxUseToolTipsInMainEditor->text()
            << sep << m_ui.checkBoxListSourceFiles->text()
            << sep << m_ui.checkBoxBreakpointsFullPath->text()
#ifdef Q_OS_WIN
            << sep << m_ui.checkBoxRegisterForPostMortem->text()
#endif
            << sep << m_ui.checkBoxCloseBuffersOnExit->text()
            << sep << m_ui.checkBoxSwitchModeOnExit->text()
            << sep << m_ui.labelMaximalStackDepth->text()
            << sep << m_ui.checkBoxBringToForegroundOnInterrrupt->text()
            << sep << m_ui.checkBoxShowQmlObjectTree->text()
               ;
    rc.remove(QLatin1Char('&'));
    return rc;
}

GlobalDebuggerOptions CommonOptionsPageWidget::globalOptions() const
{
    GlobalDebuggerOptions o;
    o.sourcePathMap = m_ui.sourcesMappingWidget->sourcePathMap();
    return o;
}

void CommonOptionsPageWidget::setGlobalOptions(const GlobalDebuggerOptions &go)
{
    m_ui.sourcesMappingWidget->setSourcePathMap(go.sourcePathMap);
}

///////////////////////////////////////////////////////////////////////
//
// CommonOptionsPage
//
///////////////////////////////////////////////////////////////////////

CommonOptionsPage::CommonOptionsPage(const QSharedPointer<GlobalDebuggerOptions> &go) :
    m_options(go)
{
    setId(QLatin1String(DEBUGGER_COMMON_SETTINGS_ID));
    setDisplayName(QCoreApplication::translate("Debugger", "General"));
    setCategory(QLatin1String(DEBUGGER_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Debugger", DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

CommonOptionsPage::~CommonOptionsPage()
{
}

void CommonOptionsPage::apply()
{
    QTC_ASSERT(!m_widget.isNull() && !m_group.isNull(), return);

    QSettings *settings = ICore::settings();
    m_group->apply(settings);

    const GlobalDebuggerOptions newGlobalOptions = m_widget->globalOptions();
    if (newGlobalOptions != *m_options) {
        *m_options = newGlobalOptions;
        m_options->toSettings(settings);
    }
}

void CommonOptionsPage::finish()
{
    if (!m_group.isNull())
        m_group->finish();
}

QWidget *CommonOptionsPage::createPage(QWidget *parent)
{
    if (m_group.isNull())
        m_group = QSharedPointer<Utils::SavedActionSet>(new Utils::SavedActionSet);
    m_widget = new CommonOptionsPageWidget(m_group, parent);
    m_widget->setGlobalOptions(*m_options);
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeyWords();
    return m_widget;
}

bool CommonOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

QString CommonOptionsPage::msgSetBreakpointAtFunction(const char *function)
{
    return tr("Stop when %1() is called").arg(QLatin1String(function));
}

QString CommonOptionsPage::msgSetBreakpointAtFunctionToolTip(const char *function,
                                                             const QString &hint)
{
    QString result = QLatin1String("<html><head/><body>");
    result += tr("Always add a breakpoint on the <i>%1()</i> function.").arg(QLatin1String(function));
    if (!hint.isEmpty()) {
        result += QLatin1String("<br>");
        result += hint;
    }
    result += QLatin1String("</body></html>");
    return result;
}

///////////////////////////////////////////////////////////////////////
//
// LocalsAndExpressionsOptionsPage
//
///////////////////////////////////////////////////////////////////////

LocalsAndExpressionsOptionsPage::LocalsAndExpressionsOptionsPage()
{
    setId(QLatin1String("Z.LocalsAndExpressions"));
    //: '&&' will appear as one (one is marking keyboard shortcut)
    setDisplayName(QCoreApplication::translate("Debugger", "Locals && Expressions"));
    setCategory(QLatin1String(DEBUGGER_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Debugger", DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

void LocalsAndExpressionsOptionsPage::apply()
{
    m_group.apply(ICore::settings());
}

void LocalsAndExpressionsOptionsPage::finish()
{
    m_group.finish();
}

QWidget *LocalsAndExpressionsOptionsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui.setupUi(w);

    m_group.clear();
    DebuggerCore *dc = debuggerCore();

    m_group.insert(dc->action(UseDebuggingHelpers),
        m_ui.debuggingHelperGroupBox);

    m_group.insert(dc->action(UseCodeModel),
        m_ui.checkBoxUseCodeModel);
    m_ui.checkBoxUseCodeModel->setToolTip(dc->action(UseCodeModel)->toolTip());

    m_group.insert(dc->action(ShowThreadNames),
        m_ui.checkBoxShowThreadNames);
    m_group.insert(dc->action(ShowStdNamespace), m_ui.checkBoxShowStdNamespace);
    m_group.insert(dc->action(ShowQtNamespace), m_ui.checkBoxShowQtNamespace);


#ifndef QT_DEBUG
#if 0
    cmd = am->registerAction(m_dumpLogAction,
        DUMP_LOG, globalcontext);
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+L")));
    cmd->setDefaultKeySequence(QKeySequence(QCoreApplication::translate("Debugger", "Ctrl+Shift+F11")));
    mdebug->addAction(cmd);
#endif
#endif

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords)
                << ' ' << m_ui.debuggingHelperGroupBox->title()
                << ' ' << m_ui.checkBoxUseCodeModel->text()
                << ' ' << m_ui.checkBoxShowThreadNames->text()
                << ' ' << m_ui.checkBoxShowStdNamespace->text()
                << ' ' << m_ui.checkBoxShowQtNamespace->text();

        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

bool LocalsAndExpressionsOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}


} // namespace Internal
} // namespace Debugger
