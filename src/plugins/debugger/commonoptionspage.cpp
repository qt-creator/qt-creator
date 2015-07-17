/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "commonoptionspage.h"

#include "debuggeractions.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"

#include <coreplugin/icore.h>
#include <utils/hostosinfo.h>
#include <utils/savedaction.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QTextStream>

using namespace Core;
using namespace Debugger::Constants;
using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

class CommonOptionsPageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommonOptionsPageWidget(const QSharedPointer<Utils::SavedActionSet> &group);

    GlobalDebuggerOptions globalOptions() const;
    void setGlobalOptions(const GlobalDebuggerOptions &go);

private:
    QCheckBox *checkBoxUseAlternatingRowColors;
    QCheckBox *checkBoxFontSizeFollowsEditor;
    QCheckBox *checkBoxUseToolTipsInMainEditor;
    QCheckBox *checkBoxListSourceFiles;
    QCheckBox *checkBoxCloseSourceBuffersOnExit;
    QCheckBox *checkBoxCloseMemoryBuffersOnExit;
    QCheckBox *checkBoxSwitchModeOnExit;
    QCheckBox *checkBoxBringToForegroundOnInterrrupt;
    QCheckBox *checkBoxShowQmlObjectTree;
    QCheckBox *checkBoxBreakpointsFullPath;
    QCheckBox *checkBoxRegisterForPostMortem;
    QCheckBox *checkBoxWarnOnReleaseBuilds;
    QCheckBox *checkBoxKeepEditorStationaryWhileStepping;
    QLabel *labelMaximalStackDepth;
    QSpinBox *spinBoxMaximalStackDepth;

    DebuggerSourcePathMappingWidget *sourcesMappingWidget;
    const QSharedPointer<Utils::SavedActionSet> m_group;
};

