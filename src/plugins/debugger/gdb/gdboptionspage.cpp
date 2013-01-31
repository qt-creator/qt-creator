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

#include "gdboptionspage.h"
#include "commonoptionspage.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerinternalconstants.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <utils/savedaction.h>

#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>

#include <QCheckBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextEdit>

namespace Debugger {
namespace Internal {

class GdbOptionsPageWidget : public QWidget
{
public:
    explicit GdbOptionsPageWidget(QWidget *parent);

    QGroupBox *groupBoxGeneral;
    QLabel *labelGdbWatchdogTimeout;
    QSpinBox *spinBoxGdbWatchdogTimeout;
    QCheckBox *checkBoxSkipKnownFrames;
    QCheckBox *checkBoxUseMessageBoxForSignals;
    QCheckBox *checkBoxAdjustBreakpointLocations;
    QCheckBox *checkBoxUseDynamicType;
    QCheckBox *checkBoxLoadGdbInit;
    QCheckBox *checkBoxWarnOnReleaseBuilds;
    QLabel *labelDangerous;
    QCheckBox *checkBoxTargetAsync;
    QCheckBox *checkBoxAutoEnrichParameters;
    QCheckBox *checkBoxBreakOnWarning;
    QCheckBox *checkBoxBreakOnFatal;
    QCheckBox *checkBoxBreakOnAbort;
    QCheckBox *checkBoxEnableReverseDebugging;
    QCheckBox *checkBoxAttemptQuickStart;
    QCheckBox *checkBoxMultiInferior;
    QCheckBox *checkBoxIntelFlavor;

    QGroupBox *groupBoxStartupCommands;
    QTextEdit *textEditStartupCommands;
    QGroupBox *groupBoxPostAttachCommands;
    QTextEdit *textEditPostAttachCommands;

    //QGroupBox *groupBoxPluginDebugging;
    //QRadioButton *radioButtonAllPluginBreakpoints;
    //QRadioButton *radioButtonSelectedPluginBreakpoints;
    //QRadioButton *radioButtonNoPluginBreakpoints;
    //QLabel *labelSelectedPluginBreakpoints;
    //QLineEdit *lineEditSelectedPluginBreakpointsPattern;

    Utils::SavedActionSet group;
    QString searchKeywords;
};

GdbOptionsPageWidget::GdbOptionsPageWidget(QWidget *parent)
    : QWidget(parent)
{
    groupBoxGeneral = new QGroupBox(this);
    groupBoxGeneral->setTitle(GdbOptionsPage::tr("General"));

    labelGdbWatchdogTimeout = new QLabel(groupBoxGeneral);
    labelGdbWatchdogTimeout->setText(GdbOptionsPage::tr("GDB timeout:"));
    labelGdbWatchdogTimeout->setToolTip(GdbOptionsPage::tr(
        "The number of seconds Qt Creator will wait before it terminates\n"
        "a non-responsive GDB process. The default value of 20 seconds should\n"
        "be sufficient for most applications, but there are situations when\n"
        "loading big libraries or listing source files takes much longer than\n"
        "that on slow machines. In this case, the value should be increased."));

    spinBoxGdbWatchdogTimeout = new QSpinBox(groupBoxGeneral);
    spinBoxGdbWatchdogTimeout->setToolTip(labelGdbWatchdogTimeout->toolTip());
    spinBoxGdbWatchdogTimeout->setSuffix(GdbOptionsPage::tr("sec"));
    spinBoxGdbWatchdogTimeout->setLayoutDirection(Qt::LeftToRight);
    spinBoxGdbWatchdogTimeout->setMinimum(20);
    spinBoxGdbWatchdogTimeout->setMaximum(1000000);
    spinBoxGdbWatchdogTimeout->setSingleStep(20);
    spinBoxGdbWatchdogTimeout->setValue(20);

    checkBoxSkipKnownFrames = new QCheckBox(groupBoxGeneral);
    checkBoxSkipKnownFrames->setText(GdbOptionsPage::tr("Skip known frames when stepping"));
    checkBoxSkipKnownFrames->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body><p>"
        "Allows <i>Step Into</i> to compress several steps into one step\n"
        "for less noisy debugging. For example, the atomic reference\n"
        "counting code is skipped, and a single <i>Step Into</i> for a signal\n"
        "emission ends up directly in the slot connected to it."));

