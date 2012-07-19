/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "gdboptionspage.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerinternalconstants.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>

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

class GdbOptionsPageUi
{
public:
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

    QGroupBox *groupBoxStartupCommands;
    QTextEdit *textEditStartupCommands;

    //QGroupBox *groupBoxPluginDebugging;
    //QRadioButton *radioButtonAllPluginBreakpoints;
    //QRadioButton *radioButtonSelectedPluginBreakpoints;
    //QRadioButton *radioButtonNoPluginBreakpoints;
    //QLabel *labelSelectedPluginBreakpoints;
    //QLineEdit *lineEditSelectedPluginBreakpointsPattern;

    void setupUi(QWidget *q)
    {
        groupBoxGeneral = new QGroupBox(q);
        groupBoxGeneral->setTitle(GdbOptionsPage::tr("General"));

        labelGdbWatchdogTimeout = new QLabel(groupBoxGeneral);
        labelGdbWatchdogTimeout->setText(GdbOptionsPage::tr("GDB timeout:"));
        labelGdbWatchdogTimeout->setToolTip(GdbOptionsPage::tr(
            "This is the number of seconds Qt Creator will wait before\n"
            "it terminates a non-responsive GDB process. The default value of 20 seconds\n"
            "should be sufficient for most applications, but there are situations when\n"
            "loading big libraries or listing source files takes much longer than that\n"
            "on slow machines. In this case, the value should be increased."));

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
            "Allows 'Step Into' to compress several steps into one step\n"
            "for less noisy debugging. For example, the atomic reference\n"
            "counting code is skipped, and a single 'Step Into' for a signal\n"
            "emission ends up directly in the slot connected to it."));

        checkBoxUseMessageBoxForSignals = new QCheckBox(groupBoxGeneral);
        checkBoxUseMessageBoxForSignals->setText(GdbOptionsPage::tr(
            "Show a message box when receiving a signal"));
        checkBoxUseMessageBoxForSignals->setToolTip(GdbOptionsPage::tr(
            "This will show a message box as soon as your application\n"
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
            "This specifies whether the dynamic or the static type of objects will be"
            "displayed. Choosing the dynamic type might be slower."));

        checkBoxLoadGdbInit = new QCheckBox(groupBoxGeneral);
        checkBoxLoadGdbInit->setText(GdbOptionsPage::tr("Load .gdbinit file on startup"));
        checkBoxLoadGdbInit->setToolTip(GdbOptionsPage::tr(
            "This allows or inhibits reading the user's default\n"
            ".gdbinit file on debugger startup."));

        checkBoxWarnOnReleaseBuilds = new QCheckBox(groupBoxGeneral);
        checkBoxWarnOnReleaseBuilds->setText(GdbOptionsPage::tr(
            "Warn when debugging \"Release\" builds"));
        checkBoxWarnOnReleaseBuilds->setToolTip(GdbOptionsPage::tr(
            "Show a warning when starting the debugger "
            "on a binary with insufficient debug information."));

        labelDangerous = new QLabel(GdbOptionsPage::tr(
            "The options below should be used with care."));

        checkBoxTargetAsync = new QCheckBox(groupBoxGeneral);
        checkBoxTargetAsync->setText(GdbOptionsPage::tr(
            "Use asynchronous mode to control the inferior"));

        checkBoxAutoEnrichParameters = new QCheckBox(groupBoxGeneral);
        checkBoxAutoEnrichParameters->setText(GdbOptionsPage::tr(
            "Use common locations for debug information"));
        checkBoxAutoEnrichParameters->setToolTip(GdbOptionsPage::tr(
            "This adds common paths to locations of debug information\n"
            "at debugger startup."));

        checkBoxBreakOnWarning = new QCheckBox(groupBoxGeneral);
        checkBoxBreakOnWarning->setText(GdbOptionsPage::tr("Stop when a qWarning is issued"));

        checkBoxBreakOnFatal = new QCheckBox(groupBoxGeneral);
        checkBoxBreakOnFatal->setText(GdbOptionsPage::tr("Stop when a qFatal is issued"));

        checkBoxBreakOnAbort = new QCheckBox(groupBoxGeneral);
        checkBoxBreakOnAbort->setText(GdbOptionsPage::tr("Stop when abort() is called"));

        checkBoxEnableReverseDebugging = new QCheckBox(groupBoxGeneral);
        checkBoxEnableReverseDebugging->setText(GdbOptionsPage::tr("Enable reverse debugging"));
        checkBoxEnableReverseDebugging->setToolTip(GdbOptionsPage::tr(
            "<html><head/><body><p>Selecting this enables reverse debugging.</p><.p>"
            "<b>Note:</b> This feature is very slow and unstable on the GDB side."
            "It exhibits unpredictable behavior when going backwards over system "
            "calls and is very likely to destroy your debugging session.</p><body></html>"));

        checkBoxAttemptQuickStart = new QCheckBox(groupBoxGeneral);
        checkBoxAttemptQuickStart->setText(GdbOptionsPage::tr("Attempt quick start"));
        checkBoxAttemptQuickStart->setToolTip(GdbOptionsPage::tr("Checking this option "
            "will postpone reading debug information as long as possible. This can result "
            "in faster startup times at the price of not being able to set breakpoints "
            "by file and number."));

        groupBoxStartupCommands = new QGroupBox(q);
        groupBoxStartupCommands->setTitle(GdbOptionsPage::tr("Additional Startup Commands"));

        textEditStartupCommands = new QTextEdit(groupBoxStartupCommands);
        textEditStartupCommands->setAcceptRichText(false);

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
        formLayout->addRow(new QLabel(QString()));
        formLayout->addRow(labelDangerous);
        formLayout->addRow(checkBoxTargetAsync);
        formLayout->addRow(checkBoxAutoEnrichParameters);
        formLayout->addRow(checkBoxBreakOnWarning);
        formLayout->addRow(checkBoxBreakOnFatal);
        formLayout->addRow(checkBoxBreakOnAbort);
        formLayout->addRow(checkBoxEnableReverseDebugging);
        formLayout->addRow(checkBoxAttemptQuickStart);

        QGridLayout *startLayout = new QGridLayout(groupBoxStartupCommands);
        startLayout->addWidget(textEditStartupCommands, 0, 0, 1, 1);

        //QHBoxLayout *horizontalLayout = new QHBoxLayout();
        //horizontalLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Preferred, QSizePolicy::Minimum));
        //horizontalLayout->addWidget(labelSelectedPluginBreakpoints);
        //horizontalLayout->addWidget(lineEditSelectedPluginBreakpointsPattern);

        QGridLayout *gridLayout = new QGridLayout(q);
        gridLayout->addWidget(groupBoxGeneral, 0, 0);
        gridLayout->addWidget(groupBoxStartupCommands, 0, 1);

        //gridLayout->addWidget(groupBoxStartupCommands, 0, 1, 1, 1);
        //gridLayout->addWidget(radioButtonAllPluginBreakpoints, 0, 0, 1, 1);
        //gridLayout->addWidget(radioButtonSelectedPluginBreakpoints, 1, 0, 1, 1);

        //gridLayout->addLayout(horizontalLayout, 2, 0, 1, 1);
        //gridLayout->addWidget(radioButtonNoPluginBreakpoints, 3, 0, 1, 1);
        //gridLayout->addWidget(groupBoxPluginDebugging, 1, 0, 1, 2);
    }
};

