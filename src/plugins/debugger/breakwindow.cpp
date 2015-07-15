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

#include "breakwindow.h"
#include "breakhandler.h"
#include "debuggerengine.h"
#include "debuggeractions.h"
#include "debuggercore.h"

#include <coreplugin/mainwindow.h>
#include <utils/checkablemessagebox.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSpinBox>
#include <QTextEdit>

namespace Debugger {
namespace Internal {

class SmallTextEdit : public QTextEdit
{
public:
    explicit SmallTextEdit(QWidget *parent) : QTextEdit(parent) {}
    QSize sizeHint() const { return QSize(QTextEdit::sizeHint().width(), 100); }
    QSize minimumSizeHint() const { return sizeHint(); }
};

///////////////////////////////////////////////////////////////////////
//
// BreakpointDialog: Show a dialog for editing breakpoints. Shows controls
// for the file-and-line, function and address parameters depending on the
// breakpoint type. The controls not applicable to the current type
// (say function name for file-and-line) are disabled and cleared out.
// However,the values are saved and restored once the respective mode
// is again chosen, which is done using m_savedParameters and
// setters/getters taking the parts mask enumeration parameter.
//
///////////////////////////////////////////////////////////////////////

class BreakpointDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BreakpointDialog(Breakpoint b, QWidget *parent = 0);
    bool showDialog(BreakpointParameters *data, BreakpointParts *parts);

    void setParameters(const BreakpointParameters &data);
    BreakpointParameters parameters() const;

public slots:
    void typeChanged(int index);

private:
    void setPartsEnabled(unsigned partsMask);
    void clearOtherParts(unsigned partsMask);
    void getParts(unsigned partsMask, BreakpointParameters *data) const;
    void setParts(unsigned partsMask, const BreakpointParameters &data);

    void setType(BreakpointType type);
    BreakpointType type() const;

    unsigned m_enabledParts;
    BreakpointParameters m_savedParameters;
    BreakpointType m_previousType;
    bool m_firstTypeChange;