    checkBoxUseMessageBoxForSignals = new QCheckBox(groupBoxGeneral);
    checkBoxUseMessageBoxForSignals->setText(GdbOptionsPage::tr(
        "Show a message box when receiving a signal"));
    checkBoxUseMessageBoxForSignals->setToolTip(GdbOptionsPage::tr(
        "Displays a message box as soon as your application\n"
        "receives a signal like SIGSEGV during debugging."));

    checkBoxAdjustBreakpointLocations = new QCheckBox(groupBoxGeneral);
    checkBoxAdjustBreakpointLocations->setText(GdbOptionsPage::tr(
        "Adjust breakpoint locations"));
    checkBoxAdjustBreakpointLocations->setToolTip(GdbOptionsPage::tr(
        "GDB allows setting breakpoints on source lines for which no code \n"
        "was generated. In such situations the breakpoint is shifted to the\n"
        "next source code line for which code was actually generated.\n"
        "This option reflects such temporary change by moving the breakpoint\n"
        "markers in the source code editor."));

    checkBoxUseDynamicType = new QCheckBox(groupBoxGeneral);
    checkBoxUseDynamicType->setText(GdbOptionsPage::tr(
        "Use dynamic object type for display"));
    checkBoxUseDynamicType->setToolTip(GdbOptionsPage::tr(
        "Specifies whether the dynamic or the static type of objects will be "
        "displayed. Choosing the dynamic type might be slower."));

    checkBoxLoadGdbInit = new QCheckBox(groupBoxGeneral);
    checkBoxLoadGdbInit->setText(GdbOptionsPage::tr("Load .gdbinit file on startup"));
    checkBoxLoadGdbInit->setToolTip(GdbOptionsPage::tr(
        "Allows or inhibits reading the user's default\n"
        ".gdbinit file on debugger startup."));

    checkBoxWarnOnReleaseBuilds = new QCheckBox(groupBoxGeneral);
    checkBoxWarnOnReleaseBuilds->setText(GdbOptionsPage::tr(
        "Warn when debugging \"Release\" builds"));
    checkBoxWarnOnReleaseBuilds->setToolTip(GdbOptionsPage::tr(
        "Show a warning when starting the debugger "
        "on a binary with insufficient debug information."));

