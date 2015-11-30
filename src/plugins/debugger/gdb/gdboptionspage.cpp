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

#include <debugger/commonoptionspage.h>
#include <debugger/debuggeractions.h>
#include <debugger/debuggercore.h>
#include <debugger/debuggerinternalconstants.h>

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>

#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QDebug>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QSpinBox>
#include <QTextEdit>

using namespace Core;

namespace Debugger {
namespace Internal {

/////////////////////////////////////////////////////////////////////////
//
// GdbOptionsPageWidget - harmless options
//
/////////////////////////////////////////////////////////////////////////

class GdbOptionsPageWidget : public QWidget
{
    Q_OBJECT
public:
    GdbOptionsPageWidget();
    Utils::SavedActionSet group;
};

class GdbOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    GdbOptionsPage();

    QWidget *widget();
    void apply();
    void finish();

private:
    QPointer<GdbOptionsPageWidget> m_widget;
};

GdbOptionsPageWidget::GdbOptionsPageWidget()
{
    auto groupBoxGeneral = new QGroupBox(this);
    groupBoxGeneral->setTitle(GdbOptionsPage::tr("General"));

    auto labelGdbWatchdogTimeout = new QLabel(groupBoxGeneral);
    labelGdbWatchdogTimeout->setText(GdbOptionsPage::tr("GDB timeout:"));
    labelGdbWatchdogTimeout->setToolTip(GdbOptionsPage::tr(
        "The number of seconds Qt Creator will wait before it terminates\n"
        "a non-responsive GDB process. The default value of 20 seconds should\n"
        "be sufficient for most applications, but there are situations when\n"
        "loading big libraries or listing source files takes much longer than\n"
        "that on slow machines. In this case, the value should be increased."));

    auto spinBoxGdbWatchdogTimeout = new QSpinBox(groupBoxGeneral);
    spinBoxGdbWatchdogTimeout->setToolTip(labelGdbWatchdogTimeout->toolTip());
    spinBoxGdbWatchdogTimeout->setSuffix(GdbOptionsPage::tr("sec"));
    spinBoxGdbWatchdogTimeout->setLayoutDirection(Qt::LeftToRight);
    spinBoxGdbWatchdogTimeout->setMinimum(20);
    spinBoxGdbWatchdogTimeout->setMaximum(1000000);
    spinBoxGdbWatchdogTimeout->setSingleStep(20);
    spinBoxGdbWatchdogTimeout->setValue(20);

    auto checkBoxSkipKnownFrames = new QCheckBox(groupBoxGeneral);
    checkBoxSkipKnownFrames->setText(GdbOptionsPage::tr("Skip known frames when stepping"));
    checkBoxSkipKnownFrames->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body><p>"
        "Allows <i>Step Into</i> to compress several steps into one step\n"
        "for less noisy debugging. For example, the atomic reference\n"
        "counting code is skipped, and a single <i>Step Into</i> for a signal\n"
        "emission ends up directly in the slot connected to it."));

    auto checkBoxUseMessageBoxForSignals = new QCheckBox(groupBoxGeneral);
    checkBoxUseMessageBoxForSignals->setText(GdbOptionsPage::tr(
        "Show a message box when receiving a signal"));
    checkBoxUseMessageBoxForSignals->setToolTip(GdbOptionsPage::tr(
        "Displays a message box as soon as your application\n"
        "receives a signal like SIGSEGV during debugging."));

    auto checkBoxAdjustBreakpointLocations = new QCheckBox(groupBoxGeneral);
    checkBoxAdjustBreakpointLocations->setText(GdbOptionsPage::tr(
        "Adjust breakpoint locations"));
    checkBoxAdjustBreakpointLocations->setToolTip(GdbOptionsPage::tr(
        "GDB allows setting breakpoints on source lines for which no code \n"
        "was generated. In such situations the breakpoint is shifted to the\n"
        "next source code line for which code was actually generated.\n"
        "This option reflects such temporary change by moving the breakpoint\n"
        "markers in the source code editor."));

    auto checkBoxUseDynamicType = new QCheckBox(groupBoxGeneral);
    checkBoxUseDynamicType->setText(GdbOptionsPage::tr(
        "Use dynamic object type for display"));
    checkBoxUseDynamicType->setToolTip(GdbOptionsPage::tr(
        "Specifies whether the dynamic or the static type of objects will be "
        "displayed. Choosing the dynamic type might be slower."));

    auto checkBoxLoadGdbInit = new QCheckBox(groupBoxGeneral);
    checkBoxLoadGdbInit->setText(GdbOptionsPage::tr("Load .gdbinit file on startup"));
    checkBoxLoadGdbInit->setToolTip(GdbOptionsPage::tr(
        "Allows or inhibits reading the user's default\n"
        ".gdbinit file on debugger startup."));

    auto checkBoxLoadGdbDumpers = new QCheckBox(groupBoxGeneral);
    checkBoxLoadGdbDumpers->setText(GdbOptionsPage::tr("Load system GDB pretty printers"));
    checkBoxLoadGdbDumpers->setToolTip(GdbOptionsPage::tr(
        "Uses the default GDB pretty printers installed in your "
        "system or linked to the libraries your application uses."));

    auto checkBoxIntelFlavor = new QCheckBox(groupBoxGeneral);
    checkBoxIntelFlavor->setText(GdbOptionsPage::tr("Use Intel style disassembly"));
    checkBoxIntelFlavor->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>GDB shows by default AT&&T style disassembly."
        "</body></html>"));

    auto checkBoxIdentifyDebugInfoPackages = new QCheckBox(groupBoxGeneral);
    checkBoxIdentifyDebugInfoPackages->setText(GdbOptionsPage::tr("Create tasks from missing packages"));
    checkBoxIdentifyDebugInfoPackages->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body><p>Attempts to identify missing debug info packages "
        "and lists them in the Issues output pane.</p><p>"
        "<b>Note:</b> This feature needs special support from the Linux "
        "distribution and GDB build and is not available everywhere.</p></body></html>"));

    QString howToUsePython = GdbOptionsPage::tr(
        "<p>To execute simple Python commands, prefix them with \"python\".</p>"
        "<p>To execute sequences of Python commands spanning multiple lines "
        "prepend the block with \"python\" on a separate line, and append "
        "\"end\" on a separate line.</p>"
        "<p>To execute arbitrary Python scripts, "
        "use <i>python execfile('/path/to/script.py')</i>.</p>");

    auto groupBoxStartupCommands = new QGroupBox(this);
    groupBoxStartupCommands->setTitle(GdbOptionsPage::tr("Additional Startup Commands"));
    groupBoxStartupCommands->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body><p>GDB commands entered here will be executed after "
        "GDB has been started, but before the debugged program is started or "
        "attached, and before the debugging helpers are initialized.</p>%1"
        "</body></html>").arg(howToUsePython));

    auto textEditStartupCommands = new QTextEdit(groupBoxStartupCommands);
    textEditStartupCommands->setAcceptRichText(false);
    textEditStartupCommands->setToolTip(groupBoxStartupCommands->toolTip());

    auto groupBoxPostAttachCommands = new QGroupBox(this);
    groupBoxPostAttachCommands->setTitle(GdbOptionsPage::tr("Additional Attach Commands"));
    groupBoxPostAttachCommands->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body><p>GDB commands entered here will be executed after "
        "GDB has successfully attached to remote targets.</p>"
        "<p>You can add commands to further set up the target here, "
        "such as \"monitor reset\" or \"load\"."
        "</body></html>"));

    auto textEditPostAttachCommands = new QTextEdit(groupBoxPostAttachCommands);
    textEditPostAttachCommands->setAcceptRichText(false);
    textEditPostAttachCommands->setToolTip(groupBoxPostAttachCommands->toolTip());

    auto groupBoxCustomDumperCommands = new QGroupBox(this);
    groupBoxCustomDumperCommands->setTitle(GdbOptionsPage::tr("Debugging Helper Customization"));
    groupBoxCustomDumperCommands->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body><p>GDB commands entered here will be executed after "
        "Qt Creator's debugging helpers have been loaded and fully initialized. "
        "You can load additional debugging helpers or modify existing ones here.</p>"
        "%1</body></html>").arg(howToUsePython));

    auto textEditCustomDumperCommands = new QTextEdit(groupBoxCustomDumperCommands);
    textEditCustomDumperCommands->setAcceptRichText(false);
    textEditCustomDumperCommands->setToolTip(groupBoxCustomDumperCommands->toolTip());

    auto groupBoxExtraDumperFile = new QGroupBox(this);
    groupBoxExtraDumperFile->setTitle(GdbOptionsPage::tr("Extra Debugging Helpers"));
    groupBoxExtraDumperFile->setToolTip(GdbOptionsPage::tr(
        "Path to a Python file containing additional data dumpers."));

    auto pathChooserExtraDumperFile = new Utils::PathChooser(groupBoxExtraDumperFile);
    pathChooserExtraDumperFile->setExpectedKind(Utils::PathChooser::File);

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

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(textEditCustomDumperCommands);
    chooser->addSupportedWidget(textEditPostAttachCommands);
    chooser->addSupportedWidget(textEditStartupCommands);
    chooser->addSupportedWidget(pathChooserExtraDumperFile->lineEdit());

    auto formLayout = new QFormLayout(groupBoxGeneral);
    formLayout->addRow(labelGdbWatchdogTimeout, spinBoxGdbWatchdogTimeout);
    formLayout->addRow(checkBoxSkipKnownFrames);
    formLayout->addRow(checkBoxUseMessageBoxForSignals);
    formLayout->addRow(checkBoxAdjustBreakpointLocations);
    formLayout->addRow(checkBoxUseDynamicType);
    formLayout->addRow(checkBoxLoadGdbInit);
    formLayout->addRow(checkBoxLoadGdbDumpers);
    formLayout->addRow(checkBoxIntelFlavor);
    formLayout->addRow(checkBoxIdentifyDebugInfoPackages);

    auto startLayout = new QGridLayout(groupBoxStartupCommands);
    startLayout->addWidget(textEditStartupCommands, 0, 0, 1, 1);

    auto postAttachLayout = new QGridLayout(groupBoxPostAttachCommands);
    postAttachLayout->addWidget(textEditPostAttachCommands, 0, 0, 1, 1);

    auto customDumperLayout = new QGridLayout(groupBoxCustomDumperCommands);
    customDumperLayout->addWidget(textEditCustomDumperCommands, 0, 0, 1, 1);

    auto extraDumperLayout = new QGridLayout(groupBoxExtraDumperFile);
    extraDumperLayout->addWidget(pathChooserExtraDumperFile, 0, 0, 1, 1);

    auto gridLayout = new QGridLayout(this);
    gridLayout->addWidget(groupBoxGeneral, 0, 0, 5, 1);
    gridLayout->addWidget(groupBoxExtraDumperFile, 5, 0, 1, 1);

    gridLayout->addWidget(groupBoxStartupCommands, 0, 1, 2, 1);
    gridLayout->addWidget(groupBoxPostAttachCommands, 2, 1, 2, 1);
    gridLayout->addWidget(groupBoxCustomDumperCommands, 4, 1, 2, 1);

    group.insert(action(GdbStartupCommands), textEditStartupCommands);
    group.insert(action(ExtraDumperFile), pathChooserExtraDumperFile);
    group.insert(action(ExtraDumperCommands), textEditCustomDumperCommands);
    group.insert(action(GdbPostAttachCommands), textEditPostAttachCommands);
    group.insert(action(LoadGdbInit), checkBoxLoadGdbInit);
    group.insert(action(LoadGdbDumpers), checkBoxLoadGdbDumpers);
    group.insert(action(UseDynamicType), checkBoxUseDynamicType);
    group.insert(action(AdjustBreakpointLocations), checkBoxAdjustBreakpointLocations);
    group.insert(action(GdbWatchdogTimeout), spinBoxGdbWatchdogTimeout);
    group.insert(action(IntelFlavor), checkBoxIntelFlavor);
    group.insert(action(IdentifyDebugInfoPackages), checkBoxIdentifyDebugInfoPackages);
    group.insert(action(UseMessageBoxForSignals), checkBoxUseMessageBoxForSignals);
    group.insert(action(SkipKnownFrames), checkBoxSkipKnownFrames);

    //lineEditSelectedPluginBreakpointsPattern->
    //    setEnabled(action(SelectedPluginBreakpoints)->value().toBool());
    //connect(radioButtonSelectedPluginBreakpoints, SIGNAL(toggled(bool)),
    //    lineEditSelectedPluginBreakpointsPattern, SLOT(setEnabled(bool)));
}

