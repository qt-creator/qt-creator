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

#include "breakhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggericons.h"
#include "debuggerinternalconstants.h"
#include "simplifytype.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugin.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <extensionsystem/invoker.h>
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
using namespace Utils;

namespace Debugger {
namespace Internal {

class LocationItem : public TreeItem
{
public:
    QVariant data(int column, int role) const
    {
        if (role == Qt::DisplayRole) {
            switch (column) {
                case BreakpointNumberColumn:
                    return params.id.toString();
                case BreakpointFunctionColumn:
                    return params.functionName;
                case BreakpointAddressColumn:
                    if (params.address)
                        return QString::fromLatin1("0x%1").arg(params.address, 0, 16);
            }
        }
        return QVariant();
    }

    BreakpointResponse params;
};

class BreakpointMarker;

class BreakpointItem : public QObject, public TypedTreeItem<LocationItem>
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::BreakHandler)

public:
    ~BreakpointItem();

    QVariant data(int column, int role) const;

    QIcon icon() const;

    void removeBreakpoint();
    void updateLineNumberFromMarker(int lineNumber);
    void updateFileNameFromMarker(const QString &fileName);
    void changeLineNumberFromMarker(int lineNumber);
    bool isLocatedAt(const QString &fileName, int lineNumber, bool useMarkerPosition) const;

    void setMarkerFileAndLine(const QString &fileName, int lineNumber);

    void insertSubBreakpoint(const BreakpointResponse &params);
    QString markerFileName() const;
    int markerLineNumber() const;

    bool needsChange() const;

private:
    friend class BreakHandler;
    friend class Breakpoint;
    BreakpointItem(BreakHandler *handler);

    void destroyMarker();
    void updateMarker();
    void updateMarkerIcon();
    void scheduleSynchronization();

    QString toToolTip() const;
    void setState(BreakpointState state);
    void deleteThis();
    bool isEngineRunning() const;

    BreakHandler * const m_handler;
    const BreakpointModelId m_id;
    BreakpointParameters m_params;
    BreakpointState m_state;   // Current state of breakpoint.
    DebuggerEngine *m_engine;  // Engine currently handling the breakpoint.
    BreakpointResponse m_response;
    BreakpointMarker *m_marker;
};

//
// BreakpointMarker
//

// The red blob on the left side in the cpp editor.
class BreakpointMarker : public TextEditor::TextMark
{
public:
    BreakpointMarker(BreakpointItem *b, const QString &fileName, int lineNumber)
        : TextMark(fileName, lineNumber, Constants::TEXT_MARK_CATEGORY_BREAKPOINT), m_bp(b)
    {
        setIcon(b->icon());
        setPriority(TextEditor::TextMark::NormalPriority);
    }

    void removedFromEditor()
    {
        if (m_bp)
            m_bp->removeBreakpoint();
    }

    void updateLineNumber(int lineNumber)
    {
        TextMark::updateLineNumber(lineNumber);
        m_bp->updateLineNumberFromMarker(lineNumber);
    }

    void updateFileName(const QString &fileName)
    {
        TextMark::updateFileName(fileName);
        m_bp->updateFileNameFromMarker(fileName);
    }

    bool isDraggable() const { return true; }
    void dragToLine(int line) { m_bp->changeLineNumberFromMarker(line); }
    bool isClickable() const { return true; }
    void clicked() { m_bp->removeBreakpoint(); }

public:
    BreakpointItem *m_bp;
};

static QString stateToString(BreakpointState state)
{
    switch (state) {
        case BreakpointNew:
            return BreakHandler::tr("New");
        case BreakpointInsertRequested:
            return BreakHandler::tr("Insertion requested");
        case BreakpointInsertProceeding:
            return BreakHandler::tr("Insertion proceeding");
        case BreakpointChangeRequested:
            return BreakHandler::tr("Change requested");
        case BreakpointChangeProceeding:
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
    LeftElideDelegate() {}

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
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::BreakHandler)

public:
    explicit BreakpointDialog(Breakpoint b, QWidget *parent = 0);
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

BreakpointDialog::BreakpointDialog(Breakpoint b, QWidget *parent)
    : QDialog(parent), m_enabledParts(~0), m_previousType(UnknownBreakpointType),
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
    m_pathChooserFileName->setHistoryCompleter(QLatin1String("Debugger.Breakpoint.File.History"));
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
        tr("<p>Specifying the module (base name of the library or executable) "
           "for function or file type breakpoints can significantly speed up "
           "debugger startup times (CDB, LLDB).");
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

BreakHandler::BreakHandler()
  : m_syncTimerId(-1)
{
    qRegisterMetaType<BreakpointModelId>();
    TextEditor::TextMark::setCategoryColor(Constants::TEXT_MARK_CATEGORY_BREAKPOINT,
                                           Theme::Debugger_Breakpoint_TextMarkColor);
    TextEditor::TextMark::setDefaultToolTip(Constants::TEXT_MARK_CATEGORY_BREAKPOINT,
                                            tr("Breakpoint"));

#if USE_BREAK_MODEL_TEST
    new ModelTest(this, 0);
#endif
    setHeader(QStringList({tr("Number"), tr("Function"), tr("File"), tr("Line"), tr("Address"),
                           tr("Condition"), tr("Ignore"), tr("Threads")}));
}

static inline bool fileNameMatch(const QString &f1, const QString &f2)
{
    if (HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive)
        return f1.compare(f2, Qt::CaseInsensitive) == 0;
    return f1 == f2;
}

static bool isSimilarTo(const BreakpointParameters &params, const BreakpointResponse &needle)
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

    // At least at a position we were looking for.
    // FIXME: breaks multiple breakpoints at the same location
    if (!params.fileName.isEmpty()
            && fileNameMatch(params.fileName, needle.fileName)
            && params.lineNumber == needle.lineNumber)
        return true;

    return false;
}

Breakpoint BreakHandler::findSimilarBreakpoint(const BreakpointResponse &needle) const
{
    // Search a breakpoint we might refer to.
    return Breakpoint(findItemAtLevel<1>([needle](BreakpointItem *b) {
        if (b->m_response.id.isValid() && b->m_response.id.majorPart() == needle.id.majorPart())
            return true;
        return isSimilarTo(b->m_params, needle);
    }));
}