CommonOptionsPageWidget::CommonOptionsPageWidget
    (const QSharedPointer<Utils::SavedActionSet> &group)
  : m_group(group)
{
    QGroupBox *behaviorBox = new QGroupBox(this);
    behaviorBox->setTitle(tr("Behavior"));

    checkBoxUseAlternatingRowColors = new QCheckBox(behaviorBox);
    checkBoxUseAlternatingRowColors->setText(tr("Use alternating row colors in debug views"));

    checkBoxFontSizeFollowsEditor = new QCheckBox(behaviorBox);
    checkBoxFontSizeFollowsEditor->setToolTip(tr("Changes the font size in the debugger views when the font size in the main editor changes."));
    checkBoxFontSizeFollowsEditor->setText(tr("Debugger font size follows main editor"));

    checkBoxUseToolTipsInMainEditor = new QCheckBox(behaviorBox);
    checkBoxUseToolTipsInMainEditor->setText(tr("Use tooltips in main editor while debugging"));

    checkBoxListSourceFiles = new QCheckBox(behaviorBox);
    checkBoxListSourceFiles->setToolTip(tr("Populates the source file view automatically. This might slow down debugger startup considerably."));
    checkBoxListSourceFiles->setText(tr("Populate source file view automatically"));

    QString t = tr("Stopping and stepping in the debugger "
        "will automatically open views associated with the current location.") + QLatin1Char('\n');
    checkBoxCloseSourceBuffersOnExit = new QCheckBox(behaviorBox);
    checkBoxCloseSourceBuffersOnExit->setText(tr("Close temporary source views on debugger exit"));
    checkBoxCloseSourceBuffersOnExit->setToolTip(t + tr("Select this option to close "
            "automatically opened source views when the debugger exits."));

    checkBoxCloseMemoryBuffersOnExit = new QCheckBox(behaviorBox);
    checkBoxCloseMemoryBuffersOnExit->setText(tr("Close temporary memory views on debugger exit"));
    checkBoxCloseMemoryBuffersOnExit->setToolTip(t + tr("Select this option to close "
             "automatically opened memory views when the debugger exits."));

    checkBoxSwitchModeOnExit = new QCheckBox(behaviorBox);
    checkBoxSwitchModeOnExit->setText(tr("Switch to previous mode on debugger exit"));

    checkBoxBringToForegroundOnInterrrupt = new QCheckBox(behaviorBox);
    checkBoxBringToForegroundOnInterrrupt->setText(tr("Bring Qt Creator to foreground when application interrupts"));

    checkBoxShowQmlObjectTree = new QCheckBox(behaviorBox);
    checkBoxShowQmlObjectTree->setToolTip(tr("Shows QML object tree in Locals and Expressions when connected and not stepping."));
    checkBoxShowQmlObjectTree->setText(tr("Show QML object tree"));

    checkBoxBreakpointsFullPath = new QCheckBox(behaviorBox);
    checkBoxBreakpointsFullPath->setToolTip(tr("Enables a full file path in breakpoints by default also for GDB."));
    checkBoxBreakpointsFullPath->setText(tr("Set breakpoints using a full absolute path"));

    checkBoxRegisterForPostMortem = new QCheckBox(behaviorBox);
    checkBoxRegisterForPostMortem->setToolTip(tr("Registers Qt Creator for debugging crashed applications."));
    checkBoxRegisterForPostMortem->setText(tr("Use Qt Creator for post-mortem debugging"));

    checkBoxWarnOnReleaseBuilds = new QCheckBox(behaviorBox);
    checkBoxWarnOnReleaseBuilds->setText(tr("Warn when debugging \"Release\" builds"));
    checkBoxWarnOnReleaseBuilds->setToolTip(tr("Shows a warning when starting the debugger "
                                            "on a binary with insufficient debug information."));

    checkBoxKeepEditorStationaryWhileStepping = new QCheckBox(behaviorBox);
    checkBoxKeepEditorStationaryWhileStepping->setText(tr("Keep editor stationary when stepping"));
    checkBoxKeepEditorStationaryWhileStepping->setToolTip(tr("Scrolls the editor only when it is necessary "
                                                             "to keep the current line in view, "
                                                             "instead of keeping the next statement centered at "
                                                             "all times."));

    labelMaximalStackDepth = new QLabel(tr("Maximum stack depth:"), behaviorBox);

    spinBoxMaximalStackDepth = new QSpinBox(behaviorBox);
    spinBoxMaximalStackDepth->setSpecialValueText(tr("<unlimited>"));
    spinBoxMaximalStackDepth->setMaximum(999);
    spinBoxMaximalStackDepth->setSingleStep(5);
    spinBoxMaximalStackDepth->setValue(10);

    sourcesMappingWidget = new DebuggerSourcePathMappingWidget(this);

    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(labelMaximalStackDepth);
    horizontalLayout->addWidget(spinBoxMaximalStackDepth);
    horizontalLayout->addStretch();

    QGridLayout *gridLayout = new QGridLayout(behaviorBox);
    gridLayout->addWidget(checkBoxUseAlternatingRowColors, 0, 0, 1, 1);
    gridLayout->addWidget(checkBoxUseToolTipsInMainEditor, 1, 0, 1, 1);
    gridLayout->addWidget(checkBoxCloseSourceBuffersOnExit, 2, 0, 1, 1);
    gridLayout->addWidget(checkBoxCloseMemoryBuffersOnExit, 3, 0, 1, 1);
    gridLayout->addWidget(checkBoxBringToForegroundOnInterrrupt, 4, 0, 1, 1);
    gridLayout->addWidget(checkBoxBreakpointsFullPath, 5, 0, 1, 1);
    gridLayout->addWidget(checkBoxWarnOnReleaseBuilds, 6, 0, 1, 1);
    gridLayout->addLayout(horizontalLayout, 7, 0, 1, 2);

    gridLayout->addWidget(checkBoxFontSizeFollowsEditor, 0, 1, 1, 1);
    gridLayout->addWidget(checkBoxListSourceFiles, 1, 1, 1, 1);
    gridLayout->addWidget(checkBoxSwitchModeOnExit, 2, 1, 1, 1);
    gridLayout->addWidget(checkBoxShowQmlObjectTree, 3, 1, 1, 1);
    gridLayout->addWidget(checkBoxKeepEditorStationaryWhileStepping, 4, 1, 1, 1);
    gridLayout->addWidget(checkBoxRegisterForPostMortem, 5, 1, 1, 1);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(behaviorBox);
    verticalLayout->addWidget(sourcesMappingWidget);
    verticalLayout->addStretch();

    m_group->clear();

    m_group->insert(action(ListSourceFiles),
        checkBoxListSourceFiles);
    m_group->insert(action(UseAlternatingRowColors),
        checkBoxUseAlternatingRowColors);
    m_group->insert(action(UseToolTipsInMainEditor),
        checkBoxUseToolTipsInMainEditor);
    m_group->insert(action(CloseSourceBuffersOnExit),
        checkBoxCloseSourceBuffersOnExit);
    m_group->insert(action(CloseMemoryBuffersOnExit),
        checkBoxCloseMemoryBuffersOnExit);
    m_group->insert(action(SwitchModeOnExit),
        checkBoxSwitchModeOnExit);
    m_group->insert(action(BreakpointsFullPathByDefault),
        checkBoxBreakpointsFullPath);
    m_group->insert(action(RaiseOnInterrupt),
        checkBoxBringToForegroundOnInterrrupt);
    m_group->insert(action(ShowQmlObjectTree),
        checkBoxShowQmlObjectTree);
    m_group->insert(action(WarnOnReleaseBuilds),
        checkBoxWarnOnReleaseBuilds);
    m_group->insert(action(StationaryEditorWhileStepping),
        checkBoxKeepEditorStationaryWhileStepping);
    m_group->insert(action(FontSizeFollowsEditor),
        checkBoxFontSizeFollowsEditor);
    m_group->insert(action(AutoDerefPointers), 0);
    m_group->insert(action(UseToolTipsInLocalsView), 0);
    m_group->insert(action(AlwaysAdjustColumnWidths), 0);
    m_group->insert(action(UseToolTipsInBreakpointsView), 0);
    m_group->insert(action(UseToolTipsInStackView), 0);
    m_group->insert(action(UseAddressInBreakpointsView), 0);
    m_group->insert(action(UseAddressInStackView), 0);
    m_group->insert(action(MaximalStackDepth), spinBoxMaximalStackDepth);
    m_group->insert(action(ShowStdNamespace), 0);
    m_group->insert(action(ShowQtNamespace), 0);
    m_group->insert(action(SortStructMembers), 0);
    m_group->insert(action(LogTimeStamps), 0);
    m_group->insert(action(VerboseLog), 0);
    m_group->insert(action(BreakOnThrow), 0);
    m_group->insert(action(BreakOnCatch), 0);
    if (Utils::HostOsInfo::isWindowsHost()) {
        Utils::SavedAction *registerAction = action(RegisterForPostMortem);
        m_group->insert(registerAction,
                checkBoxRegisterForPostMortem);
        connect(registerAction, &QAction::toggled,
                checkBoxRegisterForPostMortem, &QAbstractButton::setChecked);
    } else {
        checkBoxRegisterForPostMortem->setVisible(false);
    }
}