    labelDangerous = new QLabel(GdbOptionsPage::tr(
        "The options below should be used with care."));
    labelDangerous->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>The options below give access to advanced "
        "or experimental functions of GDB. Enabling them may negatively "
        "impact your debugging experience.</body></html>"));
    QFont f = labelDangerous->font();
    f.setItalic(true);
    labelDangerous->setFont(f);

    checkBoxTargetAsync = new QCheckBox(groupBoxGeneral);
    checkBoxTargetAsync->setText(GdbOptionsPage::tr(
        "Use asynchronous mode to control the inferior"));

    checkBoxAutoEnrichParameters = new QCheckBox(groupBoxGeneral);
    checkBoxAutoEnrichParameters->setText(GdbOptionsPage::tr(
        "Use common locations for debug information"));
    checkBoxAutoEnrichParameters->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>Add common paths to locations "
        "of debug information such as <i>/usr/src/debug</i> "
        "when starting GDB.</body></html>"));

    // #fixme: 2.7 Move to common settings page.
    checkBoxBreakOnWarning = new QCheckBox(groupBoxGeneral);
    checkBoxBreakOnWarning->setText(CommonOptionsPage::msgSetBreakpointAtFunction("qWarning"));
    checkBoxBreakOnWarning->setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip("qWarning"));

    checkBoxBreakOnFatal = new QCheckBox(groupBoxGeneral);
    checkBoxBreakOnFatal->setText(CommonOptionsPage::msgSetBreakpointAtFunction("qFatal"));
    checkBoxBreakOnFatal->setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip("qFatal"));

    checkBoxBreakOnAbort = new QCheckBox(groupBoxGeneral);
    checkBoxBreakOnAbort->setText(CommonOptionsPage::msgSetBreakpointAtFunction("abort"));
    checkBoxBreakOnAbort->setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip("abort"));

    checkBoxEnableReverseDebugging = new QCheckBox(groupBoxGeneral);
    checkBoxEnableReverseDebugging->setText(GdbOptionsPage::tr("Enable reverse debugging"));
    checkBoxEnableReverseDebugging->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body><p>Enable stepping backwards.</p><p>"
        "<b>Note:</b> This feature is very slow and unstable on the GDB side. "
        "It exhibits unpredictable behavior when going backwards over system "
        "calls and is very likely to destroy your debugging session.</p></body></html>"));

    checkBoxAttemptQuickStart = new QCheckBox(groupBoxGeneral);
    checkBoxAttemptQuickStart->setText(GdbOptionsPage::tr("Attempt quick start"));
    checkBoxAttemptQuickStart->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>Postpone reading debug information as long as possible. "
        "This can result in faster startup times at the price of not being able to "
        "set breakpoints by file and number.</body></html>"));

    checkBoxMultiInferior = new QCheckBox(groupBoxGeneral);
    checkBoxMultiInferior->setText(GdbOptionsPage::tr("Debug all children"));
    checkBoxMultiInferior->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>Keep debugging all children after a fork."
        "</body></html>"));

    checkBoxIntelFlavor = new QCheckBox(groupBoxGeneral);
    checkBoxIntelFlavor->setText(GdbOptionsPage::tr("Use Intel style disassembly"));
    checkBoxIntelFlavor->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>GDB shows by default AT&&T style disassembly."
        "</body></html>"));

    groupBoxStartupCommands = new QGroupBox(this);
    groupBoxStartupCommands->setTitle(GdbOptionsPage::tr("Additional Startup Commands"));
    groupBoxStartupCommands->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body><p>GDB commands entered here will be executed after "
        "GDB has been started and the debugging helpers have been initialized.</p>"
        "<p>You can add commands to load further debugging helpers here, or "
        "modify existing ones.</p>"
        "<p>To execute simple Python commands, prefix them with \"python\".</p>"
        "<p>To execute sequences of Python commands spanning multiple lines "
        "prepend the block with \"python\" on a separate line, and append "
        "\"end\" on a separate line.</p>"
        "<p>To execute arbitrary Python scripts, "
        "use <i>python execfile('/path/to/script.py')</i>.</p>"
        "</body></html>"));

    textEditStartupCommands = new QTextEdit(groupBoxStartupCommands);
    textEditStartupCommands->setAcceptRichText(false);
    textEditStartupCommands->setToolTip(groupBoxStartupCommands->toolTip());

    groupBoxPostAttachCommands = new QGroupBox(this);
    groupBoxPostAttachCommands->setTitle(GdbOptionsPage::tr("Additional Attach Commands"));
    groupBoxPostAttachCommands->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body><p>GDB commands entered here will be executed after "
        "GDB has successfully attached to remote targets.</p>"
        "<p>You can add commands to further set up the target here, "
        "such as \"monitor reset\" or \"load\"."
        "</body></html>"));

    textEditPostAttachCommands = new QTextEdit(groupBoxPostAttachCommands);
    textEditPostAttachCommands->setAcceptRichText(false);
    textEditPostAttachCommands->setToolTip(groupBoxPostAttachCommands->toolTip());

    /*
    groupBoxPluginDebugging = new QGroupBox(q);
    groupBoxPluginDebugging->setTitle(GdbOptionsPage::tr(
        "Behavior of Breakpoint Setting in Plugins"));

    radioButtonAllPluginBreakpoints = new QRadioButton(groupBoxPluginDebugging);
    radioButtonAllPluginBreakpoints->setText(GdbOptionsPage::tr(
        "Always try to set breakpoints in plugins automatically"));
    radioButtonAllPluginBreakpoints->setToolTip(GdbOptionsPage::tr(
        "This is the slowest but safest option."));

    radioButtonSelectedPluginBreakpoints = new QRadioButton(groupBoxPluginDebugging);
    radioButtonSelectedPluginBreakpoints->setText(GdbOptionsPage::tr(
        "Try to set breakpoints in selected plugins"));

    radioButtonNoPluginBreakpoints = new QRadioButton(groupBoxPluginDebugging);
    radioButtonNoPluginBreakpoints->setText(GdbOptionsPage::tr(
        "Never set breakpoints in plugins automatically"));

    lineEditSelectedPluginBreakpointsPattern = new QLineEdit(groupBoxPluginDebugging);

    labelSelectedPluginBreakpoints = new QLabel(groupBoxPluginDebugging);
    labelSelectedPluginBreakpoints->setText(GdbOptionsPage::tr(
        "Matching regular expression: "));
    */

    QFormLayout *formLayout = new QFormLayout(groupBoxGeneral);
    formLayout->addRow(labelGdbWatchdogTimeout, spinBoxGdbWatchdogTimeout);
    formLayout->addRow(checkBoxSkipKnownFrames);
    formLayout->addRow(checkBoxUseMessageBoxForSignals);
    formLayout->addRow(checkBoxAdjustBreakpointLocations);
    formLayout->addRow(checkBoxUseDynamicType);
    formLayout->addRow(checkBoxLoadGdbInit);
    formLayout->addRow(checkBoxWarnOnReleaseBuilds);
    formLayout->addRow(checkBoxIntelFlavor);
    formLayout->addRow(labelDangerous);
    formLayout->addRow(checkBoxTargetAsync);
    formLayout->addRow(checkBoxAutoEnrichParameters);
    formLayout->addRow(checkBoxBreakOnWarning);
    formLayout->addRow(checkBoxBreakOnFatal);
    formLayout->addRow(checkBoxBreakOnAbort);
    formLayout->addRow(checkBoxEnableReverseDebugging);
    formLayout->addRow(checkBoxAttemptQuickStart);
    formLayout->addRow(checkBoxMultiInferior);

    QGridLayout *startLayout = new QGridLayout(groupBoxStartupCommands);
    startLayout->addWidget(textEditStartupCommands, 0, 0, 1, 1);

    QGridLayout *postAttachLayout = new QGridLayout(groupBoxPostAttachCommands);
    postAttachLayout->addWidget(textEditPostAttachCommands, 0, 0, 1, 1);

    //QHBoxLayout *horizontalLayout = new QHBoxLayout();
    //horizontalLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Preferred, QSizePolicy::Minimum));
    //horizontalLayout->addWidget(labelSelectedPluginBreakpoints);
    //horizontalLayout->addWidget(lineEditSelectedPluginBreakpointsPattern);

    QGridLayout *gridLayout = new QGridLayout(this);
    gridLayout->addWidget(groupBoxGeneral, 0, 0, 2, 1);
    gridLayout->addWidget(groupBoxStartupCommands, 0, 1, 1, 1);
    gridLayout->addWidget(groupBoxPostAttachCommands, 1, 1, 1, 1);

    //gridLayout->addWidget(groupBoxStartupCommands, 0, 1, 1, 1);
    //gridLayout->addWidget(radioButtonAllPluginBreakpoints, 0, 0, 1, 1);
    //gridLayout->addWidget(radioButtonSelectedPluginBreakpoints, 1, 0, 1, 1);

    //gridLayout->addLayout(horizontalLayout, 2, 0, 1, 1);
    //gridLayout->addWidget(radioButtonNoPluginBreakpoints, 3, 0, 1, 1);
    //gridLayout->addWidget(groupBoxPluginDebugging, 1, 0, 1, 2);

    DebuggerCore *dc = debuggerCore();
    group.insert(dc->action(GdbStartupCommands), textEditStartupCommands);
    group.insert(dc->action(GdbPostAttachCommands), textEditPostAttachCommands);
    group.insert(dc->action(LoadGdbInit), checkBoxLoadGdbInit);
    group.insert(dc->action(AutoEnrichParameters), checkBoxAutoEnrichParameters);
    group.insert(dc->action(UseDynamicType), checkBoxUseDynamicType);
    group.insert(dc->action(TargetAsync), checkBoxTargetAsync);
    group.insert(dc->action(WarnOnReleaseBuilds), checkBoxWarnOnReleaseBuilds);
    group.insert(dc->action(AdjustBreakpointLocations), checkBoxAdjustBreakpointLocations);
    group.insert(dc->action(BreakOnWarning), checkBoxBreakOnWarning);
    group.insert(dc->action(BreakOnFatal), checkBoxBreakOnFatal);
    group.insert(dc->action(BreakOnAbort), checkBoxBreakOnAbort);
    group.insert(dc->action(GdbWatchdogTimeout), spinBoxGdbWatchdogTimeout);
    group.insert(dc->action(AttemptQuickStart), checkBoxAttemptQuickStart);
    group.insert(dc->action(MultiInferior), checkBoxMultiInferior);
    group.insert(dc->action(IntelFlavor), checkBoxIntelFlavor);

    group.insert(dc->action(UseMessageBoxForSignals), checkBoxUseMessageBoxForSignals);
    group.insert(dc->action(SkipKnownFrames), checkBoxSkipKnownFrames);
    group.insert(dc->action(EnableReverseDebugging), checkBoxEnableReverseDebugging);
    group.insert(dc->action(GdbWatchdogTimeout), 0);

    //lineEditSelectedPluginBreakpointsPattern->
    //    setEnabled(dc->action(SelectedPluginBreakpoints)->value().toBool());
    //connect(radioButtonSelectedPluginBreakpoints, SIGNAL(toggled(bool)),
    //    lineEditSelectedPluginBreakpointsPattern, SLOT(setEnabled(bool)));

    const QLatin1Char sep(' ');
    QTextStream(&searchKeywords)
            << sep << groupBoxGeneral->title()
            << sep << checkBoxLoadGdbInit->text()
            << sep << checkBoxTargetAsync->text()
            << sep << checkBoxWarnOnReleaseBuilds->text()
            << sep << checkBoxUseDynamicType->text()
            << sep << labelGdbWatchdogTimeout->text()
            << sep << checkBoxEnableReverseDebugging->text()
            << sep << checkBoxSkipKnownFrames->text()
            << sep << checkBoxUseMessageBoxForSignals->text()
            << sep << checkBoxAdjustBreakpointLocations->text()
            << sep << checkBoxAttemptQuickStart->text()
            << sep << checkBoxMultiInferior->text()
    //        << sep << groupBoxPluginDebugging->title()
    //        << sep << radioButtonAllPluginBreakpoints->text()
    //        << sep << radioButtonSelectedPluginBreakpoints->text()
    //        << sep << labelSelectedPluginBreakpoints->text()
    //        << sep << radioButtonNoPluginBreakpoints->text()
    ;
    searchKeywords.remove(QLatin1Char('&'));
}

GdbOptionsPage::GdbOptionsPage()
{
    setId("M.Gdb");
    setDisplayName(tr("GDB"));
    setCategory(Constants::DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger", Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

GdbOptionsPage::~GdbOptionsPage()
{
}

QWidget *GdbOptionsPage::createPage(QWidget *parent)
{
    m_widget = new GdbOptionsPageWidget(parent);
    return m_widget;
}

void GdbOptionsPage::apply()
{
    if (m_widget)
        m_widget->group.apply(Core::ICore::settings());
}

void GdbOptionsPage::finish()
{
    if (m_widget)
        m_widget->group.finish();
}

bool GdbOptionsPage::matches(const QString &s) const
{
    return m_widget && m_widget->searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace Debugger