Breakpoint BreakHandler::findBreakpointByResponseId(const BreakpointResponseId &id) const
{
    return Breakpoint(findItemAtLevel<1>([id](BreakpointItem *b) {
        return b->m_response.id.majorPart() == id.majorPart();
    }));
}

Breakpoint BreakHandler::findBreakpointByFunction(const QString &functionName) const
{
    return Breakpoint(findItemAtLevel<1>([functionName](BreakpointItem *b) {
        return b->m_params.functionName == functionName;
    }));
}

Breakpoint BreakHandler::findBreakpointByAddress(quint64 address) const
{
    return Breakpoint(findItemAtLevel<1>([address](BreakpointItem *b) {
        return b->m_params.address == address;
    }));
}

Breakpoint BreakHandler::findBreakpointByFileAndLine(const QString &fileName,
    int lineNumber, bool useMarkerPosition)
{
    return Breakpoint(findItemAtLevel<1>([=](BreakpointItem *b) {
        return b->isLocatedAt(fileName, lineNumber, useMarkerPosition);
    }));
}

Breakpoint BreakHandler::breakpointById(BreakpointModelId id) const
{
    return Breakpoint(findItemAtLevel<1>([id](BreakpointItem *b) { return b->m_id == id; }));
}

QVariant BreakHandler::data(const QModelIndex &idx, int role) const
{
    if (role == BaseTreeView::ItemDelegateRole)
        return QVariant::fromValue(new LeftElideDelegate);

    return BreakModel::data(idx, role);
}

void BreakHandler::deletionHelper(BreakpointModelId id)
{
    Breakpoint b = breakpointById(id);
    QTC_ASSERT(b, return);
    destroyItem(b.b);
}

Breakpoint BreakHandler::findWatchpoint(const BreakpointParameters &params) const
{
    return Breakpoint(findItemAtLevel<1>([params](BreakpointItem *b) {
        return b->m_params.isWatchpoint()
                && b->m_params.address == params.address
                && b->m_params.size == params.size
                && b->m_params.expression == params.expression
                && b->m_params.bitpos == params.bitpos;
    }));
}

