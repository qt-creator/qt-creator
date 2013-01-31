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
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorer.h>

#include <QLabel>
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
    QGroupBox *behaviorBox = new QGroupBox(this);
    behaviorBox->setTitle(tr("Behavior"));

    checkBoxUseAlternatingRowColors = new QCheckBox(behaviorBox);
    checkBoxUseAlternatingRowColors->setText(tr("Use alternating row colors in debug views"));

    checkBoxFontSizeFollowsEditor = new QCheckBox(behaviorBox);
    checkBoxFontSizeFollowsEditor->setToolTip(tr("Change the font size in the debugger views when the font size in the main editor changes."));
    checkBoxFontSizeFollowsEditor->setText(tr("Debugger font size follows main editor"));

    checkBoxUseToolTipsInMainEditor = new QCheckBox(behaviorBox);
    checkBoxUseToolTipsInMainEditor->setText(tr("Use tooltips in main editor while debugging"));

    checkBoxListSourceFiles = new QCheckBox(behaviorBox);
    checkBoxListSourceFiles->setToolTip(tr("Populate the source file view automatically. This might slow down debugger startup considerably."));
    checkBoxListSourceFiles->setText(tr("Populate source file view automatically"));

    checkBoxCloseBuffersOnExit = new QCheckBox(behaviorBox);
    checkBoxCloseBuffersOnExit->setText(tr("Close temporary buffers on debugger exit"));

    checkBoxSwitchModeOnExit = new QCheckBox(behaviorBox);
    checkBoxSwitchModeOnExit->setText(tr("Switch to previous mode on debugger exit"));

    checkBoxBringToForegroundOnInterrrupt = new QCheckBox(behaviorBox);
    checkBoxBringToForegroundOnInterrrupt->setText(tr("Bring Qt Creator to foreground when application interrupts"));

    checkBoxShowQmlObjectTree = new QCheckBox(behaviorBox);
    checkBoxShowQmlObjectTree->setToolTip(tr("Show QML object tree in Locals & Expressions when connected and not stepping."));
    checkBoxShowQmlObjectTree->setText(tr("Show QML object tree"));

    checkBoxBreakpointsFullPath = new QCheckBox(behaviorBox);
    checkBoxBreakpointsFullPath->setToolTip(tr("Enable a full file path in breakpoints by default also for the GDB"));
    checkBoxBreakpointsFullPath->setText(tr("Breakpoints full path by default"));

    checkBoxRegisterForPostMortem = new QCheckBox(behaviorBox);
    checkBoxRegisterForPostMortem->setToolTip(tr("Register Qt Creator for debugging crashed applications."));
    checkBoxRegisterForPostMortem->setText(tr("Use Qt Creator for post-mortem debugging"));

    labelMaximalStackDepth = new QLabel(tr("Maximum stack depth:"), behaviorBox);

    spinBoxMaximalStackDepth = new QSpinBox(behaviorBox);
    spinBoxMaximalStackDepth->setSpecialValueText(tr("<unlimited>"));
    spinBoxMaximalStackDepth->setMaximum(999);
    spinBoxMaximalStackDepth->setSingleStep(5);
    spinBoxMaximalStackDepth->setValue(10);

    labelMaximalStringLength = new QLabel(tr("Maximum string length:"), behaviorBox);

    spinBoxMaximalStringLength = new QSpinBox(behaviorBox);
    spinBoxMaximalStringLength->setSpecialValueText(tr("<unlimited>"));
    spinBoxMaximalStringLength->setMaximum(10000000);
    spinBoxMaximalStringLength->setSingleStep(1000);
    spinBoxMaximalStringLength->setValue(10000);

    sourcesMappingWidget = new DebuggerSourcePathMappingWidget(this);

    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(labelMaximalStackDepth);
    horizontalLayout->addWidget(spinBoxMaximalStackDepth);
    horizontalLayout->addStretch();

    QHBoxLayout *horizontalLayout2 = new QHBoxLayout();
    horizontalLayout2->addWidget(labelMaximalStringLength);
    horizontalLayout2->addWidget(spinBoxMaximalStringLength);
    horizontalLayout2->addStretch();

    QGridLayout *gridLayout = new QGridLayout(behaviorBox);
    gridLayout->addWidget(checkBoxUseAlternatingRowColors, 0, 0, 1, 1);
    gridLayout->addWidget(checkBoxUseToolTipsInMainEditor, 1, 0, 1, 1);
    gridLayout->addWidget(checkBoxCloseBuffersOnExit, 2, 0, 1, 1);
    gridLayout->addWidget(checkBoxBringToForegroundOnInterrrupt, 3, 0, 1, 1);
    gridLayout->addWidget(checkBoxBreakpointsFullPath, 4, 0, 1, 1);
    gridLayout->addLayout(horizontalLayout, 6, 0, 1, 2);

    gridLayout->addWidget(checkBoxFontSizeFollowsEditor, 0, 1, 1, 1);
    gridLayout->addWidget(checkBoxListSourceFiles, 1, 1, 1, 1);
    gridLayout->addWidget(checkBoxSwitchModeOnExit, 2, 1, 1, 1);
    gridLayout->addWidget(checkBoxShowQmlObjectTree, 3, 1, 1, 1);
    gridLayout->addWidget(checkBoxRegisterForPostMortem, 4, 1, 1, 1);
    gridLayout->addLayout(horizontalLayout2, 6, 1, 1, 2);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(behaviorBox);
    verticalLayout->addWidget(sourcesMappingWidget);
    verticalLayout->addStretch();

    DebuggerCore *dc = debuggerCore();
    m_group->clear();

    m_group->insert(dc->action(ListSourceFiles),
        checkBoxListSourceFiles);
    m_group->insert(dc->action(UseAlternatingRowColors),
        checkBoxUseAlternatingRowColors);
    m_group->insert(dc->action(UseToolTipsInMainEditor),
        checkBoxUseToolTipsInMainEditor);
    m_group->insert(dc->action(CloseBuffersOnExit),
        checkBoxCloseBuffersOnExit);
    m_group->insert(dc->action(SwitchModeOnExit),
        checkBoxSwitchModeOnExit);
    m_group->insert(dc->action(BreakpointsFullPathByDefault),
        checkBoxBreakpointsFullPath);
    m_group->insert(dc->action(RaiseOnInterrupt),
        checkBoxBringToForegroundOnInterrrupt);
    m_group->insert(dc->action(ShowQmlObjectTree),
        checkBoxShowQmlObjectTree);
    m_group->insert(dc->action(FontSizeFollowsEditor),
        checkBoxFontSizeFollowsEditor);
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
    m_group->insert(dc->action(MaximalStackDepth), spinBoxMaximalStackDepth);
    m_group->insert(dc->action(MaximalStringLength), spinBoxMaximalStringLength);
    m_group->insert(dc->action(ShowStdNamespace), 0);
    m_group->insert(dc->action(ShowQtNamespace), 0);
    m_group->insert(dc->action(SortStructMembers), 0);
    m_group->insert(dc->action(LogTimeStamps), 0);
    m_group->insert(dc->action(VerboseLog), 0);
    m_group->insert(dc->action(BreakOnThrow), 0);
    m_group->insert(dc->action(BreakOnCatch), 0);
    if (Utils::HostOsInfo::isWindowsHost()) {
        Utils::SavedAction *registerAction = dc->action(RegisterForPostMortem);
        m_group->insert(registerAction,
                checkBoxRegisterForPostMortem);
        connect(registerAction, SIGNAL(toggled(bool)),
                checkBoxRegisterForPostMortem, SLOT(setChecked(bool)));
    } else {
        checkBoxRegisterForPostMortem->setVisible(false);
    }
}