    QLabel *m_labelType;
    QComboBox *m_comboBoxType;
    QLabel *m_labelFileName;
    Utils::PathChooser *m_pathChooserFileName;
    QLabel *m_labelLineNumber;
    QLineEdit *m_lineEditLineNumber;
    QLabel *m_labelEnabled;
    QCheckBox *m_checkBoxEnabled;
    QLabel *m_labelAddress;
    QLineEdit *m_lineEditAddress;
    QLabel *m_labelExpression;
    QLineEdit *m_lineEditExpression;
    QLabel *m_labelFunction;
    QLineEdit *m_lineEditFunction;
    QLabel *m_labelTracepoint;
    QCheckBox *m_checkBoxTracepoint;
    QLabel *m_labelOneShot;
    QCheckBox *m_checkBoxOneShot;
    QLabel *m_labelUseFullPath;
    QLabel *m_labelModule;
    QLineEdit *m_lineEditModule;
    QLabel *m_labelCommands;
    QTextEdit *m_textEditCommands;
    QComboBox *m_comboBoxPathUsage;
    QLabel *m_labelMessage;
    QLineEdit *m_lineEditMessage;
    QLabel *m_labelCondition;
    QLineEdit *m_lineEditCondition;
    QLabel *m_labelIgnoreCount;
    QSpinBox *m_spinBoxIgnoreCount;
    QLabel *m_labelThreadSpec;
    QLineEdit *m_lineEditThreadSpec;
    QDialogButtonBox *m_buttonBox;
};

BreakpointDialog::BreakpointDialog(Breakpoint b, QWidget *parent)
    : QDialog(parent), m_enabledParts(~0), m_previousType(UnknownBreakpointType),
      m_firstTypeChange(true)
{
    setWindowTitle(tr("Edit Breakpoint Properties"));

    auto groupBoxBasic = new QGroupBox(tr("Basic"), this);

    // Match BreakpointType (omitting unknown type).
    QStringList types;
    types << tr("File name and line number")
          << tr("Function name")
          << tr("Break on memory address")
          << tr("Break when C++ exception is thrown")
          << tr("Break when C++ exception is caught")
          << tr("Break when function \"main\" starts")
          << tr("Break when a new process is forked")
          << tr("Break when a new process is executed")
          << tr("Break when a system call is executed")
          << tr("Break on data access at fixed address")
          << tr("Break on data access at address given by expression")
          << tr("Break on QML signal emit")
          << tr("Break when JavaScript exception is thrown");
    // We don't list UnknownBreakpointType, so 1 less:
    QTC_CHECK(types.size() + 1 == LastBreakpointType);
    m_comboBoxType = new QComboBox(groupBoxBasic);
    m_comboBoxType->setMaxVisibleItems(20);
    m_comboBoxType->addItems(types);
    m_labelType = new QLabel(tr("Breakpoint &type:"), groupBoxBasic);
    m_labelType->setBuddy(m_comboBoxType);

    m_pathChooserFileName = new Utils::PathChooser(groupBoxBasic);
    m_pathChooserFileName->setHistoryCompleter(QLatin1String("Debugger.Breakpoint.File.History"));
    m_pathChooserFileName->setExpectedKind(Utils::PathChooser::File);
    m_labelFileName = new QLabel(tr("&File name:"), groupBoxBasic);
    m_labelFileName->setBuddy(m_pathChooserFileName);

    m_lineEditLineNumber = new QLineEdit(groupBoxBasic);
    m_labelLineNumber = new QLabel(tr("&Line number:"), groupBoxBasic);
    m_labelLineNumber->setBuddy(m_lineEditLineNumber);

    m_checkBoxEnabled = new QCheckBox(groupBoxBasic);
    m_labelEnabled = new QLabel(tr("&Enabled:"), groupBoxBasic);
    m_labelEnabled->setBuddy(m_checkBoxEnabled);

    m_lineEditAddress = new QLineEdit(groupBoxBasic);
    m_labelAddress = new QLabel(tr("&Address:"), groupBoxBasic);
    m_labelAddress->setBuddy(m_lineEditAddress);

    m_lineEditExpression = new QLineEdit(groupBoxBasic);
    m_labelExpression = new QLabel(tr("&Expression:"), groupBoxBasic);
    m_labelExpression->setBuddy(m_lineEditExpression);

    m_lineEditFunction = new QLineEdit(groupBoxBasic);
    m_labelFunction = new QLabel(tr("Fun&ction:"), groupBoxBasic);
    m_labelFunction->setBuddy(m_lineEditFunction);

    auto groupBoxAdvanced = new QGroupBox(tr("Advanced"), this);

    m_checkBoxTracepoint = new QCheckBox(groupBoxAdvanced);
    m_labelTracepoint = new QLabel(tr("T&racepoint only:"), groupBoxAdvanced);
    m_labelTracepoint->setBuddy(m_checkBoxTracepoint);

    m_checkBoxOneShot = new QCheckBox(groupBoxAdvanced);
    m_labelOneShot = new QLabel(tr("&One shot only:"), groupBoxAdvanced);
    m_labelOneShot->setBuddy(m_checkBoxOneShot);

    const QString pathToolTip =
        tr("<p>Determines how the path is specified "
                "when setting breakpoints:</p><ul>"
           "<li><i>Use Engine Default</i>: Preferred setting of the "
                "debugger engine.</li>"
           "<li><i>Use Full Path</i>: Pass full path, avoiding ambiguities "
                "should files of the same name exist in several modules. "
                "This is the engine default for CDB and LLDB.</li>"
           "<li><i>Use File Name</i>: Pass the file name only. This is "
                "useful when using a source tree whose location does "
                "not match the one used when building the modules. "
                "It is the engine default for GDB as using full paths can "
                "be slow with this engine.</li></ul>");
    m_comboBoxPathUsage = new QComboBox(groupBoxAdvanced);
    m_comboBoxPathUsage->addItem(tr("Use Engine Default"));
    m_comboBoxPathUsage->addItem(tr("Use Full Path"));
    m_comboBoxPathUsage->addItem(tr("Use File Name"));
    m_comboBoxPathUsage->setToolTip(pathToolTip);
    m_labelUseFullPath = new QLabel(tr("Pat&h:"), groupBoxAdvanced);
    m_labelUseFullPath->setBuddy(m_comboBoxPathUsage);
    m_labelUseFullPath->setToolTip(pathToolTip);

    const QString moduleToolTip =
        tr("<p>Specifying the module (base name of the library or executable) "
           "for function or file type breakpoints can significantly speed up "
           "debugger start-up times (CDB, LLDB).");
    m_lineEditModule = new QLineEdit(groupBoxAdvanced);
    m_lineEditModule->setToolTip(moduleToolTip);
    m_labelModule = new QLabel(tr("&Module:"), groupBoxAdvanced);
    m_labelModule->setBuddy(m_lineEditModule);
    m_labelModule->setToolTip(moduleToolTip);

    const QString commandsToolTip =
        tr("<p>Debugger commands to be executed when the breakpoint is hit. "
           "This feature is only available for GDB.");
    m_textEditCommands = new SmallTextEdit(groupBoxAdvanced);
    m_textEditCommands->setToolTip(commandsToolTip);
    m_labelCommands = new QLabel(tr("&Commands:"), groupBoxAdvanced);
    m_labelCommands->setBuddy(m_textEditCommands);
    m_labelCommands->setToolTip(commandsToolTip);

    m_lineEditMessage = new QLineEdit(groupBoxAdvanced);
    m_labelMessage = new QLabel(tr("&Message:"), groupBoxAdvanced);
    m_labelMessage->setBuddy(m_lineEditMessage);

    m_lineEditCondition = new QLineEdit(groupBoxAdvanced);
    m_labelCondition = new QLabel(tr("C&ondition:"), groupBoxAdvanced);
    m_labelCondition->setBuddy(m_lineEditCondition);

    m_spinBoxIgnoreCount = new QSpinBox(groupBoxAdvanced);
    m_spinBoxIgnoreCount->setMinimum(0);
    m_spinBoxIgnoreCount->setMaximum(2147483647);
    m_labelIgnoreCount = new QLabel(tr("&Ignore count:"), groupBoxAdvanced);
    m_labelIgnoreCount->setBuddy(m_spinBoxIgnoreCount);

    m_lineEditThreadSpec = new QLineEdit(groupBoxAdvanced);
    m_labelThreadSpec = new QLabel(tr("&Thread specification:"), groupBoxAdvanced);
    m_labelThreadSpec->setBuddy(m_lineEditThreadSpec);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    if (b) {
        if (DebuggerEngine *engine = b.engine()) {
            if (!engine->hasCapability(BreakConditionCapability))
                m_enabledParts &= ~ConditionPart;
            if (!engine->hasCapability(BreakModuleCapability))
                m_enabledParts &= ~ModulePart;
            if (!engine->hasCapability(TracePointCapability))
                m_enabledParts &= ~TracePointPart;
        }
    }

    auto basicLayout = new QFormLayout(groupBoxBasic);
    basicLayout->addRow(m_labelType, m_comboBoxType);
    basicLayout->addRow(m_labelFileName, m_pathChooserFileName);
    basicLayout->addRow(m_labelLineNumber, m_lineEditLineNumber);
    basicLayout->addRow(m_labelEnabled, m_checkBoxEnabled);
    basicLayout->addRow(m_labelAddress, m_lineEditAddress);
    basicLayout->addRow(m_labelExpression, m_lineEditExpression);
    basicLayout->addRow(m_labelFunction, m_lineEditFunction);
    basicLayout->addRow(m_labelOneShot, m_checkBoxOneShot);

    auto advancedLeftLayout = new QFormLayout();
    advancedLeftLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    advancedLeftLayout->addRow(m_labelCondition, m_lineEditCondition);
    advancedLeftLayout->addRow(m_labelIgnoreCount, m_spinBoxIgnoreCount);
    advancedLeftLayout->addRow(m_labelThreadSpec, m_lineEditThreadSpec);
    advancedLeftLayout->addRow(m_labelUseFullPath, m_comboBoxPathUsage);
    advancedLeftLayout->addRow(m_labelModule, m_lineEditModule);

    auto advancedRightLayout = new QFormLayout();
    advancedRightLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    advancedRightLayout->addRow(m_labelCommands, m_textEditCommands);
    advancedRightLayout->addRow(m_labelTracepoint, m_checkBoxTracepoint);
    advancedRightLayout->addRow(m_labelMessage, m_lineEditMessage);

    auto horizontalLayout = new QHBoxLayout(groupBoxAdvanced);
    horizontalLayout->addLayout(advancedLeftLayout);
    horizontalLayout->addSpacing(15);
    horizontalLayout->addLayout(advancedRightLayout);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(groupBoxBasic);
    verticalLayout->addSpacing(10);
    verticalLayout->addWidget(groupBoxAdvanced);
    verticalLayout->addSpacing(10);
    verticalLayout->addWidget(m_buttonBox);
    verticalLayout->setStretchFactor(groupBoxAdvanced, 10);

    connect(m_comboBoxType, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &BreakpointDialog::typeChanged);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void BreakpointDialog::setType(BreakpointType type)
{
    const int comboIndex = type - 1; // Skip UnknownType.
    if (comboIndex != m_comboBoxType->currentIndex() || m_firstTypeChange) {
        m_comboBoxType->setCurrentIndex(comboIndex);
        typeChanged(comboIndex);
        m_firstTypeChange = false;
    }
}

BreakpointType BreakpointDialog::type() const
{
    const int type = m_comboBoxType->currentIndex() + 1; // Skip unknown type.
    return static_cast<BreakpointType>(type);
}

void BreakpointDialog::setParameters(const BreakpointParameters &data)
{
    m_savedParameters = data;
    setType(data.type);
    setParts(AllParts, data);
}

BreakpointParameters BreakpointDialog::parameters() const
{
    BreakpointParameters data(type());
    getParts(AllParts, &data);
    return data;
}

void BreakpointDialog::setPartsEnabled(unsigned partsMask)
{
    partsMask &= m_enabledParts;
    m_labelFileName->setEnabled(partsMask & FileAndLinePart);
    m_pathChooserFileName->setEnabled(partsMask & FileAndLinePart);
    m_labelLineNumber->setEnabled(partsMask & FileAndLinePart);
    m_lineEditLineNumber->setEnabled(partsMask & FileAndLinePart);
    m_labelUseFullPath->setEnabled(partsMask & FileAndLinePart);
    m_comboBoxPathUsage->setEnabled(partsMask & FileAndLinePart);

    m_labelFunction->setEnabled(partsMask & FunctionPart);
    m_lineEditFunction->setEnabled(partsMask & FunctionPart);

    m_labelOneShot->setEnabled(partsMask & OneShotPart);
    m_checkBoxOneShot->setEnabled(partsMask & OneShotPart);

    m_labelAddress->setEnabled(partsMask & AddressPart);
    m_lineEditAddress->setEnabled(partsMask & AddressPart);
    m_labelExpression->setEnabled(partsMask & ExpressionPart);
    m_lineEditExpression->setEnabled(partsMask & ExpressionPart);

    m_labelCondition->setEnabled(partsMask & ConditionPart);
    m_lineEditCondition->setEnabled(partsMask & ConditionPart);
    m_labelIgnoreCount->setEnabled(partsMask & IgnoreCountPart);
    m_spinBoxIgnoreCount->setEnabled(partsMask & IgnoreCountPart);
    m_labelThreadSpec->setEnabled(partsMask & ThreadSpecPart);
    m_lineEditThreadSpec->setEnabled(partsMask & ThreadSpecPart);

    m_labelModule->setEnabled(partsMask & ModulePart);
    m_lineEditModule->setEnabled(partsMask & ModulePart);

    m_labelTracepoint->setEnabled(partsMask & TracePointPart);
    m_labelTracepoint->hide();
    m_checkBoxTracepoint->setEnabled(partsMask & TracePointPart);
    m_checkBoxTracepoint->hide();

    m_labelCommands->setEnabled(partsMask & CommandPart);
    m_textEditCommands->setEnabled(partsMask & CommandPart);

    m_labelMessage->setEnabled(partsMask & TracePointPart);
    m_labelMessage->hide();
    m_lineEditMessage->setEnabled(partsMask & TracePointPart);
    m_lineEditMessage->hide();
}

void BreakpointDialog::clearOtherParts(unsigned partsMask)
{
    const unsigned invertedPartsMask = ~partsMask;
    if (invertedPartsMask & FileAndLinePart) {
        m_pathChooserFileName->setPath(QString());
        m_lineEditLineNumber->clear();
        m_comboBoxPathUsage->setCurrentIndex(BreakpointPathUsageEngineDefault);
    }

    if (invertedPartsMask & FunctionPart)
        m_lineEditFunction->clear();

    if (invertedPartsMask & AddressPart)
        m_lineEditAddress->clear();
    if (invertedPartsMask & ExpressionPart)
        m_lineEditExpression->clear();

    if (invertedPartsMask & ConditionPart)
        m_lineEditCondition->clear();
    if (invertedPartsMask & IgnoreCountPart)
        m_spinBoxIgnoreCount->clear();
    if (invertedPartsMask & ThreadSpecPart)
        m_lineEditThreadSpec->clear();
    if (invertedPartsMask & ModulePart)
        m_lineEditModule->clear();

    if (partsMask & OneShotPart)
        m_checkBoxOneShot->setChecked(false);
    if (invertedPartsMask & CommandPart)
        m_textEditCommands->clear();
    if (invertedPartsMask & TracePointPart) {
        m_checkBoxTracepoint->setChecked(false);
        m_lineEditMessage->clear();
    }
}

void BreakpointDialog::getParts(unsigned partsMask, BreakpointParameters *data) const
{
    data->enabled = m_checkBoxEnabled->isChecked();

    if (partsMask & FileAndLinePart) {
        data->lineNumber = m_lineEditLineNumber->text().toInt();
        data->pathUsage = static_cast<BreakpointPathUsage>(m_comboBoxPathUsage->currentIndex());
        data->fileName = m_pathChooserFileName->path();
    }
    if (partsMask & FunctionPart)
        data->functionName = m_lineEditFunction->text();

    if (partsMask & AddressPart)
        data->address = m_lineEditAddress->text().toULongLong(0, 0);
    if (partsMask & ExpressionPart)
        data->expression = m_lineEditExpression->text();

    if (partsMask & ConditionPart)
        data->condition = m_lineEditCondition->text().toUtf8();
    if (partsMask & IgnoreCountPart)
        data->ignoreCount = m_spinBoxIgnoreCount->text().toInt();
    if (partsMask & ThreadSpecPart)
        data->threadSpec =
            BreakHandler::threadSpecFromDisplay(m_lineEditThreadSpec->text());
    if (partsMask & ModulePart)
        data->module = m_lineEditModule->text();

    if (partsMask & OneShotPart)
        data->oneShot = m_checkBoxOneShot->isChecked();
    if (partsMask & CommandPart)
        data->command = m_textEditCommands->toPlainText().trimmed();
    if (partsMask & TracePointPart) {
        data->tracepoint = m_checkBoxTracepoint->isChecked();
        data->message = m_lineEditMessage->text();
    }
}

void BreakpointDialog::setParts(unsigned mask, const BreakpointParameters &data)
{
    m_checkBoxEnabled->setChecked(data.enabled);
    m_comboBoxPathUsage->setCurrentIndex(data.pathUsage);
    m_lineEditMessage->setText(data.message);

    if (mask & FileAndLinePart) {
        m_pathChooserFileName->setPath(data.fileName);
        m_lineEditLineNumber->setText(QString::number(data.lineNumber));
    }

    if (mask & FunctionPart)
        m_lineEditFunction->setText(data.functionName);

    if (mask & AddressPart) {
        if (data.address) {
            m_lineEditAddress->setText(
                QString::fromLatin1("0x%1").arg(data.address, 0, 16));
        } else {
            m_lineEditAddress->clear();
        }
    }

    if (mask & ExpressionPart) {
        if (!data.expression.isEmpty())
            m_lineEditExpression->setText(data.expression);
        else
            m_lineEditExpression->clear();
    }

    if (mask & ConditionPart)
        m_lineEditCondition->setText(QString::fromUtf8(data.condition));
    if (mask & IgnoreCountPart)
        m_spinBoxIgnoreCount->setValue(data.ignoreCount);
    if (mask & ThreadSpecPart)
        m_lineEditThreadSpec->
            setText(BreakHandler::displayFromThreadSpec(data.threadSpec));
    if (mask & ModulePart)
        m_lineEditModule->setText(data.module);

    if (mask & OneShotPart)
        m_checkBoxOneShot->setChecked(data.oneShot);
    if (mask & TracePointPart)
        m_checkBoxTracepoint->setChecked(data.tracepoint);
    if (mask & CommandPart)
        m_textEditCommands->setPlainText(data.command);
}

void BreakpointDialog::typeChanged(int)
{
    BreakpointType previousType = m_previousType;
    const BreakpointType newType = type();
    m_previousType = newType;
    // Save current state.
    switch (previousType) {
    case UnknownBreakpointType:
    case LastBreakpointType:
        break;
    case BreakpointByFileAndLine:
        getParts(FileAndLinePart|ModulePart|AllConditionParts|TracePointPart|CommandPart, &m_savedParameters);
        break;
    case BreakpointByFunction:
        getParts(FunctionPart|ModulePart|AllConditionParts|TracePointPart|CommandPart, &m_savedParameters);
        break;
    case BreakpointAtThrow:
    case BreakpointAtCatch:
    case BreakpointAtMain:
    case BreakpointAtFork:
    case BreakpointAtExec:
    //case BreakpointAtVFork:
    case BreakpointAtSysCall:
    case BreakpointAtJavaScriptThrow:
        break;
    case BreakpointByAddress:
    case WatchpointAtAddress:
        getParts(AddressPart|AllConditionParts|TracePointPart|CommandPart, &m_savedParameters);
        break;
    case WatchpointAtExpression:
        getParts(ExpressionPart|AllConditionParts|TracePointPart|CommandPart, &m_savedParameters);
        break;
    case BreakpointOnQmlSignalEmit:
        getParts(FunctionPart, &m_savedParameters);
    }

    // Enable and set up new state from saved values.
    switch (newType) {
    case UnknownBreakpointType:
    case LastBreakpointType:
        break;
    case BreakpointByFileAndLine:
        setParts(FileAndLinePart|AllConditionParts|ModulePart|TracePointPart|CommandPart, m_savedParameters);
        setPartsEnabled(FileAndLinePart|AllConditionParts|ModulePart|TracePointPart|CommandPart);
        clearOtherParts(FileAndLinePart|AllConditionParts|ModulePart|TracePointPart|CommandPart);
        break;
    case BreakpointByFunction:
        setParts(FunctionPart|AllConditionParts|ModulePart|TracePointPart|CommandPart, m_savedParameters);
        setPartsEnabled(FunctionPart|AllConditionParts|ModulePart|TracePointPart|CommandPart);
        clearOtherParts(FunctionPart|AllConditionParts|ModulePart|TracePointPart|CommandPart);
        break;
    case BreakpointAtThrow:
    case BreakpointAtCatch:
    case BreakpointAtFork:
    case BreakpointAtExec:
    //case BreakpointAtVFork:
    case BreakpointAtSysCall:
        clearOtherParts(AllConditionParts|ModulePart|TracePointPart|CommandPart);
        setPartsEnabled(AllConditionParts|TracePointPart|CommandPart);
        break;
    case BreakpointAtJavaScriptThrow:
        clearOtherParts(AllParts);
        setPartsEnabled(0);
        break;
    case BreakpointAtMain:
        m_lineEditFunction->setText(QLatin1String("main")); // Just for display
        clearOtherParts(0);
        setPartsEnabled(0);
        break;
    case BreakpointByAddress:
    case WatchpointAtAddress:
        setParts(AddressPart|AllConditionParts|TracePointPart|CommandPart, m_savedParameters);
        setPartsEnabled(AddressPart|AllConditionParts|TracePointPart|CommandPart);
        clearOtherParts(AddressPart|AllConditionParts|TracePointPart|CommandPart);
        break;
    case WatchpointAtExpression:
        setParts(ExpressionPart|AllConditionParts|TracePointPart|CommandPart, m_savedParameters);
        setPartsEnabled(ExpressionPart|AllConditionParts|TracePointPart|CommandPart);
        clearOtherParts(ExpressionPart|AllConditionParts|TracePointPart|CommandPart);
        break;
    case BreakpointOnQmlSignalEmit:
        setParts(FunctionPart, m_savedParameters);
        setPartsEnabled(FunctionPart);
        clearOtherParts(FunctionPart);
    }
}

bool BreakpointDialog::showDialog(BreakpointParameters *data,
    BreakpointParts *parts)
{
    setParameters(*data);
    if (exec() != QDialog::Accepted)
        return false;

    // Check if changed.
    const BreakpointParameters newParameters = parameters();
    *parts = data->differencesTo(newParameters);
    if (!*parts)
        return false;

    *data = newParameters;
    return true;
}

// Dialog allowing changing properties of multiple breakpoints at a time.
class MultiBreakPointsDialog : public QDialog
{
    Q_OBJECT

public:
    MultiBreakPointsDialog(QWidget *parent = 0);

    QString condition() const { return m_lineEditCondition->text(); }
    int ignoreCount() const { return m_spinBoxIgnoreCount->value(); }
    int threadSpec() const
       { return BreakHandler::threadSpecFromDisplay(m_lineEditThreadSpec->text()); }

    void setCondition(const QString &c) { m_lineEditCondition->setText(c); }
    void setIgnoreCount(int i) { m_spinBoxIgnoreCount->setValue(i); }
    void setThreadSpec(int t)
        { return m_lineEditThreadSpec->setText(BreakHandler::displayFromThreadSpec(t)); }

private:
    QLineEdit *m_lineEditCondition;
    QSpinBox *m_spinBoxIgnoreCount;
    QLineEdit *m_lineEditThreadSpec;
    QDialogButtonBox *m_buttonBox;
};

MultiBreakPointsDialog::MultiBreakPointsDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Edit Breakpoint Properties"));

    m_lineEditCondition = new QLineEdit(this);
    m_spinBoxIgnoreCount = new QSpinBox(this);
    m_spinBoxIgnoreCount->setMinimum(0);
    m_spinBoxIgnoreCount->setMaximum(2147483647);
    m_lineEditThreadSpec = new QLineEdit(this);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    auto formLayout = new QFormLayout;
    if (currentEngine()->hasCapability(BreakConditionCapability))
        formLayout->addRow(tr("&Condition:"), m_lineEditCondition);
    formLayout->addRow(tr("&Ignore count:"), m_spinBoxIgnoreCount);
    formLayout->addRow(tr("&Thread specification:"), m_lineEditThreadSpec);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

///////////////////////////////////////////////////////////////////////
//
// BreakWindow
//
///////////////////////////////////////////////////////////////////////

BreakTreeView::BreakTreeView()
{
    setWindowIcon(QIcon(QLatin1String(":/debugger/images/debugger_breakpoints.png")));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(action(UseAddressInBreakpointsView), &QAction::toggled,
            this, &BreakTreeView::showAddressColumn);
}

void BreakTreeView::showAddressColumn(bool on)
{
    setColumnHidden(7, !on);
}

void BreakTreeView::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Delete) {
        QItemSelectionModel *sm = selectionModel();
        QTC_ASSERT(sm, return);
        QModelIndexList si = sm->selectedRows();
        if (si.isEmpty())
            si.append(currentIndex());
        const Breakpoints ids = breakHandler()->findBreakpointsByIndex(si);
        int row = qMin(model()->rowCount() - ids.size() - 1, currentIndex().row());
        deleteBreakpoints(ids);
        setCurrentIndex(model()->index(row, 0));
    } else if (ev->key() == Qt::Key_Space) {
        QItemSelectionModel *sm = selectionModel();
        QTC_ASSERT(sm, return);
        const QModelIndexList selectedIds = sm->selectedRows();
        if (!selectedIds.isEmpty()) {
            const Breakpoints items = breakHandler()->findBreakpointsByIndex(selectedIds);
            const bool isEnabled = items.isEmpty() || items.at(0).isEnabled();
            setBreakpointsEnabled(items, !isEnabled);
            foreach (const QModelIndex &id, selectedIds)
                update(id);
        }
    }
    BaseTreeView::keyPressEvent(ev);
}