void BreakHandler::saveBreakpoints()
{
    QList<QVariant> list;
    forItemsAtLevel<1>([&list](BreakpointItem *b) {
        const BreakpointParameters &params = b->m_params;
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
    setSessionValue("Breakpoints", list);
}

void BreakHandler::loadBreakpoints()
{
    QVariant value = sessionValue("Breakpoints");
    QList<QVariant> list = value.toList();
    foreach (const QVariant &var, list) {
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
            appendBreakpointInternal(params);
        else
            qWarning("Not restoring invalid breakpoint: %s", qPrintable(params.toString()));
    }
}

void BreakHandler::updateMarkers()
{
    forItemsAtLevel<1>([](BreakpointItem *b) { b->updateMarker(); });
}

Breakpoint BreakHandler::findBreakpointByIndex(const QModelIndex &index) const
{
    return Breakpoint(itemForIndexAtLevel<1>(index));
}

Breakpoints BreakHandler::findBreakpointsByIndex(const QList<QModelIndex> &list) const
{
    QSet<Breakpoint> ids;
    foreach (const QModelIndex &index, list) {
        if (Breakpoint b = findBreakpointByIndex(index))
            ids.insert(b);
    }
    return ids.toList();
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

const QString empty(QLatin1Char('-'));

QVariant BreakpointItem::data(int column, int role) const
{
    bool orig = false;
    switch (m_state) {
        case BreakpointInsertRequested:
        case BreakpointInsertProceeding:
        case BreakpointChangeRequested:
        case BreakpointChangeProceeding:
        case BreakpointInserted:
        case BreakpointRemoveRequested:
        case BreakpointRemoveProceeding:
            break;
        case BreakpointNew:
        case BreakpointDead:
            orig = true;
            break;
    };

    if (role == Qt::ForegroundRole) {
        static const QVariant gray(QColor(140, 140, 140));
        switch (m_state) {
            case BreakpointInsertRequested:
            case BreakpointInsertProceeding:
            case BreakpointChangeRequested:
            case BreakpointChangeProceeding:
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
                return m_id.toString();
            if (role == Qt::DecorationRole)
                return icon();
            break;
        case BreakpointFunctionColumn:
            if (role == Qt::DisplayRole) {
                if (!m_response.functionName.isEmpty())
                    return simplifyType(m_response.functionName);
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
                if (m_params.type == WatchpointAtAddress) {
                    quint64 address = m_response.address ? m_response.address : m_params.address;
                    return BreakHandler::tr("Data at 0x%1").arg(address, 0, 16);
                }
                if (m_params.type == WatchpointAtExpression) {
                    QString expression = !m_response.expression.isEmpty()
                            ? m_response.expression : m_params.expression;
                    return BreakHandler::tr("Data at %1").arg(expression);
                }
                return empty;
            }
            break;
        case BreakpointFileColumn:
            if (role == Qt::DisplayRole) {
                QString str;
                if (!m_response.fileName.isEmpty())
                    str = m_response.fileName;
                if (str.isEmpty() && !m_params.fileName.isEmpty())
                    str = m_params.fileName;
                if (str.isEmpty()) {
                    QString s = FileName::fromString(str).fileName();
                    if (!s.isEmpty())
                        str = s;
                }
                // FIXME: better?
                //if (params.multiple && str.isEmpty() && !response.fileName.isEmpty())
                //    str = response.fileName;
                if (!str.isEmpty())
                    return QDir::toNativeSeparators(str);
                return empty;
            }
            break;
        case BreakpointLineColumn:
            if (role == Qt::DisplayRole) {
                if (m_response.lineNumber > 0)
                    return m_response.lineNumber;
                if (m_params.lineNumber > 0)
                    return m_params.lineNumber;
                return empty;
            }
            if (role == Qt::UserRole + 1)
                return m_params.lineNumber;
            break;
        case BreakpointAddressColumn:
            if (role == Qt::DisplayRole) {
                const quint64 address = orig ? m_params.address : m_response.address;
                if (address)
                    return QString("0x%1").arg(address, 0, 16);
                return QVariant();
            }
            break;
        case BreakpointConditionColumn:
            if (role == Qt::DisplayRole)
                return orig ? m_params.condition : m_response.condition;
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit if this condition is met.");
            if (role == Qt::UserRole + 1)
                return m_params.condition;
            break;
        case BreakpointIgnoreColumn:
            if (role == Qt::DisplayRole) {
                const int ignoreCount =
                    orig ? m_params.ignoreCount : m_response.ignoreCount;
                return ignoreCount ? QVariant(ignoreCount) : QVariant(QString());
            }
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit after being ignored so many times.");
            if (role == Qt::UserRole + 1)
                return m_params.ignoreCount;
            break;
        case BreakpointThreadsColumn:
            if (role == Qt::DisplayRole)
                return BreakHandler::displayFromThreadSpec(orig ? m_params.threadSpec : m_response.threadSpec);
            if (role == Qt::ToolTipRole)
                return BreakHandler::tr("Breakpoint will only be hit in the specified thread(s).");
            if (role == Qt::UserRole + 1)
                return BreakHandler::displayFromThreadSpec(m_params.threadSpec);
            break;
    }

    if (role == Qt::ToolTipRole && boolSetting(UseToolTipsInBreakpointsView))
        return toToolTip();

    return QVariant();
}

#define PROPERTY(type, getter, setter) \
\
type Breakpoint::getter() const \
{ \
    return parameters().getter; \
} \
\
void Breakpoint::setter(const type &value) \
{ \
    QTC_ASSERT(b, return); \
    if (b->m_params.getter == value) \
        return; \
    b->m_params.getter = value; \
    if (b->m_state != BreakpointNew) { \
        b->m_state = BreakpointChangeRequested; \
        b->scheduleSynchronization(); \
    } \
}

PROPERTY(BreakpointPathUsage, pathUsage, setPathUsage)
PROPERTY(QString, fileName, setFileName)
PROPERTY(QString, functionName, setFunctionName)
PROPERTY(BreakpointType, type, setType)
PROPERTY(int, threadSpec, setThreadSpec)
PROPERTY(QString, condition, setCondition)
PROPERTY(QString, command, setCommand)
PROPERTY(quint64, address, setAddress)
PROPERTY(QString, expression, setExpression)
PROPERTY(QString, message, setMessage)
PROPERTY(int, ignoreCount, setIgnoreCount)

void BreakpointItem::scheduleSynchronization()
{
    m_handler->scheduleSynchronization();
}

const BreakpointParameters &Breakpoint::parameters() const
{
    static BreakpointParameters p;
    QTC_ASSERT(b, return p);
    return b->m_params;
}

void Breakpoint::addToCommand(DebuggerCommand *cmd) const
{
    cmd->arg("modelid", id().toString());
    cmd->arg("id", int(response().id.majorPart()));
    cmd->arg("type", type());
    cmd->arg("ignorecount", ignoreCount());
    cmd->arg("condition", toHex(condition()));
    cmd->arg("command", toHex(command()));
    cmd->arg("function", functionName());
    cmd->arg("oneshot", isOneShot());
    cmd->arg("enabled", isEnabled());
    cmd->arg("file", fileName());
    cmd->arg("line", lineNumber());
    cmd->arg("address", address());
    cmd->arg("expression", expression());
}

BreakpointState Breakpoint::state() const
{
    QTC_ASSERT(b, return BreakpointState());
    return b->m_state;
}

int Breakpoint::lineNumber() const { return parameters().lineNumber; }

bool Breakpoint::isEnabled() const { return parameters().enabled; }

bool Breakpoint::isWatchpoint() const { return parameters().isWatchpoint(); }

bool Breakpoint::isTracepoint() const { return parameters().isTracepoint(); }

QIcon Breakpoint::icon() const { return b ? b->icon() : QIcon(); }

DebuggerEngine *Breakpoint::engine() const
{
    return b ? b->m_engine : 0;
}

const BreakpointResponse &Breakpoint::response() const
{
    static BreakpointResponse r;
    return b ? b->m_response : r;
}

bool Breakpoint::isOneShot() const { return parameters().oneShot; }

void Breakpoint::removeAlienBreakpoint()
{
    b->m_state = BreakpointRemoveProceeding;
    b->deleteThis();
}

void Breakpoint::removeBreakpoint() const
{
    if (b)
        b->removeBreakpoint();
}

Breakpoint::Breakpoint(BreakpointItem *b)
    : b(b)
{}

void Breakpoint::setEnabled(bool on) const
{
    QTC_ASSERT(b, return);
    if (b->m_params.enabled == on)
        return;
    b->m_params.enabled = on;
    b->updateMarkerIcon();
    b->update();
    if (b->m_engine) {
        b->m_state = BreakpointChangeRequested;
        b->scheduleSynchronization();
    }
}

void Breakpoint::setMarkerFileAndLine(const QString &fileName, int lineNumber)
{
    if (b)
        b->setMarkerFileAndLine(fileName, lineNumber);
}

void Breakpoint::setTracepoint(bool on)
{
    if (b->m_params.tracepoint == on)
        return;
    b->m_params.tracepoint = on;
    b->updateMarkerIcon();

    if (b->m_engine) {
        b->m_state = BreakpointChangeRequested;
        b->scheduleSynchronization();
    }
}

void BreakpointItem::setMarkerFileAndLine(const QString &fileName, int lineNumber)
{
    if (m_response.fileName == fileName && m_response.lineNumber == lineNumber)
        return;
    m_response.fileName = fileName;
    m_response.lineNumber = lineNumber;
    destroyMarker();
    updateMarker();
    update();
}

void Breakpoint::setEngine(DebuggerEngine *value)
{
    QTC_ASSERT(b->m_state == BreakpointNew, qDebug() << "STATE: " << b->m_state << b->m_id);
    QTC_ASSERT(!b->m_engine, qDebug() << "NO ENGINE" << b->m_id; return);
    b->m_engine = value;
    b->m_state = BreakpointInsertRequested;
    b->m_response = BreakpointResponse();
    b->updateMarker();
    //b->scheduleSynchronization();
}

static bool isAllowedTransition(BreakpointState from, BreakpointState to)
{
    switch (from) {
    case BreakpointNew:
        return to == BreakpointInsertRequested
            || to == BreakpointDead;
    case BreakpointInsertRequested:
        return to == BreakpointInsertProceeding;
    case BreakpointInsertProceeding:
        return to == BreakpointInserted
            || to == BreakpointDead
            || to == BreakpointChangeRequested
            || to == BreakpointRemoveRequested;
    case BreakpointChangeRequested:
        return to == BreakpointChangeProceeding;
    case BreakpointChangeProceeding:
        return to == BreakpointInserted
            || to == BreakpointDead;
    case BreakpointInserted:
        return to == BreakpointChangeRequested
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

bool BreakpointItem::isEngineRunning() const
{
    if (!m_engine)
        return false;
    const DebuggerState state = m_engine->state();
    return state != DebuggerFinished && state != DebuggerNotReady;
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
        qDebug() << "STATE UNCHANGED: " << m_id << m_state;
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

void BreakpointItem::deleteThis()
{
    setState(BreakpointDead);
    destroyMarker();

    // This is called from b directly. So delay deletion of b.
    ExtensionSystem::InvokerBase invoker;
    invoker.addArgument(m_id);
    invoker.setConnectionType(Qt::QueuedConnection);
    invoker.invoke(m_handler, "deletionHelper");
    QTC_CHECK(invoker.wasSuccessful());
}

void Breakpoint::gotoState(BreakpointState target, BreakpointState assumedCurrent)
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == assumedCurrent, qDebug() << b->m_state);
    b->setState(target);
}

void Breakpoint::notifyBreakpointChangeAfterInsertNeeded()
{
    gotoState(BreakpointChangeRequested, BreakpointInsertProceeding);
}

void Breakpoint::notifyBreakpointInsertProceeding()
{
    gotoState(BreakpointInsertProceeding, BreakpointInsertRequested);
}

void Breakpoint::notifyBreakpointInsertOk()
{
    gotoState(BreakpointInserted, BreakpointInsertProceeding);
    if (b->m_engine)
        b->m_engine->updateBreakpointMarker(*this);
}

void Breakpoint::notifyBreakpointInsertFailed()
{
    gotoState(BreakpointDead, BreakpointInsertProceeding);
}

void Breakpoint::notifyBreakpointRemoveProceeding()
{
    gotoState(BreakpointRemoveProceeding, BreakpointRemoveRequested);
}

void Breakpoint::notifyBreakpointRemoveOk()
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == BreakpointRemoveProceeding, qDebug() << b->m_state);
    if (b->m_engine)
        b->m_engine->removeBreakpointMarker(*this);
    b->deleteThis();
}

void Breakpoint::notifyBreakpointRemoveFailed()
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == BreakpointRemoveProceeding, qDebug() << b->m_state);
    if (b->m_engine)
        b->m_engine->removeBreakpointMarker(*this);
    b->deleteThis();
}

void Breakpoint::notifyBreakpointChangeProceeding()
{
    gotoState(BreakpointChangeProceeding, BreakpointChangeRequested);
}

void Breakpoint::notifyBreakpointChangeOk()
{
    gotoState(BreakpointInserted, BreakpointChangeProceeding);
}

void Breakpoint::notifyBreakpointChangeFailed()
{
    gotoState(BreakpointDead, BreakpointChangeProceeding);
}

void Breakpoint::notifyBreakpointReleased()
{
    QTC_ASSERT(b, return);
    b->removeChildren();
    //QTC_ASSERT(b->m_state == BreakpointChangeProceeding, qDebug() << b->m_state);
    b->m_state = BreakpointNew;
    b->m_engine = 0;
    b->m_response = BreakpointResponse();
    b->destroyMarker();
    b->updateMarker();
    if (b->m_params.type == WatchpointAtAddress
            || b->m_params.type == WatchpointAtExpression
            || b->m_params.type == BreakpointByAddress)
        b->m_params.enabled = false;
    else
        b->m_params.address = 0;
    b->update();
}

void Breakpoint::notifyBreakpointAdjusted(const BreakpointParameters &params)
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == BreakpointInserted, qDebug() << b->m_state);
    b->m_params = params;
    //if (b->needsChange())
    //    b->setState(BreakpointChangeRequested);
}

