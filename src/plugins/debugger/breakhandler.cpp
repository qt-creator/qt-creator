/***************************************************************************
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

#include "breakhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggericons.h"
#include "debuggerinternalconstants.h"
#include "disassembleragent.h"
#include "enginemanager.h"
#include "simplifytype.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugin.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/session.h>

#include <texteditor/textmark.h>
#include <texteditor/texteditor.h>

#include <utils/basetreeview.h>
#include <utils/checkablemessagebox.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/theme/theme.h>

#if USE_BREAK_MODEL_TEST
#include <modeltest.h>
#endif

#include <QApplication>
#include <QTimerEvent>
#include <QDir>
#include <QDebug>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QMenu>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {
namespace Internal {

static BreakpointManager *theBreakpointManager = nullptr;

//
// BreakpointMarker
//

// The red blob on the left side in the cpp editor.
class BreakpointMarker : public TextEditor::TextMark
{
public:
    BreakpointMarker(const Breakpoint &bp, const FileName &fileName, int lineNumber)
        : TextMark(fileName, lineNumber, Constants::TEXT_MARK_CATEGORY_BREAKPOINT), m_bp(bp)
    {
        setColor(Theme::Debugger_Breakpoint_TextMarkColor);
        setDefaultToolTip(QApplication::translate("BreakHandler", "Breakpoint"));
        setPriority(TextEditor::TextMark::NormalPriority);
        setIcon(bp->icon());
        setToolTip(bp->toolTip());
    }

    void updateLineNumber(int lineNumber) final
    {
        TextMark::updateLineNumber(lineNumber);
        QTC_ASSERT(m_bp, return);
        m_bp->setLineNumber(lineNumber);
        if (GlobalBreakpoint gbp = m_bp->globalBreakpoint())
            gbp->m_params.lineNumber = lineNumber;
    }

    void updateFileName(const FileName &fileName) final
    {
        TextMark::updateFileName(fileName);
        QTC_ASSERT(m_bp, return);
        m_bp->setFileName(fileName.toString());
        if (GlobalBreakpoint gbp = m_bp->globalBreakpoint())
            gbp->m_params.fileName = fileName.toString();
    }

    bool isDraggable() const final { return true; }

    void dragToLine(int line) final
    {
        QTC_ASSERT(m_bp, return);
        GlobalBreakpoint gbp = m_bp->globalBreakpoint();
        if (!gbp)
            return;
        BreakpointParameters params = gbp->m_params;
        params.lineNumber = line;
        gbp->deleteBreakpoint();
        BreakpointManager::createBreakpoint(params);
    }

    bool isClickable() const final { return true; }

    void clicked() final
    {
        QTC_ASSERT(m_bp, return);
        m_bp->deleteGlobalOrThisBreakpoint();
    }

public:
    Breakpoint m_bp;
};

// The red blob on the left side in the cpp editor.
class GlobalBreakpointMarker : public TextEditor::TextMark
{
public:
    GlobalBreakpointMarker(GlobalBreakpoint gbp, const FileName &fileName, int lineNumber)
        : TextMark(fileName, lineNumber, Constants::TEXT_MARK_CATEGORY_BREAKPOINT), m_gbp(gbp)
    {
        setColor(Theme::Debugger_Breakpoint_TextMarkColor);
        setDefaultToolTip(QApplication::translate("BreakHandler", "Breakpoint"));
        setPriority(TextEditor::TextMark::NormalPriority);
        setIcon(m_gbp->icon());
    }

    void removedFromEditor() final
    {
        QTC_ASSERT(m_gbp, return);
        m_gbp->removeBreakpointFromModel();
    }

    void updateLineNumber(int lineNumber) final
    {
        TextMark::updateLineNumber(lineNumber);
        QTC_ASSERT(m_gbp, return);

        // Ignore updates to the "real" line number while the debugger is
        // running, as this can be triggered by moving the breakpoint to
        // the next line that generated code.

        m_gbp->m_params.lineNumber = lineNumber;
        m_gbp->updateMarker();
        m_gbp->update();
    }

    void updateFileName(const FileName &fileName) final
    {
        TextMark::updateFileName(fileName);
        QTC_ASSERT(m_gbp, return);
        m_gbp->m_params.fileName = fileName.toString();
        m_gbp->update();
    }

    bool isDraggable() const final { return true; }

    void dragToLine(int line) final
    {
        QTC_ASSERT(m_gbp, return);
        QTC_ASSERT(BreakpointManager::globalBreakpoints().contains(m_gbp), return);
        BreakpointParameters params = m_gbp->m_params;
        params.lineNumber = line;
        GlobalBreakpoint gbp = m_gbp;
        m_gbp = GlobalBreakpoint();
        gbp->deleteBreakpoint();
        m_gbp = BreakpointManager::createBreakpoint(params);
    }

    bool isClickable() const final { return true; }

    void clicked() final
    {
        QTC_ASSERT(m_gbp, return);
        m_gbp->removeBreakpointFromModel();
    }

public:
    GlobalBreakpoint m_gbp;
};

static QString stateToString(BreakpointState state)
{
    switch (state) {
        case BreakpointNew:
            return BreakHandler::tr("New");
        case BreakpointInsertionRequested:
            return BreakHandler::tr("Insertion requested");
        case BreakpointInsertionProceeding:
            return BreakHandler::tr("Insertion proceeding");
        case BreakpointUpdateRequested:
            return BreakHandler::tr("Change requested");
        case BreakpointUpdateProceeding:
            return BreakHandler::tr("Change proceeding");
        case BreakpointInserted:
            return BreakHandler::tr("Breakpoint inserted");
        case BreakpointRemoveRequested:
            return BreakHandler::tr("Removal requested");
        case BreakpointRemoveProceeding:
            return BreakHandler::tr("Removal proceeding");
        case BreakpointDead:
            return BreakHandler::tr("Dead");
        default:
            break;
    }
    //: Invalid breakpoint state.
    return BreakHandler::tr("<invalid state>");
}

static QString msgBreakpointAtSpecialFunc(const QString &func)
{
    return BreakHandler::tr("Breakpoint at \"%1\"").arg(func);
}

static QString typeToString(BreakpointType type)
{
    switch (type) {
        case BreakpointByFileAndLine:
            return BreakHandler::tr("Breakpoint by File and Line");
        case BreakpointByFunction:
            return BreakHandler::tr("Breakpoint by Function");
        case BreakpointByAddress:
            return BreakHandler::tr("Breakpoint by Address");
        case BreakpointAtThrow:
            return msgBreakpointAtSpecialFunc("throw");
        case BreakpointAtCatch:
            return msgBreakpointAtSpecialFunc("catch");
        case BreakpointAtFork:
            return msgBreakpointAtSpecialFunc("fork");
        case BreakpointAtExec:
            return msgBreakpointAtSpecialFunc("exec");
        //case BreakpointAtVFork:
        //    return msgBreakpointAtSpecialFunc("vfork");
        case BreakpointAtSysCall:
            return msgBreakpointAtSpecialFunc("syscall");
        case BreakpointAtMain:
            return BreakHandler::tr("Breakpoint at Function \"main()\"");
        case WatchpointAtAddress:
            return BreakHandler::tr("Watchpoint at Address");
        case WatchpointAtExpression:
            return BreakHandler::tr("Watchpoint at Expression");
        case BreakpointOnQmlSignalEmit:
            return BreakHandler::tr("Breakpoint on QML Signal Emit");
        case BreakpointAtJavaScriptThrow:
            return BreakHandler::tr("Breakpoint at JavaScript throw");
        case UnknownBreakpointType:
        case LastBreakpointType:
            break;
    }
    return BreakHandler::tr("Unknown Breakpoint Type");
}

class LeftElideDelegate : public QStyledItemDelegate
{
public:
    LeftElideDelegate() = default;

    void paint(QPainter *pain, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        opt.textElideMode = Qt::ElideLeft;
        QStyledItemDelegate::paint(pain, opt, index);
    }
};

class SmallTextEdit : public QTextEdit
{
public:
    explicit SmallTextEdit(QWidget *parent) : QTextEdit(parent) {}
    QSize sizeHint() const override { return QSize(QTextEdit::sizeHint().width(), 100); }
    QSize minimumSizeHint() const override { return sizeHint(); }
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
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::BreakHandler)

public:
    explicit BreakpointDialog(unsigned int enabledParts, QWidget *parent = nullptr);
    bool showDialog(BreakpointParameters *data, BreakpointParts *parts);

    void setParameters(const BreakpointParameters &data);
    BreakpointParameters parameters() const;

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

BreakpointDialog::BreakpointDialog(unsigned int enabledParts, QWidget *parent)
    : QDialog(parent), m_enabledParts(enabledParts), m_previousType(UnknownBreakpointType),
      m_firstTypeChange(true)
{
    setWindowTitle(tr("Edit Breakpoint Properties"));

    auto groupBoxBasic = new QGroupBox(tr("Basic"), this);

    // Match BreakpointType (omitting unknown type).
    const QStringList types = {
        tr("File Name and Line Number"),
        tr("Function Name"),
        tr("Break on Memory Address"),
        tr("Break When C++ Exception Is Thrown"),
        tr("Break When C++ Exception Is Caught"),
        tr("Break When Function \"main\" Starts"),
        tr("Break When a New Process Is Forked"),
        tr("Break When a New Process Is Executed"),
        tr("Break When a System Call Is Executed"),
        tr("Break on Data Access at Fixed Address"),
        tr("Break on Data Access at Address Given by Expression"),
        tr("Break on QML Signal Emit"),
        tr("Break When JavaScript Exception Is Thrown")
    };
    // We don't list UnknownBreakpointType, so 1 less:
    QTC_CHECK(types.size() + 1 == LastBreakpointType);
    m_comboBoxType = new QComboBox(groupBoxBasic);
    m_comboBoxType->setMaxVisibleItems(20);
    m_comboBoxType->addItems(types);
    m_labelType = new QLabel(tr("Breakpoint &type:"), groupBoxBasic);
    m_labelType->setBuddy(m_comboBoxType);

    m_pathChooserFileName = new PathChooser(groupBoxBasic);
    m_pathChooserFileName->setHistoryCompleter("Debugger.Breakpoint.File.History");
    m_pathChooserFileName->setExpectedKind(PathChooser::File);
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
            "<p>" + tr("Specifying the module (base name of the library or executable) "
                       "for function or file type breakpoints can significantly speed up "
                       "debugger startup times (CDB, LLDB).") + "</p>";
    m_lineEditModule = new QLineEdit(groupBoxAdvanced);
    m_lineEditModule->setToolTip(moduleToolTip);
    m_labelModule = new QLabel(tr("&Module:"), groupBoxAdvanced);
    m_labelModule->setBuddy(m_lineEditModule);
    m_labelModule->setToolTip(moduleToolTip);

    const QString commandsToolTip =
            "<p>" + tr("Debugger commands to be executed when the breakpoint is hit. "
                       "This feature is only available for GDB.") + "</p>";
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
        data->address = m_lineEditAddress->text().toULongLong(nullptr, 0);
    if (partsMask & ExpressionPart)
        data->expression = m_lineEditExpression->text();

    if (partsMask & ConditionPart)
        data->condition = m_lineEditCondition->text();
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
            m_lineEditAddress->setText(QString("0x%1").arg(data.address, 0, 16));
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
        m_lineEditCondition->setText(data.condition);
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
        m_lineEditFunction->setText("main"); // Just for display
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
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::BreakHandler)

public:
    MultiBreakPointsDialog(unsigned int enabledParts, QWidget *parent);

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

MultiBreakPointsDialog::MultiBreakPointsDialog(unsigned int enabledParts, QWidget *parent) :
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
    if (enabledParts & ConditionPart)
        formLayout->addRow(tr("&Condition:"), m_lineEditCondition);
    formLayout->addRow(tr("&Ignore count:"), m_spinBoxIgnoreCount);
    formLayout->addRow(tr("&Thread specification:"), m_lineEditThreadSpec);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

BreakHandler::BreakHandler(DebuggerEngine *engine)
  : m_engine(engine)
{
#if USE_BREAK_MODEL_TEST
    new ModelTest(this, 0);
#endif
    setHeader({tr("Number"), tr("Function"), tr("File"), tr("Line"), tr("Address"),
               tr("Condition"), tr("Ignore"), tr("Threads")});
}

static inline bool fileNameMatch(const QString &f1, const QString &f2)
{
    if (HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive)
        return f1.compare(f2, Qt::CaseInsensitive) == 0;
    return f1 == f2;
}

bool BreakpointParameters::isLocatedAt(const QString &file, int line, const QString &markerFile) const
{
    return lineNumber == line
            && (fileNameMatch(fileName, file) || fileNameMatch(fileName, markerFile));
}

static bool isSimilarTo(const BreakpointParameters &params, const BreakpointParameters &needle)
{
    // Clear miss.
    if (needle.type != UnknownBreakpointType && params.type != UnknownBreakpointType
            && params.type != needle.type)
        return false;

    // Clear hit.
    if (params.address && params.address == needle.address)
        return true;

    // Clear hit.
    if (params == needle)
        return true;

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!params.fileName.isEmpty()
            && fileNameMatch(params.fileName, needle.fileName)
            && params.lineNumber == needle.lineNumber)
        return true;

    return false;
}

Breakpoint BreakHandler::findBreakpointByResponseId(const QString &id) const
{
    return findItemAtLevel<1>([id](const Breakpoint bp) {
        return bp && bp->responseId() == id;
    });
}

SubBreakpoint BreakHandler::findSubBreakpointByResponseId(const QString &id) const
{
    return findItemAtLevel<2>([id](const SubBreakpoint sub) {
        return sub && sub->responseId == id;
    });
}

QVariant BreakHandler::data(const QModelIndex &idx, int role) const
{
    if (role == BaseTreeView::ItemDelegateRole)
        return QVariant::fromValue(new LeftElideDelegate);

    return BreakHandlerModel::data(idx, role);
}

Breakpoint BreakHandler::findWatchpoint(const BreakpointParameters &params) const
{
    return findItemAtLevel<1>([params](const Breakpoint &bp) {
        return bp->m_parameters.isWatchpoint()
                && bp->m_parameters.address == params.address
                && bp->m_parameters.size == params.size
                && bp->m_parameters.expression == params.expression
                && bp->m_parameters.bitpos == params.bitpos;
    });
}

Breakpoint BreakHandler::findBreakpointByIndex(const QModelIndex &index) const
{
    return itemForIndexAtLevel<1>(index);
}

SubBreakpoint BreakHandler::findSubBreakpointByIndex(const QModelIndex &index) const
{
    return itemForIndexAtLevel<2>(index);
}

Breakpoint BreakHandler::findBreakpointByModelId(int modelId) const
{
    return findItemAtLevel<1>([modelId](const Breakpoint &bp) {
        QTC_ASSERT(bp, return false);
        return bp->modelId() == modelId;
    });
}

Breakpoints BreakHandler::findBreakpointsByIndex(const QList<QModelIndex> &list) const
{
    QSet<Breakpoint> items;
    for (const QModelIndex &index : list) {
        if (Breakpoint bp = findBreakpointByIndex(index))
            items.insert(bp);
    }
    return items.toList();
}

SubBreakpoints BreakHandler::findSubBreakpointsByIndex(const QList<QModelIndex> &list) const
{
    QSet<SubBreakpoint> items;
    for (const QModelIndex &index : list) {
        if (SubBreakpoint sbp = findSubBreakpointByIndex(index))
            items.insert(sbp);
    }
    return items.toList();

}

QString BreakHandler::displayFromThreadSpec(int spec)
{
    return spec == -1 ? BreakHandler::tr("(all)") : QString::number(spec);
}

int BreakHandler::threadSpecFromDisplay(const QString &str)
{
    bool ok = false;
    int result = str.toInt(&ok);
    return ok ? result : -1;
}

const QString empty("-");

QVariant BreakpointItem::data(int column, int role) const
{
    if (role == Qt::ForegroundRole) {
        static const QVariant gray(QColor(140, 140, 140));
        switch (m_state) {
            case BreakpointInsertionRequested:
            case BreakpointInsertionProceeding:
            case BreakpointUpdateRequested:
            case BreakpointUpdateProceeding:
            case BreakpointRemoveRequested:
            case BreakpointRemoveProceeding:
                return gray;
            case BreakpointInserted:
            case BreakpointNew:
            case BreakpointDead:
                break;
        };
    }

    switch (column) {
        case BreakpointNumberColumn:
            if (role == Qt::DisplayRole)
                return m_displayName.isEmpty() ? m_responseId : m_displayName;
            if (role == Qt::DecorationRole)
                return icon();
            break;
        case BreakpointFunctionColumn:
            if (role == Qt::DisplayRole) {
                if (!m_parameters.functionName.isEmpty())
                    return simplifyType(m_parameters.functionName);
                if (m_parameters.type == BreakpointAtMain
                        || m_parameters.type == BreakpointAtThrow
                        || m_parameters.type == BreakpointAtCatch
                        || m_parameters.type == BreakpointAtFork
                        || m_parameters.type == BreakpointAtExec
                        //|| m_response.type == BreakpointAtVFork
                        || m_parameters.type == BreakpointAtSysCall)
                    return typeToString(m_parameters.type);
                if (m_parameters.type == WatchpointAtAddress) {
                    quint64 address = m_parameters.address ? m_parameters.address : m_parameters.address;
                    return BreakHandler::tr("Data at 0x%1").arg(address, 0, 16);
                }
                if (m_parameters.type == WatchpointAtExpression) {
                    QString expression = !m_parameters.expression.isEmpty()
                            ? m_parameters.expression : m_parameters.expression;
                    return BreakHandler::tr("Data at %1").arg(expression);
                }
                return empty;
            }
            break;
        case BreakpointFileColumn:
            if (role == Qt::DisplayRole) {
                const QString str = markerFileName();
                if (!str.isEmpty())
                    return QDir::toNativeSeparators(str);
                return empty;
            }
            break;
        case BreakpointLineColumn:
            if (role == Qt::DisplayRole) {
                const int line = markerLineNumber();
                if (line > 0)
                    return line;
                return empty;
            }
            if (role == Qt::UserRole + 1)
                return m_parameters.lineNumber;
            break;
        case BreakpointAddressColumn:
            if (role == Qt::DisplayRole) {
                if (m_parameters.address)
                    return QString("0x%1").arg(m_parameters.address, 0, 16);
                return QVariant();
            }
            break;
        case BreakpointConditionColumn:
            if (role == Qt::DisplayRole)
                return m_parameters.condition;
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit if this condition is met.");
            if (role == Qt::UserRole + 1)
                return m_parameters.condition;
            break;
        case BreakpointIgnoreColumn:
            if (role == Qt::DisplayRole) {
                const int ignoreCount = m_parameters.ignoreCount;
                return ignoreCount ? QVariant(ignoreCount) : QVariant(QString());
            }
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit after being ignored so many times.");
            if (role == Qt::UserRole + 1)
                return m_parameters.ignoreCount;
            break;
        case BreakpointThreadsColumn:
            if (role == Qt::DisplayRole)
                return BreakHandler::displayFromThreadSpec(m_parameters.threadSpec);
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit in the specified thread(s).");
            if (role == Qt::UserRole + 1)
                return BreakHandler::displayFromThreadSpec(m_parameters.threadSpec);
            break;
    }

    if (role == Qt::ToolTipRole && boolSetting(UseToolTipsInBreakpointsView))
        return toolTip();

    return QVariant();
}

void BreakpointItem::addToCommand(DebuggerCommand *cmd) const
{
    QTC_ASSERT(m_globalBreakpoint, return);
    const BreakpointParameters &requested = requestedParameters();
    cmd->arg("modelid", modelId());
    cmd->arg("id", m_responseId);
    cmd->arg("type", requested.type);
    cmd->arg("ignorecount", requested.ignoreCount);
    cmd->arg("condition", toHex(requested.condition));
    cmd->arg("command", toHex(requested.command));
    cmd->arg("function", requested.functionName);
    cmd->arg("oneshot", requested.oneShot);
    cmd->arg("enabled", requested.enabled);
    cmd->arg("file", requested.fileName);
    cmd->arg("line", requested.lineNumber);
    cmd->arg("address", requested.address);
    cmd->arg("expression", requested.expression);
}

void BreakpointItem::updateFromGdbOutput(const GdbMi &bkpt)
{
    m_parameters.updateFromGdbOutput(bkpt);
    adjustMarker();
}

int BreakpointItem::modelId() const
{
    return m_globalBreakpoint ? m_globalBreakpoint->modelId() : 0;
}

void BreakpointItem::setPending(bool pending)
{
    m_parameters.pending = pending;
    adjustMarker();
}

void BreakHandler::removeAlienBreakpoint(const QString &rid)
{
    Breakpoint bp = findBreakpointByResponseId(rid);
    destroyItem(bp);
}

void BreakHandler::requestBreakpointInsertion(const Breakpoint &bp)
{
    bp->gotoState(BreakpointInsertionRequested, BreakpointNew);
    QTimer::singleShot(0, m_engine, [this, bp] { m_engine->insertBreakpoint(bp); });
}

void BreakHandler::requestBreakpointUpdate(const Breakpoint &bp)
{
    bp->gotoState(BreakpointUpdateRequested, BreakpointInserted);
    QTimer::singleShot(0, m_engine, [this, bp] { m_engine->updateBreakpoint(bp); });
}

void BreakHandler::requestBreakpointRemoval(const Breakpoint &bp)
{
    bp->gotoState(BreakpointRemoveRequested, BreakpointInserted);
    QTimer::singleShot(0, m_engine, [this, bp] { m_engine->removeBreakpoint(bp); });
}

void BreakHandler::requestBreakpointEnabling(const Breakpoint &bp, bool enabled)
{
    if (bp->m_parameters.enabled != enabled) {
        bp->updateMarkerIcon();
        bp->update();
        requestBreakpointUpdate(bp);
    }
}

void BreakHandler::requestSubBreakpointEnabling(const SubBreakpoint &sbp, bool enabled)
{
    if (sbp->params.enabled != enabled) {
        sbp->params.enabled = enabled;
        sbp->breakpoint()->update();
        QTimer::singleShot(0, m_engine, [this, sbp, enabled] {
            m_engine->enableSubBreakpoint(sbp, enabled);
        });
    }
}

void BreakpointItem::setMarkerFileAndLine(const QString &fileName, int lineNumber)
{
    if (m_parameters.fileName == fileName && m_parameters.lineNumber == lineNumber)
        return;
    m_parameters.fileName = fileName;
    m_parameters.lineNumber = lineNumber;
    destroyMarker();
    updateMarker();
    update();
}

static bool isAllowedTransition(BreakpointState from, BreakpointState to)
{
    switch (from) {
    case BreakpointNew:
        return to == BreakpointInsertionRequested
            || to == BreakpointDead;
    case BreakpointInsertionRequested:
        return to == BreakpointInsertionProceeding;
    case BreakpointInsertionProceeding:
        return to == BreakpointInserted
            || to == BreakpointDead
            || to == BreakpointUpdateRequested
            || to == BreakpointRemoveRequested;
    case BreakpointUpdateRequested:
        return to == BreakpointUpdateProceeding;
    case BreakpointUpdateProceeding:
        return to == BreakpointInserted
            || to == BreakpointDead;
    case BreakpointInserted:
        return to == BreakpointUpdateRequested
            || to == BreakpointRemoveRequested;
    case BreakpointRemoveRequested:
        return to == BreakpointRemoveProceeding;
    case BreakpointRemoveProceeding:
        return to == BreakpointDead;
    case BreakpointDead:
        return false;
    }
    qDebug() << "UNKNOWN BREAKPOINT STATE:" << from;
    return false;
}

void BreakpointItem::gotoState(BreakpointState target, BreakpointState assumedCurrent)
{
    QTC_ASSERT(m_state == assumedCurrent, qDebug() << m_state);
    setState(target);
}

void BreakHandler::updateDisassemblerMarker(const Breakpoint &bp)
{
    return m_engine->disassemblerAgent()->updateBreakpointMarker(bp);
}

void BreakHandler::removeDisassemblerMarker(const Breakpoint &bp)
{
    m_engine->disassemblerAgent()->removeBreakpointMarker(bp);
    bp->destroyMarker();
    if (GlobalBreakpoint gbp = bp->globalBreakpoint())
        gbp->updateMarker();
}

void BreakpointItem::setState(BreakpointState state)
{
    //qDebug() << "BREAKPOINT STATE TRANSITION, ID: " << m_id
    //    << " FROM: " << state << " TO: " << state;
    if (!isAllowedTransition(m_state, state)) {
        qDebug() << "UNEXPECTED BREAKPOINT STATE TRANSITION" << m_state << state;
        QTC_CHECK(false);
    }

    if (m_state == state) {
        qDebug() << "STATE UNCHANGED: " << responseId() << m_state;
        return;
    }

    m_state = state;

    // FIXME: updateMarker() should recognize the need for icon changes.
    if (state == BreakpointInserted) {
        destroyMarker();
        updateMarker();
    }
    update();
}

const GlobalBreakpoint BreakpointItem::globalBreakpoint() const
{
    return m_globalBreakpoint;
}

void BreakpointItem::setParameters(const BreakpointParameters &value)
{
    m_parameters = value;
    adjustMarker();
}

void BreakpointItem::setEnabled(bool on)
{
    m_parameters.enabled = on;
    adjustMarker();
}

void DebuggerEngine::notifyBreakpointInsertProceeding(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    bp->gotoState(BreakpointInsertionProceeding, BreakpointInsertionRequested);
}

void DebuggerEngine::notifyBreakpointInsertOk(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    bp->adjustMarker();
    bp->gotoState(BreakpointInserted, BreakpointInsertionProceeding);
    breakHandler()->updateDisassemblerMarker(bp);
    bp->updateMarker();
}

void DebuggerEngine::notifyBreakpointInsertFailed(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    bp->gotoState(BreakpointDead, BreakpointInsertionProceeding);
}

void DebuggerEngine::notifyBreakpointRemoveProceeding(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    bp->gotoState(BreakpointRemoveProceeding, BreakpointRemoveRequested);
}

void DebuggerEngine::notifyBreakpointRemoveOk(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_ASSERT(bp->state() == BreakpointRemoveProceeding, qDebug() << bp->state());
    breakHandler()->removeDisassemblerMarker(bp);
    breakHandler()->destroyItem(bp);
}

void DebuggerEngine::notifyBreakpointRemoveFailed(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_ASSERT(bp->m_state == BreakpointRemoveProceeding, qDebug() << bp->m_state);
    breakHandler()->removeDisassemblerMarker(bp);
    breakHandler()->destroyItem(bp);
}

void DebuggerEngine::notifyBreakpointChangeProceeding(const Breakpoint &bp)
{
    bp->gotoState(BreakpointUpdateProceeding, BreakpointUpdateRequested);
}

void DebuggerEngine::notifyBreakpointChangeOk(const Breakpoint &bp)
{
    bp->gotoState(BreakpointInserted, BreakpointUpdateProceeding);
}

void DebuggerEngine::notifyBreakpointChangeFailed(const Breakpoint &bp)
{
    bp->gotoState(BreakpointDead, BreakpointUpdateProceeding);
}

void DebuggerEngine::notifyBreakpointNeedsReinsertion(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    QTC_ASSERT(bp->m_state == BreakpointUpdateProceeding, qDebug() << bp->m_state);
    bp->m_state = BreakpointInsertionRequested;
}

void BreakHandler::handleAlienBreakpoint(const QString &responseId, const BreakpointParameters &params)
{
    // Search a breakpoint we might refer to.
    Breakpoint bp = findItemAtLevel<1>([params, responseId](const Breakpoint &bp) {
        if (bp && !bp->responseId().isEmpty() && bp->responseId() == responseId)
            return true;
        return bp && isSimilarTo(bp->m_parameters, params);
    });

    if (bp) {
        // FIXME: x.y looks rather gdb specific.
        if (bp->responseId().contains('.')) {
            SubBreakpoint loc = bp->findOrCreateSubBreakpoint(bp->responseId());
            QTC_ASSERT(loc, return);
            loc->setParameters(params);
        } else {
            bp->setParameters(params);
        }
    } else {
        bp = new BreakpointItem(nullptr);
        bp->m_responseId = responseId;
        bp->m_parameters = params;
        bp->m_state = BreakpointInserted;
        bp->updateMarker();
        rootItem()->appendChild(bp);
        // This has no global breakpoint, so there's nothing to update here.
    }
}

SubBreakpoint BreakpointItem::findOrCreateSubBreakpoint(const QString &responseId)
{
    SubBreakpoint loc = findFirstLevelChild([&](const SubBreakpoint &l) {
        return l->responseId == responseId;
    });
    if (loc) {
        // This modifies an existing sub-breakpoint.
        loc->update();
    } else {
        // This is a new sub-breakpoint.
        loc = new SubBreakpointItem;
        loc->responseId = responseId;
        appendChild(loc);
        expand();
    }
    return loc;
}

bool BreakHandler::tryClaimBreakpoint(const GlobalBreakpoint &gbp)
{
    const Breakpoints bps = breakpoints();
    if (Utils::anyOf(bps, [gbp](const Breakpoint &bp) { return bp->globalBreakpoint() == gbp; }))
        return false;

    if (!m_engine->acceptsBreakpoint(gbp->requestedParameters())) {
        m_engine->showMessage(QString("BREAKPOINT %1 IS NOT ACCEPTED BY ENGINE %2")
                    .arg(gbp->displayName()).arg(objectName()));
        return false;
    }

    m_engine->showMessage(QString("TAKING OWNERSHIP OF BREAKPOINT %1").arg(gbp->displayName()));

    Breakpoint bp(new BreakpointItem(gbp));
    rootItem()->appendChild(bp);

    gbp->updateMarker();
    requestBreakpointInsertion(bp);

    return true;
}

void BreakHandler::gotoLocation(const Breakpoint &bp) const
{
    QTC_ASSERT(bp, return);
    QTC_ASSERT(m_engine, return);
    if (bp->m_parameters.type == BreakpointByAddress) {
        m_engine->gotoLocation(bp->m_parameters.address);
    } else {
        // Don't use gotoLocation unconditionally as this ends up in
        // disassembly if OperateByInstruction is on. But fallback
        // to disassembly if we can't open the file.
        const QString file = QDir::cleanPath(bp->markerFileName());
        if (IEditor *editor = EditorManager::openEditor(file))
            editor->gotoLine(bp->markerLineNumber(), 0);
        else
            m_engine->openDisassemblerView(Location(bp->m_parameters.address));
    }
}

const Breakpoints BreakHandler::breakpoints() const
{
    QList<Breakpoint> items;
    forItemsAtLevel<1>([&items](Breakpoint bp) { if (bp) items.append(bp); });
    return items;
}

void BreakpointItem::adjustMarker()
{
    destroyMarker();
    updateMarker();
}

void BreakpointItem::deleteBreakpoint()
{
    QTC_ASSERT(!globalBreakpoint(), return); // Use deleteBreakpoint(GlobalBreakpoint gbp) instead.

    for (QPointer<DebuggerEngine> engine : EngineManager::engines())
        engine->breakHandler()->requestBreakpointRemoval(this);
}

void BreakpointItem::deleteGlobalOrThisBreakpoint()
{
    if (GlobalBreakpoint gbp = globalBreakpoint()) {
        gbp->deleteBreakpoint();
    } else {
        deleteBreakpoint();
    }
}

bool BreakHandler::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == BaseTreeView::ItemActivatedRole) {
        if (Breakpoint bp = findBreakpointByIndex(idx))
            gotoLocation(bp);
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = value.value<ItemViewEvent>();

        if (ev.as<QContextMenuEvent>())
            return contextMenuEvent(ev);

        if (auto kev = ev.as<QKeyEvent>(QEvent::KeyPress)) {
            if (kev->key() == Qt::Key_Delete) {
                QModelIndexList si = ev.currentOrSelectedRows();
                const Breakpoints bps = findBreakpointsByIndex(si);
                for (Breakpoint bp : bps) {
                    if (GlobalBreakpoint gbp = bp->globalBreakpoint())
                        gbp->deleteBreakpoint();
                    else
                        bp->deleteBreakpoint();
                }
//                int row = qMin(rowCount() - ids.size() - 1, idx.row());
//                setCurrentIndex(index(row, 0));   FIXME
                return true;
            }
            if (kev->key() == Qt::Key_Space) {
                const QModelIndexList selectedIds = ev.selectedRows();
                if (!selectedIds.isEmpty()) {
                    const Breakpoints bps = findBreakpointsByIndex(selectedIds);
                    const SubBreakpoints sbps = findSubBreakpointsByIndex(selectedIds);
                    const bool isEnabled = (bps.isEmpty() && sbps.isEmpty())
                            || (!bps.isEmpty() && bps.at(0)->isEnabled())
                            || (!sbps.isEmpty() && sbps.at(0)->params.enabled);
                    for (Breakpoint bp : bps) {
                        requestBreakpointEnabling(bp, !isEnabled);
                        if (GlobalBreakpoint gbp = bp->globalBreakpoint())
                            gbp->setEnabled(!isEnabled, false);
                    }
                    for (SubBreakpoint sbp : sbps)
                        requestSubBreakpointEnabling(sbp, !isEnabled);
                    return true;
                }
            }
        }

        if (ev.as<QMouseEvent>(QEvent::MouseButtonDblClick)) {
            if (Breakpoint bp = findBreakpointByIndex(idx)) {
                if (idx.column() >= BreakpointAddressColumn)
                    editBreakpoints({bp}, ev.view());
                else
                    gotoLocation(bp);
            } else if (SubBreakpoint loc = itemForIndexAtLevel<2>(idx)) {
                gotoLocation(loc->breakpoint());
            } else {
                BreakpointManager::executeAddBreakpointDialog();
            }
            return true;
        }
    }

    return false;
}

bool BreakHandler::contextMenuEvent(const ItemViewEvent &ev)
{
    const QModelIndexList selectedIndices = ev.selectedRows();

    const Breakpoints selectedBreakpoints = findBreakpointsByIndex(selectedIndices);
    const bool breakpointsEnabled = selectedBreakpoints.isEmpty() || selectedBreakpoints.at(0)->isEnabled();

    QList<SubBreakpointItem *> selectedLocations;
    bool handlesIndividualLocations = false;
    for (const QModelIndex &index : selectedIndices) {
        if (SubBreakpointItem *location = itemForIndexAtLevel<2>(index)) {
            if (selectedLocations.contains(location))
                continue;
            selectedLocations.append(location);
            if (m_engine->hasCapability(BreakIndividualLocationsCapability))
                handlesIndividualLocations = true;
        }
    }
    const bool locationsEnabled = selectedLocations.isEmpty() || selectedLocations.at(0)->params.enabled;

    auto menu = new QMenu;

    addAction(menu, tr("Add Breakpoint..."), true, &BreakpointManager::executeAddBreakpointDialog);

    addAction(menu, tr("Delete Selected Breakpoints"),
              !selectedBreakpoints.isEmpty(),
              [selectedBreakpoints] {
                for (Breakpoint bp : selectedBreakpoints) {
                    if (GlobalBreakpoint gbp = bp->globalBreakpoint()) {
                        gbp->deleteBreakpoint();
                    } else {
                        bp->deleteBreakpoint();
                    }
                }
             });

    addAction(menu, tr("Edit Selected Breakpoints..."),
              !selectedBreakpoints.isEmpty(),
              [this, selectedBreakpoints, ev] { editBreakpoints(selectedBreakpoints, ev.view()); });


    // FIXME BP: m_engine->threadsHandler()->currentThreadId();
    // int threadId = 0;
    // addAction(menu,
    //           threadId == -1 ? tr("Associate Breakpoint with All Threads")
    //                          : tr("Associate Breakpoint with Thread %1").arg(threadId),
    //           !selectedItems.isEmpty(),
    //           [this, selectedItems, threadId] {
    //                 for (Breakpoint bp : selectedItems)
    //                     bp.setThreadSpec(threadId);
    //           });

    addAction(menu,
              selectedBreakpoints.size() > 1
                  ? breakpointsEnabled ? tr("Disable Selected Breakpoints") : tr("Enable Selected Breakpoints")
                  : breakpointsEnabled ? tr("Disable Breakpoint") : tr("Enable Breakpoint"),
              !selectedBreakpoints.isEmpty(),
              [this, selectedBreakpoints, breakpointsEnabled] {
                    for (Breakpoint bp : selectedBreakpoints) {
                        requestBreakpointEnabling(bp, !breakpointsEnabled);
                        if (GlobalBreakpoint gbp = bp->globalBreakpoint())
                            gbp->setEnabled(!breakpointsEnabled, false);
                    }
              }
    );

    addAction(menu,
              selectedLocations.size() > 1
                  ? locationsEnabled ? tr("Disable Selected Locations") : tr("Enable Selected Locations")
                  : locationsEnabled ? tr("Disable Location") : tr("Enable Location"),
              !selectedLocations.isEmpty() && handlesIndividualLocations,
              [this, selectedLocations, locationsEnabled] {
                   for (const SubBreakpoint &sbp : selectedLocations)
                       requestSubBreakpointEnabling(sbp, !locationsEnabled);
              }
    );

    menu->addSeparator();

    addAction(menu, tr("Delete All Breakpoints"),
              rowCount() > 0,
              &BreakpointManager::executeDeleteAllBreakpointsDialog);

    // Delete by file: Find indices of breakpoints of the same file.
    QList<Breakpoint> breakpointsInFile;
    QString file;
    if (Breakpoint bp = itemForIndexAtLevel<1>(ev.sourceModelIndex())) {
        const QModelIndex index = ev.sourceModelIndex().sibling(ev.sourceModelIndex().row(), BreakpointFileColumn);
        if (!file.isEmpty()) {
            for (int i = 0; i != rowCount(); ++i)
                if (index.data().toString() == file)
                    breakpointsInFile.append(findBreakpointByIndex(index));
        }
    }
    addAction(menu, tr("Delete Breakpoints of \"%1\"").arg(file),
              tr("Delete Breakpoints of File"),
              breakpointsInFile.size() > 1,
              [breakpointsInFile] {
                for (Breakpoint bp : breakpointsInFile)
                    bp->deleteGlobalOrThisBreakpoint();
              });

    menu->addSeparator();

    menu->addAction(action(UseToolTipsInBreakpointsView));
    menu->addAction(action(UseAddressInBreakpointsView));
    menu->addSeparator();
    menu->addAction(action(SettingsDialog));

    menu->popup(ev.globalPos());

    return true;
}

void BreakHandler::removeBreakpoint(const Breakpoint &bp)
{
    QTC_ASSERT(bp, return);
    switch (bp->m_state) {
    case BreakpointRemoveRequested:
        break;
    case BreakpointInserted:
    case BreakpointInsertionProceeding:
        requestBreakpointRemoval(bp);
        break;
    case BreakpointNew:
        bp->setState(BreakpointDead);
        bp->destroyMarker();
        destroyItem(bp);
        break;
    default:
        qWarning("Warning: Cannot remove breakpoint %s in state '%s'.",
               qPrintable(bp->responseId()), qPrintable(stateToString(bp->m_state)));
        bp->m_state = BreakpointRemoveRequested;
        break;
    }
}

static unsigned int engineBreakpointCapabilities(DebuggerEngine *engine)
{
    unsigned int enabledParts = ~0;
    if (!engine->hasCapability(BreakConditionCapability))
        enabledParts &= ~ConditionPart;
    if (!engine->hasCapability(BreakModuleCapability))
        enabledParts &= ~ModulePart;
    if (!engine->hasCapability(TracePointCapability))
        enabledParts &= ~TracePointPart;
    return enabledParts;
}

void BreakHandler::editBreakpoint(const Breakpoint &bp, QWidget *parent)
{
    QTC_ASSERT(bp, return);
    BreakpointParameters params = bp->requestedParameters();
    BreakpointParts parts = NoParts;

    BreakpointDialog dialog(engineBreakpointCapabilities(m_engine), parent);
    if (!dialog.showDialog(&params, &parts))
        return;

    if (params != bp->requestedParameters()) {
        bp->setParameters(params);
        updateDisassemblerMarker(bp);
        bp->updateMarker();
        bp->update();
        if (bp->needsChange() && bp->m_state != BreakpointNew)
            requestBreakpointUpdate(bp);
    }
}

void BreakHandler::editBreakpoints(const Breakpoints &bps, QWidget *parent)
{
    QTC_ASSERT(!bps.isEmpty(), return);

    Breakpoint bp = bps.at(0);

    if (bps.size() == 1) {
        editBreakpoint(bp, parent);
        return;
    }

    // This allows to change properties of multiple breakpoints at a time.
    QTC_ASSERT(bp, return);

    MultiBreakPointsDialog dialog(engineBreakpointCapabilities(m_engine), parent);
    dialog.setCondition(bp->condition());
    dialog.setIgnoreCount(bp->ignoreCount());
    dialog.setThreadSpec(bp->threadSpec());

    if (dialog.exec() == QDialog::Rejected)
        return;

    const QString newCondition = dialog.condition();
    const int newIgnoreCount = dialog.ignoreCount();
    const int newThreadSpec = dialog.threadSpec();

    for (Breakpoint bp : bps) {
        if (bp) {
            bp->m_parameters.condition = newCondition;
            bp->m_parameters.ignoreCount = newIgnoreCount;
            bp->m_parameters.threadSpec = newThreadSpec;
            if (bp->m_state != BreakpointNew)
                requestBreakpointUpdate(bp);
        }
    }
}

//////////////////////////////////////////////////////////////////
//
// Storage
//
//////////////////////////////////////////////////////////////////

BreakpointItem::BreakpointItem(const GlobalBreakpoint &gbp)
    : m_globalBreakpoint(gbp)
{
}

BreakpointItem::~BreakpointItem()
{
    delete m_marker;
}

void BreakpointItem::destroyMarker()
{
    if (m_marker) {
        BreakpointMarker *m = m_marker;
        m_marker = nullptr;
        delete m;
    }
}

QString BreakpointItem::markerFileName() const
{
    // Some heuristics to find a "good" file name.
    if (!m_parameters.fileName.isEmpty()) {
        QFileInfo fi(m_parameters.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    const QString origFileName = requestedParameters().fileName;
    if (m_parameters.fileName.endsWith(origFileName))
        return m_parameters.fileName;
    if (origFileName.endsWith(m_parameters.fileName))
        return origFileName;
    return m_parameters.fileName.size() > origFileName ? m_parameters.fileName : origFileName;
}

int BreakpointItem::markerLineNumber() const
{
    if (m_parameters.lineNumber > 0)
        return m_parameters.lineNumber;
    return requestedParameters().lineNumber;
}

const BreakpointParameters &BreakpointItem::requestedParameters() const
{
    return m_globalBreakpoint ? m_globalBreakpoint->requestedParameters() : m_alienParameters;
}

static void formatAddress(QTextStream &str, quint64 address)
{
    if (address) {
        str << "0x";
        str.setIntegerBase(16);
        str << address;
        str.setIntegerBase(10);
    }
}

bool BreakpointItem::needsChange() const
{
    const BreakpointParameters &oparams = requestedParameters();
    if (!oparams.conditionsMatch(m_parameters.condition))
        return true;
    if (oparams.ignoreCount != m_parameters.ignoreCount)
        return true;
    if (oparams.enabled != m_parameters.enabled)
        return true;
    if (oparams.threadSpec != m_parameters.threadSpec)
        return true;
    if (oparams.command != m_parameters.command)
        return true;
    if (oparams.type == BreakpointByFileAndLine && oparams.lineNumber != m_parameters.lineNumber)
        return true;
    // FIXME: Too strict, functions may have parameter lists, or not.
    // if (m_params.type == BreakpointByFunction && m_params.functionName != m_response.functionName)
    //     return true;
    // if (m_params.type == BreakpointByAddress && m_params.address != m_response.address)
    //     return true;
    return false;
}

void BreakpointItem::updateMarkerIcon()
{
    if (m_marker) {
        m_marker->setIcon(icon());
        m_marker->updateMarker();
    }
}

void BreakpointItem::updateMarker()
{
    FileName file = FileName::fromString(markerFileName());
    int line = markerLineNumber();
    if (m_marker && (file != m_marker->fileName() || line != m_marker->lineNumber()))
        destroyMarker();

    if (!m_marker && !file.isEmpty() && line > 0)
        m_marker = new BreakpointMarker(this, file, line);

    if (m_marker)
        m_marker->setToolTip(toolTip());
}

QIcon BreakpointItem::icon() const
{
    // FIXME: This seems to be called on each cursor blink as soon as the
    // cursor is near a line with a breakpoint marker (+/- 2 lines or so).
    if (m_parameters.isTracepoint())
        return Icons::TRACEPOINT.icon();
    if (m_parameters.type == WatchpointAtAddress)
        return Icons::WATCHPOINT.icon();
    if (m_parameters.type == WatchpointAtExpression)
        return Icons::WATCHPOINT.icon();
    if (!m_parameters.enabled)
        return Icons::BREAKPOINT_DISABLED.icon();
    if (m_state == BreakpointInserted && !m_parameters.pending)
        return Icons::BREAKPOINT.icon();
    return Icons::BREAKPOINT_PENDING.icon();
}

QString BreakpointItem::toolTip() const
{
    const BreakpointParameters &requested = requestedParameters();
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><b>" << tr("Breakpoint") << "</b>"
        << "<table>"
        << "<tr><td>" << tr("Internal ID:")
        << "</td><td>" << m_responseId << "</td></tr>"
        << "<tr><td>" << tr("State:")
        << "</td><td>" << (requestedParameters().enabled ? tr("Enabled") : tr("Disabled"));
    if (m_parameters.pending)
        str << ", " << tr("pending");
    str << ", " << stateToString(m_state) << "</td></tr>";
    str << "<tr><td>" << tr("Breakpoint Type:")
        << "</td><td>" << typeToString(requested.type) << "</td></tr>"
        << "<tr><td>" << tr("Marker File:")
        << "</td><td>" << QDir::toNativeSeparators(markerFileName()) << "</td></tr>"
        << "<tr><td>" << tr("Marker Line:")
        << "</td><td>" << markerLineNumber() << "</td></tr>"
        << "<tr><td>" << tr("Hit Count:")
        << "</td><td>" << m_parameters.hitCount << "</td></tr>"
        << "</table><br><table>"
        << "<tr><th>" << tr("Property")
        << "</th><th>" << tr("Requested")
        << "</th><th>" << tr("Obtained") << "</th></tr>";
    if (!m_displayName.isEmpty()) {
        str << "<tr><td>" << tr("Display Name:")
            << "</td><td>&mdash;</td><td>" << m_displayName << "</td></tr>";
    }
    if (m_parameters.type == BreakpointByFunction) {
        str << "<tr><td>" << tr("Function Name:")
        << "</td><td>" << requested.functionName
        << "</td><td>" << m_parameters.functionName
        << "</td></tr>";
    }
    if (m_parameters.type == BreakpointByFileAndLine) {
        str << "<tr><td>" << tr("File Name:")
            << "</td><td>" << QDir::toNativeSeparators(requested.fileName)
            << "</td><td>" << QDir::toNativeSeparators(m_parameters.fileName)
            << "</td></tr>"
            << "<tr><td>" << tr("Line Number:")
            << "</td><td>" << requested.lineNumber
            << "</td><td>" << m_parameters.lineNumber << "</td></tr>";
    }
    if (requested.type == BreakpointByFunction || m_parameters.type == BreakpointByFileAndLine) {
        str << "<tr><td>" << tr("Module:")
            << "</td><td>" << requested.module
            << "</td><td>" << m_parameters.module
            << "</td></tr>";
    }
    str << "<tr><td>" << tr("Breakpoint Address:")
        << "</td><td>";
    formatAddress(str, requested.address);
    str << "</td><td>";
    formatAddress(str, m_parameters.address);
    str << "</td></tr>";
    if (!requested.command.isEmpty() || !m_parameters.command.isEmpty()) {
        str << "<tr><td>" << tr("Command:")
            << "</td><td>" << requested.command
            << "</td><td>" << m_parameters.command
            << "</td></tr>";
    }
    if (!requested.message.isEmpty() || !m_parameters.message.isEmpty()) {
        str << "<tr><td>" << tr("Message:")
            << "</td><td>" << requested.message
            << "</td><td>" << m_parameters.message
            << "</td></tr>";
    }
    if (!requested.condition.isEmpty() || !m_parameters.condition.isEmpty()) {
        str << "<tr><td>" << tr("Condition:")
            << "</td><td>" << requested.condition
            << "</td><td>" << m_parameters.condition
            << "</td></tr>";
    }
    if (requested.ignoreCount || m_parameters.ignoreCount) {
        str << "<tr><td>" << tr("Ignore Count:") << "</td><td>";
        if (requested.ignoreCount)
            str << m_parameters.ignoreCount;
        str << "</td><td>";
        if (m_parameters.ignoreCount)
            str << m_parameters.ignoreCount;
        str << "</td></tr>";
    }
    if (requested.threadSpec >= 0 || m_parameters.threadSpec >= 0) {
        str << "<tr><td>" << tr("Thread Specification:")
            << "</td><td>";
        if (requested.threadSpec >= 0)
            str << requested.threadSpec;
        str << "</td><td>";
        if (m_parameters.threadSpec >= 0)
            str << m_parameters.threadSpec;
        str << "</td></tr>";
    }
    str  << "</table></body></html>";
    return rc;
}

void BreakHandler::setWatchpointAtAddress(quint64 address, unsigned size)
{
    BreakpointParameters params(WatchpointAtAddress);
    params.address = address;
    params.size = size;
    if (findWatchpoint(params)) {
        qDebug() << "WATCHPOINT EXISTS";
        //   removeBreakpoint(index);
        return;
    }
    BreakpointManager::createBreakpointForEngine(params, m_engine);
}

void BreakHandler::setWatchpointAtExpression(const QString &exp)
{
    BreakpointParameters params(WatchpointAtExpression);
    params.expression = exp;
    if (findWatchpoint(params)) {
        qDebug() << "WATCHPOINT EXISTS";
        //   removeBreakpoint(index);
        return;
    }
    BreakpointManager::createBreakpointForEngine(params, m_engine);
}

void BreakHandler::releaseAllBreakpoints()
{
    GlobalBreakpoints gbps;
    for (Breakpoint bp : breakpoints()) {
        bp->removeChildren();
        bp->destroyMarker();
        gbps.append(bp->globalBreakpoint());
    }
    clear();
    // Make now-unclaimed breakpoints globally visible again.
    for (GlobalBreakpoint gbp: qAsConst(gbps)) {
        if (gbp)
            gbp->updateMarker();
    }
}

QString BreakpointItem::msgWatchpointByExpressionTriggered(const QString &expr) const
{
    return tr("Internal data breakpoint %1 at %2 triggered.")
            .arg(responseId()).arg(expr);
}

QString BreakpointItem::msgWatchpointByExpressionTriggered(const QString &expr,
                                                           const QString &threadId) const
{
    return tr("Internal data breakpoint %1 at %2 in thread %3 triggered.")
            .arg(responseId()).arg(expr).arg(threadId);
}

QString BreakpointItem::msgWatchpointByAddressTriggered(quint64 address) const
{
    return tr("Internal data breakpoint %1 at 0x%2 triggered.")
            .arg(responseId()).arg(address, 0, 16);
}

QString BreakpointItem::msgWatchpointByAddressTriggered(quint64 address,
                                                        const QString &threadId) const
{
    return tr("Internal data breakpoint %1 at 0x%2 in thread %3 triggered.")
            .arg(responseId()).arg(address, 0, 16).arg(threadId);
}

QString BreakpointItem::msgBreakpointTriggered(const QString &threadId) const
{
    return tr("Stopped at breakpoint %1 in thread %2.")
            .arg(responseId()).arg(threadId);
}


QVariant SubBreakpointItem::data(int column, int role) const
{
    if (role == Qt::DecorationRole && column == 0) {
        return params.enabled ? Icons::BREAKPOINT.icon()
                              : Icons::BREAKPOINT_DISABLED.icon();
    }

    if (role == Qt::DisplayRole) {
        switch (column) {
        case BreakpointNumberColumn:
            return displayName.isEmpty() ? responseId : displayName;
        case BreakpointFunctionColumn:
            return params.functionName;
        case BreakpointAddressColumn:
            if (params.address)
                return QString::fromLatin1("0x%1").arg(params.address, 0, 16);
        }
    }
    return QVariant();
}


//
// GlobalBreakpointItem
//

// Ok to be not thread-safe. The order does not matter and only the gui
// produces authoritative ids.
static int currentId = 0;

GlobalBreakpointItem::GlobalBreakpointItem()
    : m_modelId(++currentId)
{
}

GlobalBreakpointItem::~GlobalBreakpointItem()
{}

QVariant GlobalBreakpointItem::data(int column, int role) const
{

    switch (column) {
        case BreakpointNumberColumn:
            if (role == Qt::DisplayRole) {
                if (auto engine = usingEngine())
                    return engine->runParameters().displayName;


                return QString("-");
            }
            if (role == Qt::DecorationRole)
                return icon();
            break;
        case BreakpointFunctionColumn:
            if (role == Qt::DisplayRole) {
                if (!m_params.functionName.isEmpty())
                    return m_params.functionName;
                if (m_params.type == BreakpointAtMain
                        || m_params.type == BreakpointAtThrow
                        || m_params.type == BreakpointAtCatch
                        || m_params.type == BreakpointAtFork
                        || m_params.type == BreakpointAtExec
                        //|| m_params.type == BreakpointAtVFork
                        || m_params.type == BreakpointAtSysCall)
                    return typeToString(m_params.type);
                if (m_params.type == WatchpointAtAddress)
                    return BreakHandler::tr("Data at 0x%1").arg(m_params.address, 0, 16);
                if (m_params.type == WatchpointAtExpression)
                    return BreakHandler::tr("Data at %1").arg(m_params.expression);
                return empty;
            }
            break;
        case BreakpointFileColumn:
            if (role == Qt::DisplayRole) {
                QString str = m_params.fileName;
                if (!str.isEmpty())
                    return QDir::toNativeSeparators(str);
                return empty;
            }
            break;
        case BreakpointLineColumn:
            if (role == Qt::DisplayRole) {
                if (m_params.lineNumber > 0)
                    return m_params.lineNumber;
                return empty;
            }
            if (role == Qt::UserRole + 1)
                return m_params.lineNumber;
            break;
        case BreakpointAddressColumn:
            if (role == Qt::DisplayRole) {
                const quint64 address = m_params.address;
                if (address)
                    return QString("0x%1").arg(address, 0, 16);
                return QVariant();
            }
            break;
        case BreakpointConditionColumn:
            if (role == Qt::DisplayRole)
                return m_params.condition;
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit if this condition is met.");
            if (role == Qt::UserRole + 1)
                return m_params.condition;
            break;
        case BreakpointIgnoreColumn:
            if (role == Qt::DisplayRole) {
                const int ignoreCount = m_params.ignoreCount;
                return ignoreCount ? QVariant(ignoreCount) : QVariant(QString());
            }
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit after being ignored so many times.");
            if (role == Qt::UserRole + 1)
                return m_params.ignoreCount;
            break;
        case BreakpointThreadsColumn:
            if (role == Qt::DisplayRole)
                return BreakHandler::displayFromThreadSpec(m_params.threadSpec);
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit in the specified thread(s).");
            if (role == Qt::UserRole + 1)
                return BreakHandler::displayFromThreadSpec(m_params.threadSpec);
            break;
    }

    if (role == Qt::ToolTipRole && boolSetting(UseToolTipsInBreakpointsView))
        return toolTip();

    return QVariant();
}

QIcon GlobalBreakpointItem::icon() const
{
    // FIXME: This seems to be called on each cursor blink as soon as the
    // cursor is near a line with a breakpoint marker (+/- 2 lines or so).
    if (m_params.isTracepoint())
        return Icons::TRACEPOINT.icon();
    if (m_params.type == WatchpointAtAddress)
        return Icons::WATCHPOINT.icon();
    if (m_params.type == WatchpointAtExpression)
        return Icons::WATCHPOINT.icon();
    if (!m_params.enabled)
        return Icons::BREAKPOINT_DISABLED.icon();

    return Icons::BREAKPOINT_PENDING.icon();
}

QPointer<DebuggerEngine> GlobalBreakpointItem::usingEngine() const
{
    for (QPointer<DebuggerEngine> engine : EngineManager::engines()) {
        for (Breakpoint bp : engine->breakHandler()->breakpoints()) {
            if (bp->globalBreakpoint() == this)
                return engine;
        }
    }
    return nullptr;
}

int GlobalBreakpointItem::modelId() const
{
    return m_modelId;
}

QString GlobalBreakpointItem::displayName() const
{
    return QString::number(m_modelId);
}

void GlobalBreakpointItem::deleteBreakpoint()
{
    for (QPointer<DebuggerEngine> engine : EngineManager::engines()) {
        BreakHandler *handler = engine->breakHandler();
        for (Breakpoint bp : handler->breakpoints()) {
            if (bp->globalBreakpoint() == this)
                handler->removeBreakpoint(bp);
        }
    }
    removeBreakpointFromModel();
}

void GlobalBreakpointItem::removeBreakpointFromModel()
{
    delete m_marker;
    m_marker = nullptr;
    theBreakpointManager->destroyItem(this);
}

QString GlobalBreakpointItem::markerFileName() const
{
    // Some heuristics to find a "good" file name.
    if (!m_params.fileName.isEmpty()) {
        QFileInfo fi(m_params.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    return m_params.fileName;
}

int GlobalBreakpointItem::markerLineNumber() const
{
    return m_params.lineNumber;
}

void GlobalBreakpointItem::updateMarkerIcon()
{
    if (m_marker) {
        m_marker->setIcon(icon());
        m_marker->updateMarker();
    }
}

void GlobalBreakpointItem::updateMarker()
{
    if (usingEngine() != nullptr) {
        // Don't show markers that are claimed by engines.
        // FIXME: Apart, perhaps, when the engine's reported location does not match?
        destroyMarker();
        return;
    }

    const FileName file = FileName::fromString(m_params.fileName);
    const int line = m_params.lineNumber;
    if (m_marker && (file != m_marker->fileName() || line != m_marker->lineNumber()))
        destroyMarker();

    if (!m_marker && !file.isEmpty() && line > 0)
        m_marker = new GlobalBreakpointMarker(this, file, line);

    if (m_marker)
        m_marker->setToolTip(toolTip());
}

void GlobalBreakpointItem::setEnabled(bool enabled, bool descend)
{
    if (m_params.enabled != enabled) {
        m_params.enabled = enabled;
        updateMarkerIcon();
        update();
    }

    if (descend) {
        for (QPointer<DebuggerEngine> engine : EngineManager::engines()) {
            BreakHandler *handler = engine->breakHandler();
            for (Breakpoint bp : handler->breakpoints()) {
                if (bp->globalBreakpoint() == this)
                    handler->requestBreakpointEnabling(bp, enabled);
            }
        }
    }
}

void GlobalBreakpointItem::destroyMarker()
{
    delete m_marker;
    m_marker = nullptr;
}

QString GlobalBreakpointItem::toolTip() const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><b>" << BreakpointItem::tr("Unclaimed Breakpoint") << "</b>"
        << "<table>"
        //<< "<tr><td>" << tr("ID:") << "</td><td>" << m_id << "</td></tr>"
        << "<tr><td>" << BreakpointItem::tr("State:")
        << "</td><td>" << (m_params.enabled ? BreakpointItem::tr("Enabled") : BreakpointItem::tr("Disabled"))
        << "<tr><td>" << BreakpointItem::tr("Breakpoint Type:")
        << "</td><td>" << typeToString(m_params.type) << "</td></tr>";
    if (m_params.type == BreakpointByFunction) {
        str << "<tr><td>" << BreakpointItem::tr("Function Name:")
        << "</td><td>" << m_params.functionName
        << "</td></tr>";
    }
    if (m_params.type == BreakpointByFileAndLine) {
        str << "<tr><td>" << BreakpointItem::tr("File Name:")
            << "</td><td>" << QDir::toNativeSeparators(m_params.fileName)
            << "</td></tr>"
            << "<tr><td>" << BreakpointItem::tr("Line Number:")
            << "</td><td>" << m_params.lineNumber;
    }
    if (m_params.type == BreakpointByFunction || m_params.type == BreakpointByFileAndLine) {
        str << "<tr><td>" << BreakpointItem::tr("Module:")
            << "</td><td>" << m_params.module
            << "</td></tr>";
    }
    str << "<tr><td>" << BreakpointItem::tr("Breakpoint Address:") << "</td><td>";
    formatAddress(str, m_params.address);
    str << "</td></tr>";
    if (!m_params.command.isEmpty())
        str << "<tr><td>" << BreakpointItem::tr("Command:") << "</td><td>" << m_params.command << "</td></tr>";
    if (!m_params.message.isEmpty())
        str << "<tr><td>" << BreakpointItem::tr("Message:") << "</td><td>" << m_params.message << "</td></tr>";
    if (!m_params.condition.isEmpty())
        str << "<tr><td>" << BreakpointItem::tr("Condition:") << "</td><td>" << m_params.condition << "</td></tr>";
    if (m_params.ignoreCount)
        str << "<tr><td>" << BreakpointItem::tr("Ignore Count:") << "</td><td>" << m_params.ignoreCount << "</td></tr>";
    if (m_params.threadSpec >= 0)
        str << "<tr><td>" << BreakpointItem::tr("Thread Specification:") << "</td><td>" << m_params.threadSpec << "</td></tr>";

    str  << "</table></body></html><hr>";
    return rc;
}

//
// BreakpointManager
//

BreakpointManager::BreakpointManager()
{
    theBreakpointManager = this;
    setHeader({tr("Debuggee"), tr("Function"), tr("File"), tr("Line"), tr("Address"),
               tr("Condition"), tr("Ignore"), tr("Threads")});
    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            this, &BreakpointManager::loadSessionData);
    connect(SessionManager::instance(), &SessionManager::aboutToSaveSession,
            this, &BreakpointManager::saveSessionData);
    connect(SessionManager::instance(), &SessionManager::aboutToUnloadSession,
            this, &BreakpointManager::aboutToUnloadSession);
}

QAbstractItemModel *BreakpointManager::model()
{
    return theBreakpointManager;
}

const GlobalBreakpoints BreakpointManager::globalBreakpoints()
{
    GlobalBreakpoints items;
    theBreakpointManager->forItemsAtLevel<1>([&items](GlobalBreakpointItem *b) { items.append(b); });
    return items;
}

void BreakpointManager::claimBreakpointsForEngine(DebuggerEngine *engine)
{
    theBreakpointManager->forItemsAtLevel<1>([&](GlobalBreakpoint gbp) {
        engine->breakHandler()->tryClaimBreakpoint(gbp);
        gbp->updateMarker();
    });
}

GlobalBreakpoint BreakpointManager::createBreakpointHelper(const BreakpointParameters &params)
{
    GlobalBreakpoint gbp = new GlobalBreakpointItem;
    gbp->m_params = params;
    gbp->updateMarker();
    theBreakpointManager->rootItem()->appendChild(gbp);
    return gbp;
}

GlobalBreakpoint BreakpointManager::findBreakpointByIndex(const QModelIndex &index)
{
    return theBreakpointManager->itemForIndexAtLevel<1>(index);
}

GlobalBreakpoints BreakpointManager::findBreakpointsByIndex(const QList<QModelIndex> &list)
{
    QSet<GlobalBreakpoint> items;
    for (const QModelIndex &index : list) {
        if (GlobalBreakpoint gbp = findBreakpointByIndex(index))
            items.insert(gbp);
    }
    return items.toList();
}

GlobalBreakpoint BreakpointManager::createBreakpoint(const BreakpointParameters &params)
{
    GlobalBreakpoint gbp = createBreakpointHelper(params);
    for (QPointer<DebuggerEngine> engine : EngineManager::engines())
        engine->breakHandler()->tryClaimBreakpoint(gbp);
    return gbp;
}

void BreakpointManager::createBreakpointForEngine(const BreakpointParameters &params, DebuggerEngine *engine)
{
    GlobalBreakpoint gbp = createBreakpointHelper(params);
    engine->breakHandler()->tryClaimBreakpoint(gbp);
}

void BreakpointManager::toggleBreakpoint(const ContextData &location, const QString &tracePointMessage)
{
    QTC_ASSERT(location.isValid(), return);
    GlobalBreakpoint gbp = findBreakpointFromContext(location);

    if (gbp) {
        gbp->deleteBreakpoint();
    } else {
        BreakpointParameters data;
        if (location.type == LocationByFile) {
            data.type = BreakpointByFileAndLine;
            if (boolSetting(BreakpointsFullPathByDefault))
                data.pathUsage = BreakpointUseFullPath;
            data.tracepoint = !tracePointMessage.isEmpty();
            data.message = tracePointMessage;
            data.fileName = location.fileName;
            data.lineNumber = location.lineNumber;
        } else if (location.type == LocationByAddress) {
            data.type = BreakpointByAddress;
            data.tracepoint = !tracePointMessage.isEmpty();
            data.message = tracePointMessage;
            data.address = location.address;
        }
        BreakpointManager::createBreakpoint(data);
    }
}

GlobalBreakpoint BreakpointManager::findBreakpointFromContext(const ContextData &location)
{
    int matchLevel = 0;
    GlobalBreakpoint bestMatch;
    theBreakpointManager->forItemsAtLevel<1>([&](const GlobalBreakpoint &gbp) {
        if (location.type == LocationByFile) {
            if (gbp->m_params.isLocatedAt(location.fileName, location.lineNumber, QString())) {
                matchLevel = 2;
                bestMatch = gbp;
            } else if (matchLevel < 2) {
                for (const QPointer<DebuggerEngine> engine : EngineManager::engines()) {
                    BreakHandler *handler = engine->breakHandler();
                    for (Breakpoint bp : handler->breakpoints()) {
                        if (bp->globalBreakpoint() == gbp) {
                            if (fileNameMatch(bp->fileName(), location.fileName)
                                    && bp->lineNumber() == location.lineNumber) {
                                matchLevel = 1;
                                bestMatch = gbp;
                            }
                        }
                    }
                }
            }
        } else if (location.type == LocationByAddress) {
            if (gbp->m_params.address == location.address) {
                matchLevel = 2;
                bestMatch = gbp;
            }
        }
    });

    return bestMatch;
}

void BreakpointManager::executeAddBreakpointDialog()
{
    BreakpointParameters data(BreakpointByFileAndLine);
    BreakpointParts parts = NoParts;
    BreakpointDialog dialog(~0, ICore::dialogParent());
    dialog.setWindowTitle(tr("Add Breakpoint"));
    if (dialog.showDialog(&data, &parts))
        BreakpointManager::createBreakpoint(data);
}

QVariant BreakpointManager::data(const QModelIndex &idx, int role) const
{
    if (role == BaseTreeView::ItemDelegateRole)
        return QVariant::fromValue(new LeftElideDelegate);

    return BreakpointManagerModel::data(idx, role);
}

bool BreakpointManager::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == BaseTreeView::ItemActivatedRole) {
        if (GlobalBreakpoint bp = findBreakpointByIndex(idx))
            gotoLocation(bp);
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = value.value<ItemViewEvent>();

        if (ev.as<QContextMenuEvent>())
            return contextMenuEvent(ev);

        if (auto kev = ev.as<QKeyEvent>(QEvent::KeyPress)) {
            if (kev->key() == Qt::Key_Delete) {
                QModelIndexList si = ev.currentOrSelectedRows();
                const GlobalBreakpoints gbps = findBreakpointsByIndex(si);
                for (GlobalBreakpoint gbp : gbps)
                    gbp->deleteBreakpoint();
//                int row = qMin(rowCount() - ids.size() - 1, idx.row());
//                setCurrentIndex(index(row, 0));   FIXME
                return true;
            }
            if (kev->key() == Qt::Key_Space) {
                const QModelIndexList selectedIds = ev.selectedRows();
                if (!selectedIds.isEmpty()) {
                    const GlobalBreakpoints gbps = findBreakpointsByIndex(selectedIds);
                    const bool isEnabled = gbps.isEmpty() || gbps.at(0)->isEnabled();
                    for (GlobalBreakpoint gbp : gbps)
                        gbp->setEnabled(!isEnabled);
//                    scheduleSynchronization();
                    return true;
                }
            }
        }

        if (ev.as<QMouseEvent>(QEvent::MouseButtonDblClick)) {
            if (GlobalBreakpoint gbp = findBreakpointByIndex(idx)) {
                if (idx.column() >= BreakpointAddressColumn)
                    editBreakpoints({gbp}, ev.view());
                else
                    gotoLocation(gbp);
            } else {
                BreakpointManager::executeAddBreakpointDialog();
            }
            return true;
        }
    }

    return false;
}

bool BreakpointManager::contextMenuEvent(const ItemViewEvent &ev)
{
    const QModelIndexList selectedIndices = ev.selectedRows();

    const GlobalBreakpoints selectedBreakpoints = findBreakpointsByIndex(selectedIndices);
    const bool breakpointsEnabled = selectedBreakpoints.isEmpty() || selectedBreakpoints.at(0)->isEnabled();

    auto menu = new QMenu;

    addAction(menu, tr("Add Breakpoint..."), true, &BreakpointManager::executeAddBreakpointDialog);

    addAction(menu, tr("Delete Selected Breakpoints"),
              !selectedBreakpoints.isEmpty(),
              [selectedBreakpoints] {
                for (GlobalBreakpoint gbp : selectedBreakpoints)
                    gbp->deleteBreakpoint();
             });

    addAction(menu, tr("Edit Selected Breakpoints..."),
              !selectedBreakpoints.isEmpty(),
              [this, selectedBreakpoints, ev] { editBreakpoints(selectedBreakpoints, ev.view()); });

    addAction(menu,
              selectedBreakpoints.size() > 1
                  ? breakpointsEnabled ? tr("Disable Selected Breakpoints") : tr("Enable Selected Breakpoints")
                  : breakpointsEnabled ? tr("Disable Breakpoint") : tr("Enable Breakpoint"),
              !selectedBreakpoints.isEmpty(),
              [selectedBreakpoints, breakpointsEnabled] {
                    for (GlobalBreakpoint gbp : selectedBreakpoints)
                        gbp->setEnabled(!breakpointsEnabled);
              }
    );

    menu->addSeparator();

    addAction(menu, tr("Delete All Breakpoints"),
              rowCount() > 0,
              &BreakpointManager::executeDeleteAllBreakpointsDialog);

    // Delete by file: Find indices of breakpoints of the same file.
    GlobalBreakpoints breakpointsInFile;
    QString file;
    if (GlobalBreakpoint gbp = itemForIndexAtLevel<1>(ev.sourceModelIndex())) {
        if (!file.isEmpty()) {
            for (int i = 0; i != rowCount(); ++i)
                if (gbp->markerFileName() == file)
                    breakpointsInFile.append(gbp);
        }
    }
    addAction(menu, tr("Delete Breakpoints of \"%1\"").arg(file),
              tr("Delete Breakpoints of File"),
              breakpointsInFile.size() > 1,
              [breakpointsInFile] {
                for (GlobalBreakpoint gbp : breakpointsInFile)
                    gbp->deleteBreakpoint();
              });

    menu->addSeparator();

    menu->addAction(action(UseToolTipsInBreakpointsView));
    menu->addAction(action(UseAddressInBreakpointsView));
    menu->addSeparator();
    menu->addAction(action(SettingsDialog));

    menu->popup(ev.globalPos());

    return true;
}

void BreakpointManager::gotoLocation(const GlobalBreakpoint &gbp) const
{
    QTC_ASSERT(gbp, return);
    const QString file = QDir::cleanPath(gbp->markerFileName());
    if (IEditor *editor = EditorManager::openEditor(file))
        editor->gotoLine(gbp->markerLineNumber(), 0);
}

void BreakpointManager::executeDeleteAllBreakpointsDialog()
{
    QDialogButtonBox::StandardButton pressed =
        CheckableMessageBox::doNotAskAgainQuestion(ICore::dialogParent(),
           tr("Remove All Breakpoints"),
           tr("Are you sure you want to remove all breakpoints "
              "from all files in the current session?"),
           ICore::settings(),
           "RemoveAllBreakpoints");
    if (pressed != QDialogButtonBox::Yes)
        return;

    for (GlobalBreakpoint gbp : globalBreakpoints())
        gbp->deleteBreakpoint();
}

void BreakpointManager::editBreakpoint(const GlobalBreakpoint &gbp, QWidget *parent)
{
    QTC_ASSERT(gbp, return);
    BreakpointParts parts = NoParts;

    BreakpointParameters params = gbp->requestedParameters();
    BreakpointDialog dialog(~0, parent);
    if (!dialog.showDialog(&params, &parts))
        return;

    gbp->destroyMarker();
    gbp->deleteBreakpoint();
    BreakpointManager::createBreakpoint(params);
}

void BreakpointManager::editBreakpoints(const GlobalBreakpoints &gbps, QWidget *parent)
{
    QTC_ASSERT(!gbps.isEmpty(), return);

    GlobalBreakpoint gbp = gbps.at(0);

    if (gbps.size() == 1) {
        editBreakpoint(gbp, parent);
        return;
    }

    // This allows to change properties of multiple breakpoints at a time.
    QTC_ASSERT(gbp, return);
    BreakpointParameters params = gbp->requestedParameters();

    MultiBreakPointsDialog dialog(~0, parent);
    dialog.setCondition(params.condition);
    dialog.setIgnoreCount(params.ignoreCount);
    dialog.setThreadSpec(params.threadSpec);

    if (dialog.exec() == QDialog::Rejected)
        return;

    const QString newCondition = dialog.condition();
    const int newIgnoreCount = dialog.ignoreCount();
    const int newThreadSpec = dialog.threadSpec();

    for (GlobalBreakpoint gbp : gbps) {
        QTC_ASSERT(gbp, continue);
        BreakpointParameters newParams = gbp->requestedParameters();
        newParams.condition = newCondition;
        newParams.ignoreCount = newIgnoreCount;
        newParams.threadSpec = newThreadSpec;
        gbp->destroyMarker();
        gbp->deleteBreakpoint();
        BreakpointManager::createBreakpoint(newParams);
    }
}
void BreakpointManager::saveSessionData()
{
    QList<QVariant> list;
    theBreakpointManager->forItemsAtLevel<1>([&list](const GlobalBreakpoint &bp) {
        const BreakpointParameters &params = bp->m_params;
        QMap<QString, QVariant> map;
        if (params.type != BreakpointByFileAndLine)
            map.insert("type", params.type);
        if (!params.fileName.isEmpty())
            map.insert("filename", params.fileName);
        if (params.lineNumber)
            map.insert("linenumber", params.lineNumber);
        if (!params.functionName.isEmpty())
            map.insert("funcname", params.functionName);
        if (params.address)
            map.insert("address", params.address);
        if (!params.condition.isEmpty())
            map.insert("condition", params.condition);
        if (params.ignoreCount)
            map.insert("ignorecount", params.ignoreCount);
        if (params.threadSpec >= 0)
            map.insert("threadspec", params.threadSpec);
        if (!params.enabled)
            map.insert("disabled", "1");
        if (params.oneShot)
            map.insert("oneshot", "1");
        if (params.pathUsage != BreakpointPathUsageEngineDefault)
            map.insert("usefullpath", QString::number(params.pathUsage));
        if (params.tracepoint)
            map.insert("tracepoint", "1");
        if (!params.module.isEmpty())
            map.insert("module", params.module);
        if (!params.command.isEmpty())
            map.insert("command", params.command);
        if (!params.expression.isEmpty())
            map.insert("expression", params.expression);
        if (!params.message.isEmpty())
            map.insert("message", params.message);
        list.append(map);
    });
    SessionManager::setValue("Breakpoints", list);
}

void BreakpointManager::aboutToUnloadSession()
{
    saveSessionData();
    clear();
}

void BreakpointManager::loadSessionData()
{
    clear();

    const QVariant value = SessionManager::value("Breakpoints");
    const QList<QVariant> list = value.toList();
    for (const QVariant &var : list) {
        const QMap<QString, QVariant> map = var.toMap();
        BreakpointParameters params(BreakpointByFileAndLine);
        QVariant v = map.value("filename");
        if (v.isValid())
            params.fileName = v.toString();
        v = map.value("linenumber");
        if (v.isValid())
            params.lineNumber = v.toString().toInt();
        v = map.value("condition");
        if (v.isValid())
            params.condition = v.toString();
        v = map.value("address");
        if (v.isValid())
            params.address = v.toString().toULongLong();
        v = map.value("ignorecount");
        if (v.isValid())
            params.ignoreCount = v.toString().toInt();
        v = map.value("threadspec");
        if (v.isValid())
            params.threadSpec = v.toString().toInt();
        v = map.value("funcname");
        if (v.isValid())
            params.functionName = v.toString();
        v = map.value("disabled");
        if (v.isValid())
            params.enabled = !v.toInt();
        v = map.value("oneshot");
        if (v.isValid())
            params.oneShot = v.toInt();
        v = map.value("usefullpath");
        if (v.isValid())
            params.pathUsage = static_cast<BreakpointPathUsage>(v.toInt());
        v = map.value("tracepoint");
        if (v.isValid())
            params.tracepoint = bool(v.toInt());
        v = map.value("type");
        if (v.isValid() && v.toInt() != UnknownBreakpointType)
            params.type = BreakpointType(v.toInt());
        v = map.value("module");
        if (v.isValid())
            params.module = v.toString();
        v = map.value("command");
        if (v.isValid())
            params.command = v.toString();
        v = map.value("expression");
        if (v.isValid())
            params.expression = v.toString();
        v = map.value("message");
        if (v.isValid())
            params.message = v.toString();
        if (params.isValid())
            BreakpointManager::createBreakpoint(params);
        else
            qWarning("Not restoring invalid breakpoint: %s", qPrintable(params.toString()));
    }
}

} // namespace Internal
} // namespace Debugger