void BreakTreeView::mouseDoubleClickEvent(QMouseEvent *ev)
{
    QModelIndex indexUnderMouse = indexAt(ev->pos());
    if (indexUnderMouse.isValid()) {
        if (indexUnderMouse.column() >= 4) {
            Breakpoint b = breakHandler()->findBreakpointByIndex(indexUnderMouse);
            QTC_ASSERT(b, return);
            editBreakpoints(Breakpoints() << b);
        }
    } else {
        addBreakpoint();
    }
    BaseTreeView::mouseDoubleClickEvent(ev);
}

void BreakTreeView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QItemSelectionModel *sm = selectionModel();
    QTC_ASSERT(sm, return);
    QModelIndexList selectedIndices = sm->selectedRows();
    QModelIndex indexUnderMouse = indexAt(ev->pos());
    if (selectedIndices.isEmpty() && indexUnderMouse.isValid())
        selectedIndices.append(indexUnderMouse);

    BreakHandler *handler = breakHandler();
    Breakpoints selectedItems = handler->findBreakpointsByIndex(selectedIndices);

    const int rowCount = model()->rowCount();
    auto deleteAction = new QAction(tr("Delete Selected Breakpoints"), &menu);
    deleteAction->setEnabled(!selectedItems.empty());

    auto deleteAllAction = new QAction(tr("Delete All Breakpoints"), &menu);
    deleteAllAction->setEnabled(model()->rowCount() > 0);

    // Delete by file: Find indices of breakpoints of the same file.
    QAction *deleteByFileAction = 0;
    Breakpoints breakpointsInFile;
    if (indexUnderMouse.isValid()) {
        const QModelIndex index = indexUnderMouse.sibling(indexUnderMouse.row(), 2);
        const QString file = index.data().toString();
        if (!file.isEmpty()) {
            for (int i = 0; i != rowCount; ++i)
                if (index.data().toString() == file)
                    breakpointsInFile.append(handler->findBreakpointByIndex(index));
            if (breakpointsInFile.size() > 1) {
                deleteByFileAction =
                    new QAction(tr("Delete Breakpoints of \"%1\"").arg(file), &menu);
                deleteByFileAction->setEnabled(true);
            }
        }
    }
    if (!deleteByFileAction) {
        deleteByFileAction = new QAction(tr("Delete Breakpoints of File"), &menu);
        deleteByFileAction->setEnabled(false);
    }

    auto editBreakpointAction = new QAction(tr("Edit Breakpoint..."), &menu);
    editBreakpointAction->setEnabled(!selectedItems.isEmpty());

    int threadId = 0;
    // FIXME BP: m_engine->threadsHandler()->currentThreadId();
    QString associateTitle = threadId == -1
        ?  tr("Associate Breakpoint With All Threads")
        :  tr("Associate Breakpoint With Thread %1").arg(threadId);
    auto associateBreakpointAction = new QAction(associateTitle, &menu);
    associateBreakpointAction->setEnabled(!selectedItems.isEmpty());

    auto synchronizeAction = new QAction(tr("Synchronize Breakpoints"), &menu);
    synchronizeAction->setEnabled(Internal::hasSnapshots());

    bool enabled = selectedItems.isEmpty() || selectedItems.at(0).isEnabled();

    const QString str5 = selectedItems.size() > 1
        ? enabled
            ? tr("Disable Selected Breakpoints")
            : tr("Enable Selected Breakpoints")
        : enabled
            ? tr("Disable Breakpoint")
            : tr("Enable Breakpoint");
    auto toggleEnabledAction = new QAction(str5, &menu);
    toggleEnabledAction->setEnabled(!selectedItems.isEmpty());

    auto addBreakpointAction = new QAction(tr("Add Breakpoint..."), this);

    menu.addAction(addBreakpointAction);
    menu.addAction(deleteAction);
    menu.addAction(editBreakpointAction);
    menu.addAction(associateBreakpointAction);
    menu.addAction(toggleEnabledAction);
    menu.addSeparator();
    menu.addAction(deleteAllAction);
    //menu.addAction(deleteByFileAction);
    menu.addSeparator();
    menu.addAction(synchronizeAction);
    menu.addSeparator();
    menu.addAction(action(UseToolTipsInBreakpointsView));
    if (currentEngine()->hasCapability(MemoryAddressCapability))
        menu.addAction(action(UseAddressInBreakpointsView));
    menu.addSeparator();
    menu.addAction(action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == deleteAction)
        deleteBreakpoints(selectedItems);
    else if (act == deleteAllAction)
        deleteAllBreakpoints();
    else if (act == deleteByFileAction)
        deleteBreakpoints(breakpointsInFile);
    else if (act == editBreakpointAction)
        editBreakpoints(selectedItems);
    else if (act == associateBreakpointAction)
        associateBreakpoint(selectedItems, threadId);
    else if (act == synchronizeAction)
        ; //synchronizeBreakpoints();
    else if (act == toggleEnabledAction)
        setBreakpointsEnabled(selectedItems, !enabled);
    else if (act == addBreakpointAction)
        addBreakpoint();
}