void Breakpoint::notifyBreakpointNeedsReinsertion()
{
    QTC_ASSERT(b, return);
    QTC_ASSERT(b->m_state == BreakpointChangeProceeding, qDebug() << b->m_state);
    b->m_state = BreakpointInsertRequested;
}

void BreakpointItem::removeBreakpoint()
{
    switch (m_state) {
    case BreakpointRemoveRequested:
        break;
    case BreakpointInserted:
    case BreakpointInsertProceeding:
        setState(BreakpointRemoveRequested);
        scheduleSynchronization();
        break;
    case BreakpointNew:
        deleteThis();
        break;
    default:
        qWarning("Warning: Cannot remove breakpoint %s in state '%s'.",
               qPrintable(m_id.toString()), qPrintable(stateToString(m_state)));
        m_state = BreakpointRemoveRequested;
        break;
    }
}

void BreakHandler::appendBreakpoint(const BreakpointParameters &params)
{
    appendBreakpointInternal(params);
    scheduleSynchronization();
}

void BreakHandler::appendBreakpointInternal(const BreakpointParameters &params)
{
    if (!params.isValid()) {
        qWarning("Not adding invalid breakpoint: %s", qPrintable(params.toString()));
        return;
    }

    BreakpointItem *b = new BreakpointItem(this);
    b->m_params = params;
    b->updateMarker();
    rootItem()->appendChild(b);
}