QString CommonOptionsPageWidget::searchKeyWords() const
{
    QString rc;
    const QLatin1Char sep(' ');
    QTextStream stream(&rc);
    stream << sep << checkBoxUseAlternatingRowColors->text()
           << sep << checkBoxFontSizeFollowsEditor->text()
           << sep << checkBoxUseToolTipsInMainEditor->text()
           << sep << checkBoxListSourceFiles->text()
           << sep << checkBoxBreakpointsFullPath->text()
           << sep << checkBoxCloseBuffersOnExit->text()
           << sep << checkBoxSwitchModeOnExit->text()
           << sep << labelMaximalStackDepth->text()
           << sep << checkBoxBringToForegroundOnInterrrupt->text()
           << sep << checkBoxShowQmlObjectTree->text();
    if (Utils::HostOsInfo::isWindowsHost())
        stream << sep << checkBoxRegisterForPostMortem->text();

    rc.remove(QLatin1Char('&'));
    return rc;
}

GlobalDebuggerOptions CommonOptionsPageWidget::globalOptions() const
{
    GlobalDebuggerOptions o;
    o.sourcePathMap = sourcesMappingWidget->sourcePathMap();
    return o;
}

void CommonOptionsPageWidget::setGlobalOptions(const GlobalDebuggerOptions &go)
{
    sourcesMappingWidget->setSourcePathMap(go.sourcePathMap);
}

///////////////////////////////////////////////////////////////////////
//
// CommonOptionsPage
//
///////////////////////////////////////////////////////////////////////

CommonOptionsPage::CommonOptionsPage(const QSharedPointer<GlobalDebuggerOptions> &go) :
    m_options(go)
{
    setId(DEBUGGER_COMMON_SETTINGS_ID);
    setDisplayName(QCoreApplication::translate("Debugger", "General"));
    setCategory(DEBUGGER_SETTINGS_CATEGORY);
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
    setId("Z.LocalsAndExpressions");
    //: '&&' will appear as one (one is marking keyboard shortcut)
    setDisplayName(QCoreApplication::translate("Debugger", "Locals && Expressions"));
    setCategory(DEBUGGER_SETTINGS_CATEGORY);
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