void BreakTreeView::setBreakpointsEnabled(const Breakpoints &bps, bool enabled)
{
    foreach (Breakpoint b, bps)
        b.setEnabled(enabled);
}

void BreakTreeView::deleteAllBreakpoints()
{
    if (Utils::CheckableMessageBox::doNotAskAgainQuestion(Core::ICore::dialogParent(),
           tr("Remove All Breakpoints"),
           tr("Are you sure you want to remove all breakpoints "
              "from all files in the current session?"),
           Core::ICore::settings(),
           QLatin1String("RemoveAllBreakpoints")) == QDialogButtonBox::Yes)
        deleteBreakpoints(breakHandler()->allBreakpoints());
}

void BreakTreeView::deleteBreakpoints(const Breakpoints &bps)
{
    foreach (Breakpoint bp, bps)
        bp.removeBreakpoint();
}

void BreakTreeView::editBreakpoint(Breakpoint bp, QWidget *parent)
{
    BreakpointParameters data = bp.parameters();
    BreakpointParts parts = NoParts;

    BreakpointDialog dialog(bp, parent);
    if (!dialog.showDialog(&data, &parts))
        return;

    bp.changeBreakpointData(data);
}

void BreakTreeView::addBreakpoint()
{
    BreakpointParameters data(BreakpointByFileAndLine);
    BreakpointParts parts = NoParts;
    BreakpointDialog dialog(Breakpoint(), this);
    dialog.setWindowTitle(tr("Add Breakpoint"));
    if (dialog.showDialog(&data, &parts))
        breakHandler()->appendBreakpoint(data);
}