void BreakHandler::handleAlienBreakpoint(const BreakpointResponse &response, DebuggerEngine *engine)
{
    Breakpoint b = findSimilarBreakpoint(response);
    if (b) {
        if (response.id.isMinor())
            b.insertSubBreakpoint(response);
        else
            b.setResponse(response);
    } else {
        auto b = new BreakpointItem(this);
        b->m_params = response;
        b->m_response = response;
        b->m_state = BreakpointInserted;
        b->m_engine = engine;
        b->updateMarker();
        rootItem()->appendChild(b);
    }
}

void Breakpoint::insertSubBreakpoint(const BreakpointResponse &params)
{
    QTC_ASSERT(b, return);
    b->insertSubBreakpoint(params);
}

void BreakpointItem::insertSubBreakpoint(const BreakpointResponse &params)
{
    QTC_ASSERT(params.id.isMinor(), return);

    int minorPart = params.id.minorPart();

    LocationItem *l = findFirstLevelChild([minorPart](LocationItem *l) {
        return l->params.id.minorPart() == minorPart;
    });
    if (l) {
        // This modifies an existing sub-breakpoint.
        l->params = params;
        l->update();
    } else {
        // This is a new sub-breakpoint.
        l = new LocationItem;
        l->params = params;
        appendChild(l);
        expand();
    }
}

void BreakHandler::saveSessionData()
{
    saveBreakpoints();
}

void BreakHandler::loadSessionData()
{
    clear();
    loadBreakpoints();
}

void BreakHandler::breakByFunction(const QString &functionName)
{
    // One breakpoint per function is enough for now. This does not handle
    // combinations of multiple conditions and ignore counts, though.
    bool found = findItemAtLevel<1>([functionName](BreakpointItem *b) {
        const BreakpointParameters &params = b->m_params;
        return params.functionName == functionName
                && params.condition.isEmpty()
                && params.ignoreCount == 0;
    });
    if (found)
        return;

    BreakpointParameters params(BreakpointByFunction);
    params.functionName = functionName;
    appendBreakpoint(params);
}

void BreakHandler::scheduleSynchronization()
{
    if (m_syncTimerId == -1)
        m_syncTimerId = startTimer(10);
}

void BreakHandler::timerEvent(QTimerEvent *event)
{
    QTC_ASSERT(event->timerId() == m_syncTimerId, return);
    killTimer(m_syncTimerId);
    m_syncTimerId = -1;
    saveBreakpoints();  // FIXME: remove?
    Internal::synchronizeBreakpoints();
}

void Breakpoint::gotoLocation() const
{
    if (DebuggerEngine *engine = currentEngine()) {
        if (b->m_params.type == BreakpointByAddress) {
            engine->gotoLocation(b->m_params.address);
        } else {
            // Don't use gotoLocation unconditionally as this ends up in
            // disassembly if OperateByInstruction is on. But fallback
            // to disassembly if we can't open the file.
            const QString file = QDir::cleanPath(b->markerFileName());
            if (IEditor *editor = EditorManager::openEditor(file))
                editor->gotoLine(b->markerLineNumber(), 0);
            else
                engine->openDisassemblerView(Location(b->m_response.address));
        }
    }
}

void BreakpointItem::updateFileNameFromMarker(const QString &fileName)
{
    m_params.fileName = fileName;
    update();
}

void BreakpointItem::updateLineNumberFromMarker(int lineNumber)
{
    // Ignore updates to the "real" line number while the debugger is
    // running, as this can be triggered by moving the breakpoint to
    // the next line that generated code.
    if (m_params.lineNumber == lineNumber)
        ; // Nothing
    else if (isEngineRunning())
        m_params.lineNumber += lineNumber - m_response.lineNumber;
    else
        m_params.lineNumber = lineNumber;
    updateMarker();
    update();
}

void BreakpointItem::changeLineNumberFromMarker(int lineNumber)
{
    m_params.lineNumber = lineNumber;

    // We need to delay this as it is called from a marker which will be destroyed.
    ExtensionSystem::InvokerBase invoker;
    invoker.addArgument(m_id);
    invoker.setConnectionType(Qt::QueuedConnection);
    invoker.invoke(m_handler, "changeLineNumberFromMarkerHelper");
    QTC_CHECK(invoker.wasSuccessful());
}

void BreakHandler::changeLineNumberFromMarkerHelper(BreakpointModelId id)
{
    Breakpoint b = breakpointById(id);
    QTC_ASSERT(b, return);
    BreakpointParameters params = b.parameters();
    destroyItem(b.b);
    appendBreakpoint(params);
}

Breakpoints BreakHandler::allBreakpoints() const
{
    Breakpoints items;
    forItemsAtLevel<1>([&items](BreakpointItem *b) { items.append(Breakpoint(b)); });
    return items;
}

Breakpoints BreakHandler::unclaimedBreakpoints() const
{
    return engineBreakpoints(0);
}

Breakpoints BreakHandler::engineBreakpoints(DebuggerEngine *engine) const
{
    Breakpoints items;
    forItemsAtLevel<1>([&items, engine](BreakpointItem *b) {
        if (b->m_engine == engine)
            items.append(Breakpoint(b));
    });
    return items;
}

QStringList BreakHandler::engineBreakpointPaths(DebuggerEngine *engine) const
{
    QSet<QString> set;
    forItemsAtLevel<1>([&set, engine](BreakpointItem *b) {
        if (b->m_engine == engine) {
            if (b->m_params.type == BreakpointByFileAndLine)
                set.insert(QFileInfo(b->m_params.fileName).dir().path());
        }
    });
    return set.toList();
}

void Breakpoint::setResponse(const BreakpointResponse &response)
{
    QTC_ASSERT(b, return);
    b->m_response = response;
    b->destroyMarker();
    b->updateMarker();
    // Take over corrected values from response.
    if ((b->m_params.type == BreakpointByFileAndLine
                || b->m_params.type == BreakpointByFunction)
            && !response.module.isEmpty())
        b->m_params.module = response.module;
}

bool Internal::Breakpoint::needsChange() const
{
    return b && b->needsChange();
}