GlobalDebuggerOptions CommonOptionsPageWidget::globalOptions() const
{
    GlobalDebuggerOptions o;
    SourcePathMap allPathMap = sourcesMappingWidget->sourcePathMap();
    for (auto it = allPathMap.begin(), end = allPathMap.end(); it != end; ++it) {
        const QString key = it.key();
        if (key.startsWith(QLatin1Char('(')))
            o.sourcePathRegExpMap.append(qMakePair(QRegExp(key), it.value()));
        else
            o.sourcePathMap.insert(key, it.value());
    }
    return o;
}

void CommonOptionsPageWidget::setGlobalOptions(const GlobalDebuggerOptions &go)
{
    SourcePathMap allPathMap = go.sourcePathMap;
    foreach (auto regExpMap, go.sourcePathRegExpMap)
        allPathMap.insert(regExpMap.first.pattern(), regExpMap.second);

    sourcesMappingWidget->setSourcePathMap(allPathMap);
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

    m_group->apply(ICore::settings());

    const GlobalDebuggerOptions newGlobalOptions = m_widget->globalOptions();
    if (newGlobalOptions != *m_options) {
        *m_options = newGlobalOptions;
        m_options->toSettings();
    }
}

void CommonOptionsPage::finish()
{
    if (!m_group.isNull())
        m_group->finish();
    delete m_widget;
}

QWidget *CommonOptionsPage::widget()
{
    if (m_group.isNull())
        m_group = QSharedPointer<Utils::SavedActionSet>(new Utils::SavedActionSet);

    if (!m_widget) {
        m_widget = new CommonOptionsPageWidget(m_group);
        m_widget->setGlobalOptions(*m_options);
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
                "std::map in the &quot;Locals and Expressions&quot; view. ")
            + QLatin1String("</p></body></html>"));

        auto checkBoxUseCodeModel = new QCheckBox(debuggingHelperGroupBox);
        auto checkBoxShowThreadNames = new QCheckBox(debuggingHelperGroupBox);
        auto checkBoxShowStdNamespace = new QCheckBox(m_widget);
        auto checkBoxShowQtNamespace = new QCheckBox(m_widget);

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

        auto verticalLayout = new QVBoxLayout(debuggingHelperGroupBox);
        verticalLayout->addWidget(label);
        verticalLayout->addWidget(checkBoxUseCodeModel);
        verticalLayout->addWidget(checkBoxShowThreadNames);

        auto layout1 = new QFormLayout;
        layout1->addItem(new QSpacerItem(10, 10));
        layout1->addRow(checkBoxShowStdNamespace);
        layout1->addRow(checkBoxShowQtNamespace);
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

        m_group.clear();
        m_group.insert(action(UseDebuggingHelpers), debuggingHelperGroupBox);
        m_group.insert(action(UseCodeModel), checkBoxUseCodeModel);
        m_group.insert(action(ShowThreadNames), checkBoxShowThreadNames);
        m_group.insert(action(ShowStdNamespace), checkBoxShowStdNamespace);
        m_group.insert(action(ShowQtNamespace), checkBoxShowQtNamespace);
        m_group.insert(action(DisplayStringLimit), spinBoxDisplayStringLimit);
        m_group.insert(action(MaximalStringLength), spinBoxMaximalStringLength);

#ifndef QT_DEBUG
#if 0
        cmd = am->registerAction(m_dumpLogAction, DUMP_LOG, globalcontext);
        //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+D,Ctrl+L")));
        cmd->setDefaultKeySequence(QKeySequence(QCoreApplication::translate("Debugger", "Ctrl+Shift+F11")));
        mdebug->addAction(cmd);
#endif
#endif
    }
    return m_widget;
}

} // namespace Internal
} // namespace Debugger

#include "commonoptionspage.moc"