GdbOptionsPage::GdbOptionsPage()
{
    setId("M.Gdb");
    setDisplayName(tr("GDB"));
    setCategory(Constants::DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger", Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

QWidget *GdbOptionsPage::widget()
{
    if (!m_widget)
        m_widget = new GdbOptionsPageWidget;
    return m_widget;
}

void GdbOptionsPage::apply()
{
    if (m_widget)
        m_widget->group.apply(ICore::settings());
}

void GdbOptionsPage::finish()
{
    if (m_widget) {
        m_widget->group.finish();
        delete m_widget;
    }
}

/////////////////////////////////////////////////////////////////////////
//
// GdbOptionsPageWidget2 - dangerous options
//
/////////////////////////////////////////////////////////////////////////

class GdbOptionsPageWidget2 : public QWidget
{
    Q_OBJECT
public:
    GdbOptionsPageWidget2();

    Utils::SavedActionSet group;
};

GdbOptionsPageWidget2::GdbOptionsPageWidget2()
{
    auto groupBoxDangerous = new QGroupBox(this);
    groupBoxDangerous->setTitle(GdbOptionsPage::tr("Extended"));

    auto labelDangerous = new QLabel(GdbOptionsPage::tr(
        "The options below should be used with care."));
    labelDangerous->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>The options below give access to advanced "
        "or experimental functions of GDB. Enabling them may negatively "
        "impact your debugging experience.</body></html>"));
    QFont f = labelDangerous->font();
    f.setItalic(true);
    labelDangerous->setFont(f);

    auto checkBoxTargetAsync = new QCheckBox(groupBoxDangerous);
    checkBoxTargetAsync->setText(GdbOptionsPage::tr(
        "Use asynchronous mode to control the inferior"));

    auto checkBoxAutoEnrichParameters = new QCheckBox(groupBoxDangerous);
    checkBoxAutoEnrichParameters->setText(GdbOptionsPage::tr(
        "Use common locations for debug information"));
    checkBoxAutoEnrichParameters->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>Adds common paths to locations "
        "of debug information such as <i>/usr/src/debug</i> "
        "when starting GDB.</body></html>"));

    // FIXME: Move to common settings page.
    auto checkBoxBreakOnWarning = new QCheckBox(groupBoxDangerous);
    checkBoxBreakOnWarning->setText(CommonOptionsPage::msgSetBreakpointAtFunction("qWarning"));
    checkBoxBreakOnWarning->setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip("qWarning"));

    auto checkBoxBreakOnFatal = new QCheckBox(groupBoxDangerous);
    checkBoxBreakOnFatal->setText(CommonOptionsPage::msgSetBreakpointAtFunction("qFatal"));
    checkBoxBreakOnFatal->setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip("qFatal"));

    auto checkBoxBreakOnAbort = new QCheckBox(groupBoxDangerous);
    checkBoxBreakOnAbort->setText(CommonOptionsPage::msgSetBreakpointAtFunction("abort"));
    checkBoxBreakOnAbort->setToolTip(CommonOptionsPage::msgSetBreakpointAtFunctionToolTip("abort"));

    QCheckBox *checkBoxEnableReverseDebugging = 0;
    if (isReverseDebuggingEnabled()) {
        checkBoxEnableReverseDebugging = new QCheckBox(groupBoxDangerous);
        checkBoxEnableReverseDebugging->setText(GdbOptionsPage::tr("Enable reverse debugging"));
        checkBoxEnableReverseDebugging->setToolTip(GdbOptionsPage::tr(
           "<html><head/><body><p>Enables stepping backwards.</p><p>"
           "<b>Note:</b> This feature is very slow and unstable on the GDB side. "
           "It exhibits unpredictable behavior when going backwards over system "
           "calls and is very likely to destroy your debugging session.</p></body></html>"));
    }

    auto checkBoxAttemptQuickStart = new QCheckBox(groupBoxDangerous);
    checkBoxAttemptQuickStart->setText(GdbOptionsPage::tr("Attempt quick start"));
    checkBoxAttemptQuickStart->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>Postpones reading debug information as long as possible. "
        "This can result in faster startup times at the price of not being able to "
        "set breakpoints by file and number.</body></html>"));

    auto checkBoxMultiInferior = new QCheckBox(groupBoxDangerous);
    checkBoxMultiInferior->setText(GdbOptionsPage::tr("Debug all children"));
    checkBoxMultiInferior->setToolTip(GdbOptionsPage::tr(
        "<html><head/><body>Keeps debugging all children after a fork."
        "</body></html>"));


    auto formLayout = new QFormLayout(groupBoxDangerous);
    formLayout->addRow(labelDangerous);
    formLayout->addRow(checkBoxTargetAsync);
    formLayout->addRow(checkBoxAutoEnrichParameters);
    formLayout->addRow(checkBoxBreakOnWarning);
    formLayout->addRow(checkBoxBreakOnFatal);
    formLayout->addRow(checkBoxBreakOnAbort);
    if (checkBoxEnableReverseDebugging)
        formLayout->addRow(checkBoxEnableReverseDebugging);
    formLayout->addRow(checkBoxAttemptQuickStart);
    formLayout->addRow(checkBoxMultiInferior);

    auto gridLayout = new QGridLayout(this);
    gridLayout->addWidget(groupBoxDangerous, 0, 0, 2, 1);

    group.insert(action(AutoEnrichParameters), checkBoxAutoEnrichParameters);
    group.insert(action(TargetAsync), checkBoxTargetAsync);
    group.insert(action(BreakOnWarning), checkBoxBreakOnWarning);
    group.insert(action(BreakOnFatal), checkBoxBreakOnFatal);
    group.insert(action(BreakOnAbort), checkBoxBreakOnAbort);
    group.insert(action(AttemptQuickStart), checkBoxAttemptQuickStart);
    group.insert(action(MultiInferior), checkBoxMultiInferior);
    if (checkBoxEnableReverseDebugging)
        group.insert(action(EnableReverseDebugging), checkBoxEnableReverseDebugging);
}

// The "Dangerous" options.
class GdbOptionsPage2 : public Core::IOptionsPage
{
    Q_OBJECT
public:
    GdbOptionsPage2();

    QWidget *widget();
    void apply();
    void finish();

private:
    QPointer<GdbOptionsPageWidget2> m_widget;
};

GdbOptionsPage2::GdbOptionsPage2()
{
    setId("M.Gdb2");
    setDisplayName(tr("GDB Extended"));
    setCategory(Constants::DEBUGGER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Debugger", Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

QWidget *GdbOptionsPage2::widget()
{
    if (!m_widget)
        m_widget = new GdbOptionsPageWidget2;
    return m_widget;
}

void GdbOptionsPage2::apply()
{
    if (m_widget)
        m_widget->group.apply(ICore::settings());
}

void GdbOptionsPage2::finish()
{
    if (m_widget) {
        m_widget->group.finish();
        delete m_widget;
    }
}

// Registration

void addGdbOptionPages(QList<IOptionsPage *> *opts)
{
    opts->push_back(new GdbOptionsPage);
    opts->push_back(new GdbOptionsPage2);
}

} // namespace Internal
} // namespace Debugger

#include "gdboptionspage.moc"