void Breakpoint::changeBreakpointData(const BreakpointParameters &params)
{
    if (!b)
        return;
    if (params == b->m_params)
        return;
    b->m_params = params;
    if (b->m_engine)
        b->m_engine->updateBreakpointMarker(*this);
    b->destroyMarker();
    b->updateMarker();
    b->update();
    if (b->needsChange() && b->m_engine && b->m_state != BreakpointNew) {
        b->setState(BreakpointChangeRequested);
        b->m_handler->scheduleSynchronization();
    }
}

bool BreakHandler::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == BaseTreeView::ItemActivatedRole) {
        if (Breakpoint bp = findBreakpointByIndex(idx))
            bp.gotoLocation();
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = value.value<ItemViewEvent>();

        if (ev.as<QContextMenuEvent>())
            return contextMenuEvent(ev);

        if (auto kev = ev.as<QKeyEvent>(QEvent::KeyPress)) {
            if (kev->key() == Qt::Key_Delete) {
                QModelIndexList si = ev.currentOrSelectedRows();
                const Breakpoints ids = findBreakpointsByIndex(si);
//                int row = qMin(rowCount() - ids.size() - 1, idx.row());
                deleteBreakpoints(ids);
//                setCurrentIndex(index(row, 0));   FIXME
                return true;
            }
            if (kev->key() == Qt::Key_Space) {
                const QModelIndexList selectedIds = ev.selectedRows();
                if (!selectedIds.isEmpty()) {
                    const Breakpoints items = findBreakpointsByIndex(selectedIds);
                    const bool isEnabled = items.isEmpty() || items.at(0).isEnabled();
                    setBreakpointsEnabled(items, !isEnabled);
// FIXME
//                    foreach (const QModelIndex &id, selectedIds)
//                        update(id);
                    return true;
                }
            }
        }

        if (ev.as<QMouseEvent>(QEvent::MouseButtonDblClick)) {
            if (Breakpoint b = findBreakpointByIndex(idx)) {
                if (idx.column() >= BreakpointAddressColumn)
                    editBreakpoints({b}, ev.view());
                else
                    b.gotoLocation();
            } else {
                addBreakpoint();
            }
            return true;
        }
    }

    return false;
}

bool BreakHandler::contextMenuEvent(const ItemViewEvent &ev)
{
    const QModelIndexList selectedIndices = ev.selectedRows();
    const Breakpoints selectedItems = findBreakpointsByIndex(selectedIndices);
    const bool enabled = selectedItems.isEmpty() || selectedItems.at(0).isEnabled();

    auto menu = new QMenu;

    addAction(menu, tr("Add Breakpoint..."), true, [this] { addBreakpoint(); });

    addAction(menu, tr("Delete Selected Breakpoints"),
              !selectedItems.isEmpty(),
              [this, selectedItems] { deleteBreakpoints(selectedItems); });

    addAction(menu, tr("Edit Selected Breakpoints..."),
              !selectedItems.isEmpty(),
              [this, selectedItems, ev] { editBreakpoints(selectedItems, ev.view()); });


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
              selectedItems.size() > 1
                  ? enabled ? tr("Disable Selected Breakpoints") : tr("Enable Selected Breakpoints")
                  : enabled ? tr("Disable Breakpoint") : tr("Enable Breakpoint"),
              !selectedItems.isEmpty(),
              [this, selectedItems, enabled] { setBreakpointsEnabled(selectedItems, !enabled); });

    menu->addSeparator();

    addAction(menu, tr("Delete All Breakpoints"),
              rowCount() > 0,
              [this] { deleteAllBreakpoints(); });

    // Delete by file: Find indices of breakpoints of the same file.
    BreakpointItem *item = itemForIndexAtLevel<1>(ev.index());
    Breakpoints breakpointsInFile;
    QString file;
    if (item) {
        const QModelIndex index = ev.index().sibling(ev.index().row(), BreakpointFileColumn);
        if (!file.isEmpty()) {
            for (int i = 0; i != rowCount(); ++i)
                if (index.data().toString() == file)
                    breakpointsInFile.append(findBreakpointByIndex(index));
        }
    }
    addAction(menu, tr("Delete Breakpoints of \"%1\"").arg(file),
              tr("Delete Breakpoints of File"),
              breakpointsInFile.size() > 1,
              [this, breakpointsInFile] { deleteBreakpoints(breakpointsInFile); });

    menu->addSeparator();

    addAction(menu, tr("Synchronize Breakpoints"),
              Internal::hasSnapshots(),
              [this] { Internal::synchronizeBreakpoints(); });

    menu->addSeparator();
    menu->addAction(action(UseToolTipsInBreakpointsView));
    if (currentEngine()->hasCapability(MemoryAddressCapability))
        menu->addAction(action(UseAddressInBreakpointsView));
    menu->addSeparator();
    menu->addAction(action(SettingsDialog));

    menu->popup(ev.globalPos());

    return true;
}

void BreakHandler::setBreakpointsEnabled(const Breakpoints &bps, bool enabled)
{
    foreach (Breakpoint b, bps)
        b.setEnabled(enabled);
}

void BreakHandler::deleteAllBreakpoints()
{
    QDialogButtonBox::StandardButton pressed =
        CheckableMessageBox::doNotAskAgainQuestion(ICore::dialogParent(),
           tr("Remove All Breakpoints"),
           tr("Are you sure you want to remove all breakpoints "
              "from all files in the current session?"),
           ICore::settings(),
           "RemoveAllBreakpoints");
    if (pressed == QDialogButtonBox::Yes)
        deleteBreakpoints(breakHandler()->allBreakpoints());
}

void BreakHandler::deleteBreakpoints(const Breakpoints &bps)
{
    foreach (Breakpoint bp, bps)
        bp.removeBreakpoint();
}

void BreakHandler::editBreakpoint(Breakpoint bp, QWidget *parent)
{
    BreakpointParameters data = bp.parameters();
    BreakpointParts parts = NoParts;

    BreakpointDialog dialog(bp, parent);
    if (!dialog.showDialog(&data, &parts))
        return;

    bp.changeBreakpointData(data);
}

