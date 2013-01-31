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

#include "breakwindow.h"
#include "debuggerinternalconstants.h"
#include "breakhandler.h"
#include "debuggerengine.h"
#include "debuggeractions.h"
#include "debuggercore.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTextEdit>
#include <QVariant>
#include <QVBoxLayout>

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
    explicit BreakpointDialog(BreakpointModelId id, QWidget *parent = 0);
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

BreakpointDialog::BreakpointDialog(BreakpointModelId id, QWidget *parent)
    : QDialog(parent), m_enabledParts(~0), m_previousType(UnknownType),
      m_firstTypeChange(true)
{
    setWindowTitle(tr("Edit Breakpoint Properties"));

    QGroupBox *groupBoxBasic = new QGroupBox(tr("Basic"), this);

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
    QTC_ASSERT(types.size() == BreakpointAtJavaScriptThrow, return);
    m_comboBoxType = new QComboBox(groupBoxBasic);
    m_comboBoxType->setMaxVisibleItems(20);
    m_comboBoxType->addItems(types);
    m_labelType = new QLabel(tr("Breakpoint &type:"), groupBoxBasic);
    m_labelType->setBuddy(m_comboBoxType);

    m_pathChooserFileName = new Utils::PathChooser(groupBoxBasic);
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

    QGroupBox *groupBoxAdvanced = new QGroupBox(tr("Advanced"), this);

    m_checkBoxTracepoint = new QCheckBox(groupBoxAdvanced);
    m_labelTracepoint = new QLabel(tr("T&racepoint only:"), groupBoxAdvanced);
    m_labelTracepoint->setBuddy(m_checkBoxTracepoint);

    m_checkBoxOneShot = new QCheckBox(groupBoxAdvanced);
    m_labelOneShot = new QLabel(tr("&One shot only:"), groupBoxAdvanced);
    m_labelOneShot->setBuddy(m_checkBoxOneShot);

    const QString pathToolTip =
        tr("<html><head/><body><p>Determines how the path is specified "
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
                "be slow with this engine.</li>"
           "</ul></body></html>");
    m_comboBoxPathUsage = new QComboBox(groupBoxAdvanced);
    m_comboBoxPathUsage->addItem(tr("Use Engine Default"));
    m_comboBoxPathUsage->addItem(tr("Use Full Path"));
    m_comboBoxPathUsage->addItem(tr("Use File Name"));
    m_comboBoxPathUsage->setToolTip(pathToolTip);
    m_labelUseFullPath = new QLabel(tr("Pat&h:"), groupBoxAdvanced);
    m_labelUseFullPath->setBuddy(m_comboBoxPathUsage);
    m_labelUseFullPath->setToolTip(pathToolTip);

    const QString moduleToolTip =
        tr("Specifying the module (base name of the library or executable)\n"
           "for function or file type breakpoints can significantly speed up\n"
           "debugger start-up times (CDB, LLDB).");
    m_lineEditModule = new QLineEdit(groupBoxAdvanced);
    m_lineEditModule->setToolTip(moduleToolTip);
    m_labelModule = new QLabel(tr("&Module:"), groupBoxAdvanced);
    m_labelModule->setBuddy(m_lineEditModule);
    m_labelModule->setToolTip(moduleToolTip);

    const QString commandsToolTip =
        tr("Debugger commands to be executed when the breakpoint is hit.\n"
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

    if (id.isValid()) {
        if (DebuggerEngine *engine = breakHandler()->engine(id)) {
            if (!engine->hasCapability(BreakConditionCapability))
                m_enabledParts &= ~ConditionPart;
            if (!engine->hasCapability(BreakModuleCapability))
                m_enabledParts &= ~ModulePart;
            if (!engine->hasCapability(TracePointCapability))
                m_enabledParts &= ~TracePointPart;
        }
    }

    QFormLayout *basicLayout = new QFormLayout(groupBoxBasic);
    basicLayout->addRow(m_labelType, m_comboBoxType);
    basicLayout->addRow(m_labelFileName, m_pathChooserFileName);
    basicLayout->addRow(m_labelLineNumber, m_lineEditLineNumber);
    basicLayout->addRow(m_labelEnabled, m_checkBoxEnabled);
    basicLayout->addRow(m_labelAddress, m_lineEditAddress);
    basicLayout->addRow(m_labelExpression, m_lineEditExpression);
    basicLayout->addRow(m_labelFunction, m_lineEditFunction);
    basicLayout->addRow(m_labelOneShot, m_checkBoxOneShot);

    QFormLayout *advancedLeftLayout = new QFormLayout();
    advancedLeftLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    advancedLeftLayout->addRow(m_labelCondition, m_lineEditCondition);
    advancedLeftLayout->addRow(m_labelIgnoreCount, m_spinBoxIgnoreCount);
    advancedLeftLayout->addRow(m_labelThreadSpec, m_lineEditThreadSpec);
    advancedLeftLayout->addRow(m_labelUseFullPath, m_comboBoxPathUsage);
    advancedLeftLayout->addRow(m_labelModule, m_lineEditModule);

    QFormLayout *advancedRightLayout = new QFormLayout();
    advancedRightLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    advancedRightLayout->addRow(m_labelCommands, m_textEditCommands);
    advancedRightLayout->addRow(m_labelTracepoint, m_checkBoxTracepoint);
    advancedRightLayout->addRow(m_labelMessage, m_lineEditMessage);

    QHBoxLayout *horizontalLayout = new QHBoxLayout(groupBoxAdvanced);
    horizontalLayout->addLayout(advancedLeftLayout);
    horizontalLayout->addSpacing(15);
    horizontalLayout->addLayout(advancedRightLayout);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(groupBoxBasic);
    verticalLayout->addSpacing(10);
    verticalLayout->addWidget(groupBoxAdvanced);
    verticalLayout->addSpacing(10);
    verticalLayout->addWidget(m_buttonBox);
    verticalLayout->setStretchFactor(groupBoxAdvanced, 10);

    connect(m_comboBoxType, SIGNAL(activated(int)), SLOT(typeChanged(int)));
    connect(m_buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), SLOT(reject()));
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
    m_checkBoxTracepoint->setEnabled(partsMask & TracePointPart);

    m_labelCommands->setEnabled(partsMask & TracePointPart);
    m_textEditCommands->setEnabled(partsMask & TracePointPart);

    m_labelMessage->setEnabled(partsMask & TracePointPart);
    m_lineEditMessage->setEnabled(partsMask & TracePointPart);
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
    if (invertedPartsMask & TracePointPart) {
        m_checkBoxTracepoint->setChecked(false);
        m_textEditCommands->clear();
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
    if (partsMask & TracePointPart) {
        data->tracepoint = m_checkBoxTracepoint->isChecked();
        data->command = m_textEditCommands->toPlainText().trimmed();
        data->message = m_lineEditMessage->text();
    }
}

void BreakpointDialog::setParts(unsigned mask, const BreakpointParameters &data)
{
    m_checkBoxEnabled->setChecked(data.enabled);
    m_comboBoxPathUsage->setCurrentIndex(data.pathUsage);
    m_textEditCommands->setPlainText(data.command);
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
}

void BreakpointDialog::typeChanged(int)
{
    BreakpointType previousType = m_previousType;
    const BreakpointType newType = type();
    m_previousType = newType;
    // Save current state.
    switch (previousType) {
    case UnknownType:
        break;
    case BreakpointByFileAndLine:
        getParts(FileAndLinePart|ModulePart|AllConditionParts|TracePointPart, &m_savedParameters);
        break;
    case BreakpointByFunction:
        getParts(FunctionPart|ModulePart|AllConditionParts|TracePointPart, &m_savedParameters);
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
        getParts(AddressPart|AllConditionParts|TracePointPart, &m_savedParameters);
        break;
    case WatchpointAtExpression:
        getParts(ExpressionPart|AllConditionParts|TracePointPart, &m_savedParameters);
        break;
    case BreakpointOnQmlSignalEmit:
        getParts(FunctionPart, &m_savedParameters);
    }

    // Enable and set up new state from saved values.
    switch (newType) {
    case UnknownType:
        break;
    case BreakpointByFileAndLine:
        setParts(FileAndLinePart|AllConditionParts|ModulePart|TracePointPart, m_savedParameters);
        setPartsEnabled(FileAndLinePart|AllConditionParts|ModulePart|TracePointPart);
        clearOtherParts(FileAndLinePart|AllConditionParts|ModulePart|TracePointPart);
        break;
    case BreakpointByFunction:
        setParts(FunctionPart|AllConditionParts|ModulePart|TracePointPart, m_savedParameters);
        setPartsEnabled(FunctionPart|AllConditionParts|ModulePart|TracePointPart);
        clearOtherParts(FunctionPart|AllConditionParts|ModulePart|TracePointPart);
        break;
    case BreakpointAtThrow:
    case BreakpointAtCatch:
    case BreakpointAtFork:
    case BreakpointAtExec:
    //case BreakpointAtVFork:
    case BreakpointAtSysCall:
        clearOtherParts(AllConditionParts|ModulePart|TracePointPart);
        setPartsEnabled(AllConditionParts|TracePointPart);
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
        setParts(AddressPart|AllConditionParts|TracePointPart, m_savedParameters);
        setPartsEnabled(AddressPart|AllConditionParts|TracePointPart|TracePointPart);
        clearOtherParts(AddressPart|AllConditionParts|TracePointPart);
        break;
    case WatchpointAtExpression:
        setParts(ExpressionPart|AllConditionParts|TracePointPart, m_savedParameters);
        setPartsEnabled(ExpressionPart|AllConditionParts|TracePointPart|TracePointPart);
        clearOtherParts(ExpressionPart|AllConditionParts|TracePointPart);
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

    QFormLayout *formLayout = new QFormLayout;
    if (debuggerCore()->currentEngine()->hasCapability(BreakConditionCapability))
        formLayout->addRow(tr("&Condition:"), m_lineEditCondition);
    formLayout->addRow(tr("&Ignore count:"), m_spinBoxIgnoreCount);
    formLayout->addRow(tr("&Thread specification:"), m_lineEditThreadSpec);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(m_buttonBox);

    QObject::connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

///////////////////////////////////////////////////////////////////////
//
// BreakWindow
//
///////////////////////////////////////////////////////////////////////

BreakTreeView::BreakTreeView(QWidget *parent)
    : BaseTreeView(parent)
{
    setWindowIcon(QIcon(QLatin1String(":/debugger/images/debugger_breakpoints.png")));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAlwaysAdjustColumnsAction(debuggerCore()->action(AlwaysAdjustBreakpointsColumnWidths));
    connect(debuggerCore()->action(UseAddressInBreakpointsView),
        SIGNAL(toggled(bool)), SLOT(showAddressColumn(bool)));
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
        QModelIndexList si = sm->selectedIndexes();
        if (si.isEmpty())
            si.append(currentIndex());
        const BreakpointModelIds ids = breakHandler()->findBreakpointsByIndex(si);
        int row = qMin(model()->rowCount() - ids.size() - 1, currentIndex().row());
        deleteBreakpoints(ids);
        setCurrentIndex(si.at(0).sibling(row, 0));
    }
    QTreeView::keyPressEvent(ev);
}

void BreakTreeView::mouseDoubleClickEvent(QMouseEvent *ev)
{
    QModelIndex indexUnderMouse = indexAt(ev->pos());
    if (indexUnderMouse.isValid()) {
        if (indexUnderMouse.column() >= 4) {
            BreakpointModelId id = breakHandler()->findBreakpointByIndex(indexUnderMouse);
            editBreakpoints(BreakpointModelIds() << id);
        }
    } else {
        addBreakpoint();
    }
    BaseTreeView::mouseDoubleClickEvent(ev);
}

void BreakTreeView::setModel(QAbstractItemModel *model)
{
    BaseTreeView::setModel(model);
    resizeColumnToContents(0); // Number
    resizeColumnToContents(3); // Line
    resizeColumnToContents(6); // Ignore count
    connect(model, SIGNAL(layoutChanged()), this, SLOT(expandAll()));
}

void BreakTreeView::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QItemSelectionModel *sm = selectionModel();
    QTC_ASSERT(sm, return);
    QModelIndexList selectedIndices = sm->selectedIndexes();
    QModelIndex indexUnderMouse = indexAt(ev->pos());
    if (selectedIndices.isEmpty() && indexUnderMouse.isValid())
        selectedIndices.append(indexUnderMouse);

    BreakHandler *handler = breakHandler();
    BreakpointModelIds selectedIds = handler->findBreakpointsByIndex(selectedIndices);

    const int rowCount = model()->rowCount();
    QAction *deleteAction = new QAction(tr("Delete Breakpoint"), &menu);
    deleteAction->setEnabled(!selectedIds.isEmpty());

    QAction *deleteAllAction = new QAction(tr("Delete All Breakpoints"), &menu);
    deleteAllAction->setEnabled(model()->rowCount() > 0);

    // Delete by file: Find indices of breakpoints of the same file.
    QAction *deleteByFileAction = 0;
    BreakpointModelIds breakpointsInFile;
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

    QAction *adjustColumnAction =
        new QAction(tr("Adjust Column Widths to Contents"), &menu);

    QAction *editBreakpointAction =
        new QAction(tr("Edit Breakpoint..."), &menu);
    editBreakpointAction->setEnabled(!selectedIds.isEmpty());

    int threadId = 0;
    // FIXME BP: m_engine->threadsHandler()->currentThreadId();
    QString associateTitle = threadId == -1
        ?  tr("Associate Breakpoint With All Threads")
        :  tr("Associate Breakpoint With Thread %1").arg(threadId);
    QAction *associateBreakpointAction = new QAction(associateTitle, &menu);
    associateBreakpointAction->setEnabled(!selectedIds.isEmpty());

    QAction *synchronizeAction =
        new QAction(tr("Synchronize Breakpoints"), &menu);
    synchronizeAction->setEnabled(debuggerCore()->hasSnapshots());

    bool enabled = selectedIds.isEmpty() || handler->isEnabled(selectedIds.at(0));

    const QString str5 = selectedIds.size() > 1
        ? enabled
            ? tr("Disable Selected Breakpoints")
            : tr("Enable Selected Breakpoints")
        : enabled
            ? tr("Disable Breakpoint")
            : tr("Enable Breakpoint");
    QAction *toggleEnabledAction = new QAction(str5, &menu);
    toggleEnabledAction->setEnabled(!selectedIds.isEmpty());

    QAction *addBreakpointAction =
        new QAction(tr("Add Breakpoint..."), this);

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
    menu.addAction(debuggerCore()->action(UseToolTipsInBreakpointsView));
    if (debuggerCore()->currentEngine()->hasCapability(MemoryAddressCapability))
        menu.addAction(debuggerCore()->action(UseAddressInBreakpointsView));
    addBaseContextActions(&menu);

    QAction *act = menu.exec(ev->globalPos());

    if (act == deleteAction)
        deleteBreakpoints(selectedIds);
    else if (act == deleteAllAction)
        deleteBreakpoints(handler->allBreakpointIds());
    else if (act == deleteByFileAction)
        deleteBreakpoints(breakpointsInFile);
    else if (act == adjustColumnAction)
        resizeColumnsToContents();
    else if (act == editBreakpointAction)
        editBreakpoints(selectedIds);
    else if (act == associateBreakpointAction)
        associateBreakpoint(selectedIds, threadId);
    else if (act == synchronizeAction)
        ; //synchronizeBreakpoints();
    else if (act == toggleEnabledAction)
        setBreakpointsEnabled(selectedIds, !enabled);
    else if (act == addBreakpointAction)
        addBreakpoint();
    else
        handleBaseContextAction(act);
}

void BreakTreeView::setBreakpointsEnabled(const BreakpointModelIds &ids, bool enabled)
{
    BreakHandler *handler = breakHandler();
    foreach (const BreakpointModelId id, ids)
        handler->setEnabled(id, enabled);
}

void BreakTreeView::deleteBreakpoints(const BreakpointModelIds &ids)
{
    BreakHandler *handler = breakHandler();
    foreach (const BreakpointModelId id, ids)
       handler->removeBreakpoint(id);
}

void BreakTreeView::editBreakpoint(BreakpointModelId id, QWidget *parent)
{
    BreakpointParameters data = breakHandler()->breakpointData(id);
    BreakpointParts parts = NoParts;
    BreakpointDialog dialog(id, parent);
    if (dialog.showDialog(&data, &parts))
        breakHandler()->changeBreakpointData(id, data, parts);
}

void BreakTreeView::addBreakpoint()
{
    BreakpointParameters data(BreakpointByFileAndLine);
    BreakpointParts parts = NoParts;
    BreakpointDialog dialog(BreakpointModelId(), this);
    dialog.setWindowTitle(tr("Add Breakpoint"));
    if (dialog.showDialog(&data, &parts))
        breakHandler()->appendBreakpoint(data);
}

void BreakTreeView::editBreakpoints(const BreakpointModelIds &ids)
{
    QTC_ASSERT(!ids.isEmpty(), return);

    const BreakpointModelId id = ids.at(0);

    if (ids.size() == 1) {
        editBreakpoint(id, this);
        return;
    }

    // This allows to change properties of multiple breakpoints at a time.
    BreakHandler *handler = breakHandler();
    MultiBreakPointsDialog dialog;
    const QString oldCondition = QString::fromLatin1(handler->condition(id));
    dialog.setCondition(oldCondition);
    const int oldIgnoreCount = handler->ignoreCount(id);
    dialog.setIgnoreCount(oldIgnoreCount);
    const int oldThreadSpec = handler->threadSpec(id);
    dialog.setThreadSpec(oldThreadSpec);

    if (dialog.exec() == QDialog::Rejected)
        return;

    const QString newCondition = dialog.condition();
    const int newIgnoreCount = dialog.ignoreCount();
    const int newThreadSpec = dialog.threadSpec();

    if (newCondition == oldCondition && newIgnoreCount == oldIgnoreCount
            && newThreadSpec == oldThreadSpec)
        return;

    foreach (const BreakpointModelId id, ids) {
        handler->setCondition(id, newCondition.toLatin1());
        handler->setIgnoreCount(id, newIgnoreCount);
        handler->setThreadSpec(id, newThreadSpec);
    }
}

void BreakTreeView::associateBreakpoint(const BreakpointModelIds &ids, int threadId)
{
    BreakHandler *handler = breakHandler();
    foreach (const BreakpointModelId id, ids)
        handler->setThreadSpec(id, threadId);
}

void BreakTreeView::rowActivated(const QModelIndex &index)
{
    breakHandler()->gotoLocation(breakHandler()->findBreakpointByIndex(index));
}

BreakWindow::BreakWindow()
    : BaseWindow(new BreakTreeView)
{
    setWindowTitle(tr("Breakpoints"));
}

} // namespace Internal
} // namespace Debugger

#include "breakwindow.moc"
