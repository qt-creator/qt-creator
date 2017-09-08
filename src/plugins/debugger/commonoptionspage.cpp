/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "commonoptionspage.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"

#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>

#include <app/app_version.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QTextEdit>
#include <QTextStream>

using namespace Core;
using namespace Debugger::Constants;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {
namespace Internal {

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
    setCategoryIcon(Icon(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

void CommonOptionsPage::apply()
{
    m_group.apply(ICore::settings());

    GlobalDebuggerOptions newOptions;
    SourcePathMap allPathMap = m_sourceMappingWidget->sourcePathMap();
    for (auto it = allPathMap.begin(), end = allPathMap.end(); it != end; ++it) {
        const QString key = it.key();
        if (key.startsWith(QLatin1Char('(')))
            newOptions.sourcePathRegExpMap.append(qMakePair(QRegExp(key), it.value()));
        else
            newOptions.sourcePathMap.insert(key, it.value());
    }

    if (newOptions.sourcePathMap != m_options->sourcePathMap
            || newOptions.sourcePathRegExpMap != m_options->sourcePathRegExpMap) {
        *m_options = newOptions;
        m_options->toSettings();
    }
}

void CommonOptionsPage::finish()
{
    m_group.finish();
    delete m_widget;
}

QWidget *CommonOptionsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;

        auto behaviorBox = new QGroupBox(m_widget);
        behaviorBox->setTitle(tr("Behavior"));

        auto checkBoxUseAlternatingRowColors = new QCheckBox(behaviorBox);
        checkBoxUseAlternatingRowColors->setText(tr("Use alternating row colors in debug views"));

        auto checkBoxFontSizeFollowsEditor = new QCheckBox(behaviorBox);
        checkBoxFontSizeFollowsEditor->setToolTip(tr("Changes the font size in the debugger views when the font size in the main editor changes."));
        checkBoxFontSizeFollowsEditor->setText(tr("Debugger font size follows main editor"));

        auto checkBoxUseToolTipsInMainEditor = new QCheckBox(behaviorBox);
        checkBoxUseToolTipsInMainEditor->setText(tr("Use tooltips in main editor while debugging"));

        QString t = tr("Stopping and stepping in the debugger "
                       "will automatically open views associated with the current location.") + QLatin1Char('\n');
        auto checkBoxCloseSourceBuffersOnExit = new QCheckBox(behaviorBox);
        checkBoxCloseSourceBuffersOnExit->setText(tr("Close temporary source views on debugger exit"));
        checkBoxCloseSourceBuffersOnExit->setToolTip(t + tr("Closes automatically opened source views when the debugger exits."));

        auto checkBoxCloseMemoryBuffersOnExit = new QCheckBox(behaviorBox);
        checkBoxCloseMemoryBuffersOnExit->setText(tr("Close temporary memory views on debugger exit"));
        checkBoxCloseMemoryBuffersOnExit->setToolTip(t + tr("Closes automatically opened memory views when the debugger exits."));

        auto checkBoxSwitchModeOnExit = new QCheckBox(behaviorBox);
        checkBoxSwitchModeOnExit->setText(tr("Switch to previous mode on debugger exit"));

        auto checkBoxBringToForegroundOnInterrrupt = new QCheckBox(behaviorBox);
        checkBoxBringToForegroundOnInterrrupt->setText(
                    tr("Bring %1 to foreground when application interrupts")
                    .arg(Core::Constants::IDE_DISPLAY_NAME));

        auto checkBoxShowQmlObjectTree = new QCheckBox(behaviorBox);
        checkBoxShowQmlObjectTree->setToolTip(tr("Shows QML object tree in Locals and Expressions when connected and not stepping."));
        checkBoxShowQmlObjectTree->setText(tr("Show QML object tree"));

        auto checkBoxBreakpointsFullPath = new QCheckBox(behaviorBox);
        checkBoxBreakpointsFullPath->setToolTip(tr("Enables a full file path in breakpoints by default also for GDB."));
        checkBoxBreakpointsFullPath->setText(tr("Set breakpoints using a full absolute path"));

        auto checkBoxRegisterForPostMortem = new QCheckBox(behaviorBox);
        checkBoxRegisterForPostMortem->setToolTip(
                    tr("Registers %1 for debugging crashed applications.")
                    .arg(Core::Constants::IDE_DISPLAY_NAME));
        checkBoxRegisterForPostMortem->setText(
                    tr("Use %1 for post-mortem debugging")
                    .arg(Core::Constants::IDE_DISPLAY_NAME));

        auto checkBoxWarnOnReleaseBuilds = new QCheckBox(behaviorBox);
        checkBoxWarnOnReleaseBuilds->setText(tr("Warn when debugging \"Release\" builds"));
        checkBoxWarnOnReleaseBuilds->setToolTip(tr("Shows a warning when starting the debugger "
                                                   "on a binary with insufficient debug information."));

        auto checkBoxKeepEditorStationaryWhileStepping = new QCheckBox(behaviorBox);
        checkBoxKeepEditorStationaryWhileStepping->setText(tr("Keep editor stationary when stepping"));
        checkBoxKeepEditorStationaryWhileStepping->setToolTip(tr("Scrolls the editor only when it is necessary "
                                                                 "to keep the current line in view, "
                                                                 "instead of keeping the next statement centered at "
                                                                 "all times."));

        auto labelMaximalStackDepth = new QLabel(tr("Maximum stack depth:"), behaviorBox);

        auto spinBoxMaximalStackDepth = new QSpinBox(behaviorBox);
        spinBoxMaximalStackDepth->setSpecialValueText(tr("<unlimited>"));
        spinBoxMaximalStackDepth->setMaximum(999);
        spinBoxMaximalStackDepth->setSingleStep(5);
        spinBoxMaximalStackDepth->setValue(10);

        m_sourceMappingWidget = new DebuggerSourcePathMappingWidget(m_widget);

        auto horizontalLayout = new QHBoxLayout;
        horizontalLayout->addWidget(labelMaximalStackDepth);
        horizontalLayout->addWidget(spinBoxMaximalStackDepth);
        horizontalLayout->addStretch();

        auto gridLayout = new QGridLayout(behaviorBox);
        gridLayout->addWidget(checkBoxUseAlternatingRowColors, 0, 0, 1, 1);
        gridLayout->addWidget(checkBoxUseToolTipsInMainEditor, 1, 0, 1, 1);
        gridLayout->addWidget(checkBoxCloseSourceBuffersOnExit, 2, 0, 1, 1);
        gridLayout->addWidget(checkBoxCloseMemoryBuffersOnExit, 3, 0, 1, 1);
        gridLayout->addWidget(checkBoxBringToForegroundOnInterrrupt, 4, 0, 1, 1);
        gridLayout->addWidget(checkBoxBreakpointsFullPath, 5, 0, 1, 1);
        gridLayout->addWidget(checkBoxWarnOnReleaseBuilds, 6, 0, 1, 1);
        gridLayout->addLayout(horizontalLayout, 7, 0, 1, 2);

        gridLayout->addWidget(checkBoxFontSizeFollowsEditor, 0, 1, 1, 1);
        gridLayout->addWidget(checkBoxSwitchModeOnExit, 1, 1, 1, 1);
        gridLayout->addWidget(checkBoxShowQmlObjectTree, 2, 1, 1, 1);
        gridLayout->addWidget(checkBoxKeepEditorStationaryWhileStepping, 3, 1, 1, 1);
        gridLayout->addWidget(checkBoxRegisterForPostMortem, 4, 1, 1, 1);

        auto verticalLayout = new QVBoxLayout(m_widget);
        verticalLayout->addWidget(behaviorBox);
        verticalLayout->addWidget(m_sourceMappingWidget);
        verticalLayout->addStretch();

        m_group.clear();

        m_group.insert(action(UseAlternatingRowColors),
                       checkBoxUseAlternatingRowColors);
        m_group.insert(action(UseToolTipsInMainEditor),
                       checkBoxUseToolTipsInMainEditor);
        m_group.insert(action(CloseSourceBuffersOnExit),
                       checkBoxCloseSourceBuffersOnExit);
        m_group.insert(action(CloseMemoryBuffersOnExit),
                       checkBoxCloseMemoryBuffersOnExit);
        m_group.insert(action(SwitchModeOnExit),
                       checkBoxSwitchModeOnExit);
        m_group.insert(action(BreakpointsFullPathByDefault),
                       checkBoxBreakpointsFullPath);
        m_group.insert(action(RaiseOnInterrupt),
                       checkBoxBringToForegroundOnInterrrupt);
        m_group.insert(action(ShowQmlObjectTree),
                       checkBoxShowQmlObjectTree);
        m_group.insert(action(WarnOnReleaseBuilds),
                       checkBoxWarnOnReleaseBuilds);
        m_group.insert(action(StationaryEditorWhileStepping),
                       checkBoxKeepEditorStationaryWhileStepping);
        m_group.insert(action(FontSizeFollowsEditor),
                       checkBoxFontSizeFollowsEditor);
        m_group.insert(action(AutoDerefPointers), 0);
        m_group.insert(action(UseToolTipsInLocalsView), 0);
        m_group.insert(action(AlwaysAdjustColumnWidths), 0);
        m_group.insert(action(UseToolTipsInBreakpointsView), 0);
        m_group.insert(action(UseToolTipsInStackView), 0);
        m_group.insert(action(UseAddressInBreakpointsView), 0);
        m_group.insert(action(UseAddressInStackView), 0);
        m_group.insert(action(MaximalStackDepth), spinBoxMaximalStackDepth);
        m_group.insert(action(ShowStdNamespace), 0);
        m_group.insert(action(ShowQtNamespace), 0);
        m_group.insert(action(ShowQObjectNames), 0);
        m_group.insert(action(SortStructMembers), 0);
        m_group.insert(action(LogTimeStamps), 0);
        m_group.insert(action(BreakOnThrow), 0);
        m_group.insert(action(BreakOnCatch), 0);
        if (HostOsInfo::isWindowsHost()) {
            SavedAction *registerAction = action(RegisterForPostMortem);
            m_group.insert(registerAction, checkBoxRegisterForPostMortem);
            connect(registerAction, &QAction::toggled,
                    checkBoxRegisterForPostMortem, &QAbstractButton::setChecked);
        } else {
            checkBoxRegisterForPostMortem->setVisible(false);
        }

        SourcePathMap allPathMap = m_options->sourcePathMap;
        foreach (auto regExpMap, m_options->sourcePathRegExpMap)
            allPathMap.insert(regExpMap.first.pattern(), regExpMap.second);
        m_sourceMappingWidget->setSourcePathMap(allPathMap);
    }
    return m_widget;
}

QString CommonOptionsPage::msgSetBreakpointAtFunction(const char *function)
{
    return tr("Stop when %1() is called").arg(QLatin1String(function));
}

QString CommonOptionsPage::msgSetBreakpointAtFunctionToolTip(const char *function,
                                                             const QString &hint)
{
    QString result = QLatin1String("<html><head/><body>");
    result += tr("Always adds a breakpoint on the <i>%1()</i> function.").arg(QLatin1String(function));
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
    setId("Z.Debugger.LocalsAndExpressions");
    //: '&&' will appear as one (one is marking keyboard shortcut)
    setDisplayName(QCoreApplication::translate("Debugger", "Locals && Expressions"));
    setCategory(DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger", DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Icon(DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

void LocalsAndExpressionsOptionsPage::apply()
{
    m_group.apply(ICore::settings());
}

void LocalsAndExpressionsOptionsPage::finish()
{
    m_group.finish();
    delete m_widget;
}

QWidget *LocalsAndExpressionsOptionsPage::widget()
{
    if (!m_widget) {
        m_widget = new QWidget;

        auto debuggingHelperGroupBox = new QGroupBox(m_widget);
        debuggingHelperGroupBox->setTitle(tr("Use Debugging Helper"));
        debuggingHelperGroupBox->setCheckable(true);

        auto label = new QLabel(debuggingHelperGroupBox);
        label->setTextFormat(Qt::AutoText);
        label->setWordWrap(true);
        label->setText(QLatin1String("<html><head/><body>\n<p>")
           + tr("The debugging helpers are used to produce a nice "
                "display of objects of certain types like QString or "
                "std::map in the &quot;Locals and Expressions&quot; view.")
            + QLatin1String("</p></body></html>"));

        auto groupBoxCustomDumperCommands = new QGroupBox(debuggingHelperGroupBox);
        groupBoxCustomDumperCommands->setTitle(tr("Debugging Helper Customization"));
        groupBoxCustomDumperCommands->setToolTip(tr(
            "<html><head/><body><p>Python commands entered here will be executed after built-in "
            "debugging helpers have been loaded and fully initialized. You can load additional "
            "debugging helpers or modify existing ones here.</p></body></html>"));

        auto textEditCustomDumperCommands = new QTextEdit(groupBoxCustomDumperCommands);
        textEditCustomDumperCommands->setAcceptRichText(false);
        textEditCustomDumperCommands->setToolTip(groupBoxCustomDumperCommands->toolTip());

        auto groupBoxExtraDumperFile = new QGroupBox(debuggingHelperGroupBox);
        groupBoxExtraDumperFile->setTitle(tr("Extra Debugging Helpers"));
        groupBoxExtraDumperFile->setToolTip(tr(
            "Path to a Python file containing additional data dumpers."));

        auto pathChooserExtraDumperFile = new Utils::PathChooser(groupBoxExtraDumperFile);
        pathChooserExtraDumperFile->setExpectedKind(Utils::PathChooser::File);

        auto checkBoxUseCodeModel = new QCheckBox(debuggingHelperGroupBox);
        auto checkBoxShowThreadNames = new QCheckBox(debuggingHelperGroupBox);
        auto checkBoxShowStdNamespace = new QCheckBox(m_widget);
        auto checkBoxShowQtNamespace = new QCheckBox(m_widget);
        auto checkBoxShowQObjectNames = new QCheckBox(m_widget);

        auto spinBoxMaximalStringLength = new QSpinBox(m_widget);
        spinBoxMaximalStringLength->setSpecialValueText(tr("<unlimited>"));
        spinBoxMaximalStringLength->setMaximum(10000000);
        spinBoxMaximalStringLength->setSingleStep(1000);
        spinBoxMaximalStringLength->setValue(10000);

        auto spinBoxDisplayStringLimit = new QSpinBox(m_widget);
        spinBoxDisplayStringLimit->setSpecialValueText(tr("<unlimited>"));
        spinBoxDisplayStringLimit->setMaximum(10000);
        spinBoxDisplayStringLimit->setSingleStep(10);
        spinBoxDisplayStringLimit->setValue(100);

        auto chooser = new VariableChooser(m_widget);
        chooser->addSupportedWidget(textEditCustomDumperCommands);
        chooser->addSupportedWidget(pathChooserExtraDumperFile->lineEdit());

        auto gridLayout = new QGridLayout(debuggingHelperGroupBox);
        gridLayout->addWidget(label, 0, 0, 1, 1);
        gridLayout->addWidget(checkBoxUseCodeModel, 1, 0, 1, 1);
        gridLayout->addWidget(checkBoxShowThreadNames, 2, 0, 1, 1);
        gridLayout->addWidget(groupBoxExtraDumperFile, 3, 0, 1, 1);
        gridLayout->addWidget(groupBoxCustomDumperCommands, 0, 1, 4, 1);

        auto layout1 = new QFormLayout;
        layout1->addItem(new QSpacerItem(10, 10));
        layout1->addRow(checkBoxShowStdNamespace);
        layout1->addRow(checkBoxShowQtNamespace);
        layout1->addRow(checkBoxShowQObjectNames);
        layout1->addItem(new QSpacerItem(10, 10));
        layout1->addRow(tr("Maximum string length:"), spinBoxMaximalStringLength);
        layout1->addRow(tr("Display string length:"), spinBoxDisplayStringLimit);

        auto lowerLayout = new QHBoxLayout;
        lowerLayout->addLayout(layout1);
        lowerLayout->addStretch();

        auto layout = new QVBoxLayout(m_widget);
        layout->addWidget(debuggingHelperGroupBox);
        layout->addLayout(lowerLayout);
        layout->addStretch();

        auto customDumperLayout = new QGridLayout(groupBoxCustomDumperCommands);
        customDumperLayout->addWidget(textEditCustomDumperCommands, 0, 0, 1, 1);

        auto extraDumperLayout = new QGridLayout(groupBoxExtraDumperFile);
        extraDumperLayout->addWidget(pathChooserExtraDumperFile, 0, 0, 1, 1);

        m_group.clear();
        m_group.insert(action(UseDebuggingHelpers), debuggingHelperGroupBox);
        m_group.insert(action(ExtraDumperFile), pathChooserExtraDumperFile);
        m_group.insert(action(ExtraDumperCommands), textEditCustomDumperCommands);
        m_group.insert(action(UseCodeModel), checkBoxUseCodeModel);
        m_group.insert(action(ShowThreadNames), checkBoxShowThreadNames);
        m_group.insert(action(ShowStdNamespace), checkBoxShowStdNamespace);
        m_group.insert(action(ShowQtNamespace), checkBoxShowQtNamespace);
        m_group.insert(action(ShowQObjectNames), checkBoxShowQObjectNames);
        m_group.insert(action(DisplayStringLimit), spinBoxDisplayStringLimit);
        m_group.insert(action(MaximalStringLength), spinBoxMaximalStringLength);
    }
    return m_widget;
}

} // namespace Internal
} // namespace Debugger