void BreakHandler::addBreakpoint()
{
    BreakpointParameters data(BreakpointByFileAndLine);
    BreakpointParts parts = NoParts;
    BreakpointDialog dialog(Breakpoint(), ICore::dialogParent());
    dialog.setWindowTitle(tr("Add Breakpoint"));
    if (dialog.showDialog(&data, &parts))
        appendBreakpoint(data);
}

void BreakHandler::editBreakpoints(const Breakpoints &bps, QWidget *parent)
{
    QTC_ASSERT(!bps.isEmpty(), return);

    const Breakpoint bp = bps.at(0);

    if (bps.size() == 1) {
        editBreakpoint(bp, parent);
        return;
    }

    // This allows to change properties of multiple breakpoints at a time.
    if (!bp)
        return;

    MultiBreakPointsDialog dialog;
    dialog.setCondition(bp.condition());
    dialog.setIgnoreCount(bp.ignoreCount());
    dialog.setThreadSpec(bp.threadSpec());

    if (dialog.exec() == QDialog::Rejected)
        return;

    const QString newCondition = dialog.condition();
    const int newIgnoreCount = dialog.ignoreCount();
    const int newThreadSpec = dialog.threadSpec();

    foreach (Breakpoint bp, bps) {
        if (bp) {
            bp.setCondition(newCondition);
            bp.setIgnoreCount(newIgnoreCount);
            bp.setThreadSpec(newThreadSpec);
        }
    }
}

//////////////////////////////////////////////////////////////////
//
// Storage
//
//////////////////////////////////////////////////////////////////

// Ok to be not thread-safe. The order does not matter and only the gui
// produces authoritative ids.
static int currentId = 0;

BreakpointItem::BreakpointItem(BreakHandler *handler)
    : m_handler(handler), m_id(++currentId), m_state(BreakpointNew), m_engine(0), m_marker(0)
{}

BreakpointItem::~BreakpointItem()
{
    delete m_marker;
}

void BreakpointItem::destroyMarker()
{
    if (m_marker) {
        BreakpointMarker *m = m_marker;
        m->m_bp = 0;
        m_marker = 0;
        delete m;
    }
}