void BreakTreeView::editBreakpoints(const Breakpoints &bps)
{
    QTC_ASSERT(!bps.isEmpty(), return);

    const Breakpoint bp = bps.at(0);

    if (bps.size() == 1) {
        editBreakpoint(bp, this);
        return;
    }

    // This allows to change properties of multiple breakpoints at a time.
    if (!bp)
        return;

    MultiBreakPointsDialog dialog;
    dialog.setCondition(QString::fromLatin1(bp.condition()));
    dialog.setIgnoreCount(bp.ignoreCount());
    dialog.setThreadSpec(bp.threadSpec());

    if (dialog.exec() == QDialog::Rejected)
        return;

    const QString newCondition = dialog.condition();
    const int newIgnoreCount = dialog.ignoreCount();
    const int newThreadSpec = dialog.threadSpec();

    foreach (Breakpoint bp, bps) {
        if (bp) {
            bp.setCondition(newCondition.toLatin1());
            bp.setIgnoreCount(newIgnoreCount);
            bp.setThreadSpec(newThreadSpec);
        }
    }
}

void BreakTreeView::associateBreakpoint(const Breakpoints &bps, int threadId)
{
    foreach (Breakpoint bp, bps) {
        if (bp)
            bp.setThreadSpec(threadId);
    }
}

void BreakTreeView::rowActivated(const QModelIndex &index)
{
    if (Breakpoint bp = breakHandler()->findBreakpointByIndex(index))
        bp.gotoLocation();
}

} // namespace Internal
} // namespace Debugger

#include "breakwindow.moc"