GdbOptionsPage::GdbOptionsPage()
    : m_ui(0)
{
    setId(QLatin1String("M.Gdb"));
    setDisplayName(tr("GDB"));
    setCategory(QLatin1String(Constants::DEBUGGER_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Debugger", Constants::DEBUGGER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::DEBUGGER_COMMON_SETTINGS_CATEGORY_ICON));
}

GdbOptionsPage::~GdbOptionsPage()
{
    delete m_ui;
}

QWidget *GdbOptionsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_ui = new GdbOptionsPageUi;
    m_ui->setupUi(w);

    m_group.clear();
    m_group.insert(debuggerCore()->action(GdbStartupCommands),
        m_ui->textEditStartupCommands);
    m_group.insert(debuggerCore()->action(LoadGdbInit),
        m_ui->checkBoxLoadGdbInit);
    m_group.insert(debuggerCore()->action(AutoEnrichParameters),
        m_ui->checkBoxAutoEnrichParameters);
    m_group.insert(debuggerCore()->action(UseDynamicType),
        m_ui->checkBoxUseDynamicType);
    m_group.insert(debuggerCore()->action(TargetAsync),
        m_ui->checkBoxTargetAsync);
    m_group.insert(debuggerCore()->action(WarnOnReleaseBuilds),
        m_ui->checkBoxWarnOnReleaseBuilds);
    m_group.insert(debuggerCore()->action(AdjustBreakpointLocations),
        m_ui->checkBoxAdjustBreakpointLocations);
    m_group.insert(debuggerCore()->action(BreakOnWarning),
        m_ui->checkBoxBreakOnWarning);
    m_group.insert(debuggerCore()->action(BreakOnFatal),
        m_ui->checkBoxBreakOnFatal);
    m_group.insert(debuggerCore()->action(BreakOnAbort),
        m_ui->checkBoxBreakOnAbort);
    m_group.insert(debuggerCore()->action(GdbWatchdogTimeout),
        m_ui->spinBoxGdbWatchdogTimeout);
    m_group.insert(debuggerCore()->action(AttemptQuickStart),
        m_ui->checkBoxAttemptQuickStart);

    m_group.insert(debuggerCore()->action(UseMessageBoxForSignals),
        m_ui->checkBoxUseMessageBoxForSignals);
    m_group.insert(debuggerCore()->action(SkipKnownFrames),
        m_ui->checkBoxSkipKnownFrames);
    m_group.insert(debuggerCore()->action(EnableReverseDebugging),
        m_ui->checkBoxEnableReverseDebugging);
    m_group.insert(debuggerCore()->action(GdbWatchdogTimeout), 0);

    //m_ui->groupBoxPluginDebugging->hide();

    //m_ui->lineEditSelectedPluginBreakpointsPattern->
    //    setEnabled(debuggerCore()->action(SelectedPluginBreakpoints)->value().toBool());
    //connect(m_ui->radioButtonSelectedPluginBreakpoints, SIGNAL(toggled(bool)),
    //    m_ui->lineEditSelectedPluginBreakpointsPattern, SLOT(setEnabled(bool)));

    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << sep << m_ui->groupBoxGeneral->title()
                << sep << m_ui->checkBoxLoadGdbInit->text()
                << sep << m_ui->checkBoxTargetAsync->text()
                << sep << m_ui->checkBoxWarnOnReleaseBuilds->text()
                << sep << m_ui->checkBoxUseDynamicType->text()
                << sep << m_ui->labelGdbWatchdogTimeout->text()
                << sep << m_ui->checkBoxEnableReverseDebugging->text()
                << sep << m_ui->checkBoxSkipKnownFrames->text()
                << sep << m_ui->checkBoxUseMessageBoxForSignals->text()
                << sep << m_ui->checkBoxAdjustBreakpointLocations->text()
                << sep << m_ui->checkBoxAttemptQuickStart->text()
        //        << sep << m_ui->groupBoxPluginDebugging->title()
        //        << sep << m_ui->radioButtonAllPluginBreakpoints->text()
        //        << sep << m_ui->radioButtonSelectedPluginBreakpoints->text()
        //        << sep << m_ui->labelSelectedPluginBreakpoints->text()
        //        << sep << m_ui->radioButtonNoPluginBreakpoints->text()
        ;
        m_searchKeywords.remove(QLatin1Char('&'));
    }

    return w;
}

void GdbOptionsPage::apply()
{
    m_group.apply(Core::ICore::settings());
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