QString BreakpointItem::markerFileName() const
{
    // Some heuristics to find a "good" file name.
    if (!m_params.fileName.isEmpty()) {
        QFileInfo fi(m_params.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    if (!m_response.fileName.isEmpty()) {
        QFileInfo fi(m_response.fileName);
        if (fi.exists())
            return fi.absoluteFilePath();
    }
    if (m_response.fileName.endsWith(m_params.fileName))
        return m_response.fileName;
    if (m_params.fileName.endsWith(m_response.fileName))
        return m_params.fileName;
    return m_response.fileName.size() > m_params.fileName.size()
        ? m_response.fileName : m_params.fileName;
}

int BreakpointItem::markerLineNumber() const
{
    return m_response.lineNumber ? m_response.lineNumber : m_params.lineNumber;
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
    if (!m_params.conditionsMatch(m_response.condition))
        return true;
    if (m_params.ignoreCount != m_response.ignoreCount)
        return true;
    if (m_params.enabled != m_response.enabled)
        return true;
    if (m_params.threadSpec != m_response.threadSpec)
        return true;
    if (m_params.command != m_response.command)
        return true;
    if (m_params.type == BreakpointByFileAndLine && m_params.lineNumber != m_response.lineNumber)
        return true;
    // FIXME: Too strict, functions may have parameter lists, or not.
    // if (m_params.type == BreakpointByFunction && m_params.functionName != m_response.functionName)
    //     return true;
    // if (m_params.type == BreakpointByAddress && m_params.address != m_response.address)
    //     return true;
    return false;
}

bool BreakpointItem::isLocatedAt
    (const QString &fileName, int lineNumber, bool useMarkerPosition) const
{
    int line = useMarkerPosition ? m_response.lineNumber : m_params.lineNumber;
    return lineNumber == line
        && (fileNameMatch(fileName, m_response.fileName)
            || fileNameMatch(fileName, markerFileName()));
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
    QString file = markerFileName();
    int line = markerLineNumber();
    if (m_marker && (file != m_marker->fileName() || line != m_marker->lineNumber()))
        destroyMarker();

    if (!m_marker && !file.isEmpty() && line > 0)
        m_marker = new BreakpointMarker(this, file, line);

    if (m_marker) {
        QString toolTip;
        auto addToToolTipText = [&toolTip](const QString &info, const QString &label) {
            if (info.isEmpty())
                return;
            if (!toolTip.isEmpty())
                toolTip += ' ';
            toolTip += label + ": '" + info + '\'';
        };
        addToToolTipText(m_params.condition, tr("Breakpoint Condition"));
        addToToolTipText(m_params.command, tr("Debugger Command"));
        m_marker->setToolTip(toolTip);
    }
}

QIcon BreakpointItem::icon() const
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
    if (m_state == BreakpointInserted && !m_response.pending)
        return Icons::BREAKPOINT.icon();
    return Icons::BREAKPOINT_PENDING.icon();
}

QString BreakpointItem::toToolTip() const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>"
        //<< "<tr><td>" << tr("ID:") << "</td><td>" << m_id << "</td></tr>"
        << "<tr><td>" << tr("State:")
        << "</td><td>" << (m_params.enabled ? tr("Enabled") : tr("Disabled"));
    if (m_response.pending)
        str << tr(", pending");
    str << ", " << m_state << "   (" << stateToString(m_state) << ")</td></tr>";
    if (m_engine) {
        str << "<tr><td>" << tr("Engine:")
        << "</td><td>" << m_engine->objectName() << "</td></tr>";
    }
    if (!m_response.pending) {
        str << "<tr><td>" << tr("Breakpoint Number:")
            << "</td><td>" << m_response.id.toString() << "</td></tr>";
    }
    str << "<tr><td>" << tr("Breakpoint Type:")
        << "</td><td>" << typeToString(m_params.type) << "</td></tr>"
        << "<tr><td>" << tr("Marker File:")
        << "</td><td>" << QDir::toNativeSeparators(markerFileName()) << "</td></tr>"
        << "<tr><td>" << tr("Marker Line:")
        << "</td><td>" << markerLineNumber() << "</td></tr>"
        << "<tr><td>" << tr("Hit Count:")
        << "</td><td>" << m_response.hitCount << "</td></tr>"
        << "</table><br><hr><table>"
        << "<tr><th>" << tr("Property")
        << "</th><th>" << tr("Requested")
        << "</th><th>" << tr("Obtained") << "</th></tr>"
        << "<tr><td>" << tr("Internal Number:")
        << "</td><td>&mdash;</td><td>" << m_response.id.toString() << "</td></tr>";
    if (m_params.type == BreakpointByFunction) {
        str << "<tr><td>" << tr("Function Name:")
        << "</td><td>" << m_params.functionName
        << "</td><td>" << m_response.functionName
        << "</td></tr>";
    }
    if (m_params.type == BreakpointByFileAndLine) {
    str << "<tr><td>" << tr("File Name:")
        << "</td><td>" << QDir::toNativeSeparators(m_params.fileName)
        << "</td><td>" << QDir::toNativeSeparators(m_response.fileName)
        << "</td></tr>"
        << "<tr><td>" << tr("Line Number:")
        << "</td><td>" << m_params.lineNumber
        << "</td><td>" << m_response.lineNumber << "</td></tr>"
        << "<tr><td>" << tr("Corrected Line Number:")
        << "</td><td>-"
        << "</td><td>" << m_response.correctedLineNumber << "</td></tr>";
    }
    if (m_params.type == BreakpointByFunction || m_params.type == BreakpointByFileAndLine) {
        str << "<tr><td>" << tr("Module:")
            << "</td><td>" << m_params.module
            << "</td><td>" << m_response.module
            << "</td></tr>";
    }
    str << "<tr><td>" << tr("Breakpoint Address:")
        << "</td><td>";
    formatAddress(str, m_params.address);
    str << "</td><td>";
    formatAddress(str, m_response.address);
    str << "</td></tr>";
    if (m_response.multiple) {
        str << "<tr><td>" << tr("Multiple Addresses:")
            << "</td><td>"
            << "</td></tr>";
    }
    if (!m_params.command.isEmpty() || !m_response.command.isEmpty()) {
        str << "<tr><td>" << tr("Command:")
            << "</td><td>" << m_params.command
            << "</td><td>" << m_response.command
            << "</td></tr>";
    }
    if (!m_params.message.isEmpty() || !m_response.message.isEmpty()) {
        str << "<tr><td>" << tr("Message:")
            << "</td><td>" << m_params.message
            << "</td><td>" << m_response.message
            << "</td></tr>";
    }
    if (!m_params.condition.isEmpty() || !m_response.condition.isEmpty()) {
        str << "<tr><td>" << tr("Condition:")
            << "</td><td>" << m_params.condition
            << "</td><td>" << m_response.condition
            << "</td></tr>";
    }
    if (m_params.ignoreCount || m_response.ignoreCount) {
        str << "<tr><td>" << tr("Ignore Count:") << "</td><td>";
        if (m_params.ignoreCount)
            str << m_params.ignoreCount;
        str << "</td><td>";
        if (m_response.ignoreCount)
            str << m_response.ignoreCount;
        str << "</td></tr>";
    }
    if (m_params.threadSpec >= 0 || m_response.threadSpec >= 0) {
        str << "<tr><td>" << tr("Thread Specification:")
            << "</td><td>";
        if (m_params.threadSpec >= 0)
            str << m_params.threadSpec;
        str << "</td><td>";
        if (m_response.threadSpec >= 0)
            str << m_response.threadSpec;
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
    appendBreakpoint(params);
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
    appendBreakpoint(params);
}

bool Breakpoint::isValid() const
{
    return b && b->m_id.isValid();
}

uint Breakpoint::hash() const
{
    return b ? 0 : qHash(b->m_id);
}

BreakpointModelId Breakpoint::id() const
{
    return b ? b->m_id : BreakpointModelId();
}

QString Breakpoint::msgWatchpointByExpressionTriggered(const int number, const QString &expr) const
{
    return id()
        ? tr("Data breakpoint %1 (%2) at %3 triggered.")
            .arg(id().toString()).arg(number).arg(expr)
        : tr("Internal data breakpoint %1 at %2 triggered.")
            .arg(number).arg(expr);
}

QString Breakpoint::msgWatchpointByExpressionTriggered(const int number, const QString &expr,
                                                       const QString &threadId) const
{
    return id()
        ? tr("Data breakpoint %1 (%2) at %3 in thread %4 triggered.")
            .arg(id().toString()).arg(number).arg(expr).arg(threadId)
        : tr("Internal data breakpoint %1 at %2 in thread %3 triggered.")
            .arg(number).arg(expr).arg(threadId);
}

QString Breakpoint::msgWatchpointByAddressTriggered(const int number, quint64 address) const
{
    return id()
        ? tr("Data breakpoint %1 (%2) at 0x%3 triggered.")
            .arg(id().toString()).arg(number).arg(address, 0, 16)
        : tr("Internal data breakpoint %1 at 0x%2 triggered.")
            .arg(number).arg(address, 0, 16);
}

QString Breakpoint::msgWatchpointByAddressTriggered(
    const int number, quint64 address, const QString &threadId) const
{
    return id()
        ? tr("Data breakpoint %1 (%2) at 0x%3 in thread %4 triggered.")
            .arg(id().toString()).arg(number).arg(address, 0, 16).arg(threadId)
        : tr("Internal data breakpoint %1 at 0x%2 in thread %3 triggered.")
            .arg(id().toString()).arg(number).arg(address, 0, 16).arg(threadId);
}

QString Breakpoint::msgBreakpointTriggered(const int number, const QString &threadId) const
{
    return id()
        ? tr("Stopped at breakpoint %1 (%2) in thread %3.")
            .arg(id().toString()).arg(number).arg(threadId)
        : tr("Stopped at internal breakpoint %1 in thread %2.")
            .arg(number).arg(threadId);
}

} // namespace Internal
} // namespace Debugger
