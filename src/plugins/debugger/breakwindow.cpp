/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "breakwindow.h"
#include "breakhandler.h"
#include "debuggerengine.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "ui_breakpoint.h"
#include "ui_breakcondition.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QIntValidator>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>

namespace Debugger {
namespace Internal {


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
    explicit BreakpointDialog(unsigned engineCapabilities, QWidget *parent = 0);
    bool showDialog(BreakpointParameters *data);

    void setParameters(const BreakpointParameters &data);
    BreakpointParameters parameters() const;

public slots:
    void typeChanged(int index);

private:
    enum DialogPart {
        FileAndLinePart = 0x1,
        FunctionPart = 0x2,
        AddressPart = 0x4,
        ConditionPart = 0x8,
        IgnoreCountPart = 0x10,
        ThreadSpecPart = 0x20,
        AllConditionParts = ConditionPart|IgnoreCountPart|ThreadSpecPart,
        ModulePart = 0x40,
        TracePointPart = 0x80,
        AllParts = FileAndLinePart|FunctionPart|AddressPart|ConditionPart
                   |IgnoreCountPart|ThreadSpecPart|ModulePart|TracePointPart
    };

    void setPartsEnabled(unsigned partsMask);
    void clearOtherParts(unsigned partsMask);
    void getParts(unsigned partsMask, BreakpointParameters *data) const;
    void setParts(unsigned partsMask, const BreakpointParameters &data);

    void setType(BreakpointType type);
    BreakpointType type() const;

    unsigned m_enabledParts;
    Ui::BreakpointDialog m_ui;
    BreakpointParameters m_savedParameters;
    BreakpointType m_previousType;
    bool m_firstTypeChange;
};

BreakpointDialog::BreakpointDialog(unsigned engineCapabilities, QWidget *parent)
    : QDialog(parent), m_enabledParts(-1), m_previousType(UnknownType),
      m_firstTypeChange(true)
{
    if (!(engineCapabilities & BreakConditionCapability))
        m_enabledParts &= ~ConditionPart;
    if (!(engineCapabilities & BreakModuleCapability))
        m_enabledParts &= ~ModulePart;
    if (!(engineCapabilities & TracePointCapability))
        m_enabledParts &= ~TracePointPart;
    // Match BreakpointType (omitting unknown type).
    m_ui.setupUi(this);
    QStringList types;
    types << tr("File and Line Number") << tr("Function Name") << tr("Address")
          << tr("throw") << tr("catch") << tr("Function \"main()\"")
          << tr("Address (Watchpoint)");
    QTC_ASSERT(types.size() == Watchpoint, return; )
    m_ui.comboBoxType->addItems(types);
    m_ui.pathChooserFileName->setExpectedKind(Utils::PathChooser::File);
    connect(m_ui.comboBoxType, SIGNAL(activated(int)), SLOT(typeChanged(int)));
    const QString moduleToolTip =
        tr("Specifying the module (base name of the library or executable)\n"
           "for function or file type breakpoints can significantly speed up\n"
           "debugger start-up times (CDB, LLDB).");
    m_ui.labelModule->setToolTip(moduleToolTip);
    m_ui.lineEditModule->setToolTip(moduleToolTip);
    const QString commandToolTip =
        tr("Debugger command to be executed when the breakpoint is hit.\n"
           "gdb allows for specifying a sequence of commands separated by the delimiter '\\n'.");
    m_ui.lineEditCommand->setToolTip(commandToolTip);
    m_ui.labelCommand->setToolTip(commandToolTip);
    m_ui.spinBoxIgnoreCount->setMinimum(0);
    m_ui.spinBoxIgnoreCount->setMaximum(2147483647);
    const QString pathToolTip =
            tr("<html><head/><body><p>Determines how the path is specified when setting breakpoints:</p><ul>"
               "<li><i>Use Engine Default</i>: Preferred setting of the debugger engine.</li>"
               "<li><i>Use Full Path</i>: Pass full path, avoiding ambiguities should files of the same "
               "name exist in several modules. This is the engine default for CDB and LLDB.</li>"
               "<li><i>Use File Name</i>: Pass the file name only. This is useful "
               "when using a source tree whose location does not match the one used when building the modules. "
               "It is the engine default for gdb as using full paths can be slow with this engine.</li>"
               "</ul></body></html>");
    m_ui.labelUseFullPath->setToolTip(pathToolTip);
    m_ui.comboBoxPathUsage->setToolTip(pathToolTip);
}

void BreakpointDialog::setType(BreakpointType type)
{
    const int comboIndex = type - 1; // Skip UnknownType.
    if (comboIndex != m_ui.comboBoxType->currentIndex() || m_firstTypeChange) {
        m_ui.comboBoxType->setCurrentIndex(comboIndex);
        typeChanged(comboIndex);
        m_firstTypeChange = false;
    }
}

BreakpointType BreakpointDialog::type() const
{
    const int type = m_ui.comboBoxType->currentIndex() + 1; // Skip unknown type.
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
    m_ui.labelFileName->setEnabled(partsMask & FileAndLinePart);
    m_ui.pathChooserFileName->setEnabled(partsMask & FileAndLinePart);
    m_ui.labelLineNumber->setEnabled(partsMask & FileAndLinePart);
    m_ui.lineEditLineNumber->setEnabled(partsMask & FileAndLinePart);
    m_ui.labelUseFullPath->setEnabled(partsMask & FileAndLinePart);
    m_ui.comboBoxPathUsage->setEnabled(partsMask & FileAndLinePart);

    m_ui.labelFunction->setEnabled(partsMask & FunctionPart);
    m_ui.lineEditFunction->setEnabled(partsMask & FunctionPart);

    m_ui.labelAddress->setEnabled(partsMask & AddressPart);
    m_ui.lineEditAddress->setEnabled(partsMask & AddressPart);

    m_ui.labelCondition->setEnabled(partsMask & ConditionPart);
    m_ui.lineEditCondition->setEnabled(partsMask & ConditionPart);
    m_ui.labelIgnoreCount->setEnabled(partsMask & IgnoreCountPart);
    m_ui.spinBoxIgnoreCount->setEnabled(partsMask & IgnoreCountPart);
    m_ui.labelThreadSpec->setEnabled(partsMask & ThreadSpecPart);
    m_ui.lineEditThreadSpec->setEnabled(partsMask & ThreadSpecPart);

    m_ui.labelModule->setEnabled(partsMask & ModulePart);
    m_ui.lineEditModule->setEnabled(partsMask & ModulePart);

    m_ui.labelTracepoint->setEnabled(partsMask & TracePointPart);
    m_ui.checkBoxTracepoint->setEnabled(partsMask & TracePointPart);
}

void BreakpointDialog::clearOtherParts(unsigned partsMask)
{
    const unsigned invertedPartsMask = ~partsMask;
    if (invertedPartsMask & FileAndLinePart) {
        m_ui.pathChooserFileName->setPath(QString());
        m_ui.lineEditLineNumber->clear();
        m_ui.comboBoxPathUsage->setCurrentIndex(BreakpointPathUsageEngineDefault);
    }

    if (invertedPartsMask & FunctionPart)
        m_ui.lineEditFunction->clear();

    if (invertedPartsMask & AddressPart)
        m_ui.lineEditAddress->clear();

    if (invertedPartsMask & ConditionPart)
        m_ui.lineEditCondition->clear();
    if (invertedPartsMask & IgnoreCountPart)
        m_ui.spinBoxIgnoreCount->clear();
    if (invertedPartsMask & ThreadSpecPart)
        m_ui.lineEditThreadSpec->clear();
    if (invertedPartsMask & ModulePart)
        m_ui.lineEditModule->clear();

    if (invertedPartsMask & TracePointPart)
        m_ui.checkBoxTracepoint->setChecked(false);
}

void BreakpointDialog::getParts(unsigned partsMask, BreakpointParameters *data) const
{
    data->enabled = m_ui.checkBoxEnabled->isChecked();
    data->command = m_ui.lineEditCommand->text().trimmed();

    if (partsMask & FileAndLinePart) {
        data->lineNumber = m_ui.lineEditLineNumber->text().toInt();
        data->pathUsage = static_cast<BreakpointPathUsage>(m_ui.comboBoxPathUsage->currentIndex());
        data->fileName = m_ui.pathChooserFileName->path();
    }
    if (partsMask & FunctionPart)
        data->functionName = m_ui.lineEditFunction->text();

    if (partsMask & AddressPart)
        data->address = m_ui.lineEditAddress->text().toULongLong(0, 0);

    if (partsMask & ConditionPart)
        data->condition = m_ui.lineEditCondition->text().toUtf8();
    if (partsMask & IgnoreCountPart)
        data->ignoreCount = m_ui.spinBoxIgnoreCount->text().toInt();
    if (partsMask & ThreadSpecPart)
        data->threadSpec =
            BreakHandler::threadSpecFromDisplay(m_ui.lineEditThreadSpec->text());
    if (partsMask & ModulePart)
        data->module = m_ui.lineEditModule->text();

    if (partsMask & TracePointPart)
        data->tracepoint = m_ui.checkBoxTracepoint->isChecked();
}

void BreakpointDialog::setParts(unsigned mask, const BreakpointParameters &data)
{
    m_ui.checkBoxEnabled->setChecked(data.enabled);
    m_ui.comboBoxPathUsage->setCurrentIndex(data.pathUsage);
    m_ui.lineEditCommand->setText(data.command);

    if (mask & FileAndLinePart) {
        m_ui.pathChooserFileName->setPath(data.fileName);
        m_ui.lineEditLineNumber->setText(QString::number(data.lineNumber));
    }

    if (mask & FunctionPart)
        m_ui.lineEditFunction->setText(data.functionName);

    if (mask & AddressPart) {
        if (data.address) {
            m_ui.lineEditAddress->setText(
                QString::fromAscii("0x%1").arg(data.address, 0, 16));
        } else {
            m_ui.lineEditAddress->clear();
        }
    }

    if (mask & ConditionPart)
        m_ui.lineEditCondition->setText(QString::fromUtf8(data.condition));
    if (mask & IgnoreCountPart)
        m_ui.spinBoxIgnoreCount->setValue(data.ignoreCount);
    if (mask & ThreadSpecPart)
        m_ui.lineEditThreadSpec->
            setText(BreakHandler::displayFromThreadSpec(data.threadSpec));
    if (mask & ModulePart)
        m_ui.lineEditModule->setText(data.module);

    if (mask & TracePointPart)
        m_ui.checkBoxTracepoint->setChecked(data.tracepoint);
}

void BreakpointDialog::typeChanged(int)
{
    BreakpointType previousType = m_previousType;
    const BreakpointType newType = type();
    m_previousType = newType;
    // Save current state.
    switch(previousType) {
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
        break;
    case BreakpointByAddress:
    case Watchpoint:
        getParts(AddressPart|AllConditionParts|TracePointPart, &m_savedParameters);
        break;
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
        clearOtherParts(AllConditionParts|ModulePart|TracePointPart);
        setPartsEnabled(AllConditionParts|TracePointPart);
        break;
    case BreakpointAtMain:
        m_ui.lineEditFunction->setText(QLatin1String("main")); // Just for display
        clearOtherParts(0);
        setPartsEnabled(0);
        break;
    case BreakpointByAddress:
    case Watchpoint:
        setParts(AddressPart|AllConditionParts|TracePointPart, m_savedParameters);
        setPartsEnabled(AddressPart|AllConditionParts|TracePointPart|TracePointPart);
        clearOtherParts(AddressPart|AllConditionParts|TracePointPart);
        break;
    }
}

bool BreakpointDialog::showDialog(BreakpointParameters *data)
{
    setParameters(*data);
    if (exec() != QDialog::Accepted)
        return false;

    // Check if changed.
    const BreakpointParameters newParameters = parameters();
    if (data->equals(newParameters))
        return false;

    *data = newParameters;
    return true;
}

// Dialog allowing changing properties of multiple breakpoints at a time.
class MultiBreakPointsDialog : public QDialog {
    Q_OBJECT
public:
    explicit MultiBreakPointsDialog(unsigned engineCapabilities = AllDebuggerCapabilities, QWidget *parent = 0);

    QString condition() const { return m_ui.lineEditCondition->text(); }
    int ignoreCount() const { return m_ui.spinBoxIgnoreCount->value(); }
    int threadSpec() const
       { return BreakHandler::threadSpecFromDisplay(m_ui.lineEditThreadSpec->text()); }

    void setCondition(const QString &c) { m_ui.lineEditCondition->setText(c); }
    void setIgnoreCount(int i) { m_ui.spinBoxIgnoreCount->setValue(i); }
    void setThreadSpec(int t)
        { return m_ui.lineEditThreadSpec->setText(BreakHandler::displayFromThreadSpec(t)); }

private:
    Ui::BreakCondition m_ui;
};

MultiBreakPointsDialog::MultiBreakPointsDialog(unsigned engineCapabilities,QWidget *parent) :
    QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui.setupUi(this);
    setWindowTitle(tr("Edit Breakpoint Properties"));
    m_ui.spinBoxIgnoreCount->setMinimum(0);
    m_ui.spinBoxIgnoreCount->setMaximum(2147483647);
    if (!(engineCapabilities & BreakConditionCapability)) {
        m_ui.labelCondition->setEnabled(false);
        m_ui.lineEditCondition->setEnabled(false);
    }
}

///////////////////////////////////////////////////////////////////////
//
// BreakWindow
//
///////////////////////////////////////////////////////////////////////

BreakWindow::BreakWindow(QWidget *parent)
  : QTreeView(parent)
{
    m_alwaysResizeColumnsToContents = false;

    QAction *act = debuggerCore()->action(UseAlternatingRowColors);
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setWindowTitle(tr("Breakpoints"));
    setWindowIcon(QIcon(QLatin1String(":/debugger/images/debugger_breakpoints.png")));
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(this, SIGNAL(activated(QModelIndex)),
        SLOT(rowActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
    connect(debuggerCore()->action(UseAddressInBreakpointsView),
        SIGNAL(toggled(bool)),
        SLOT(showAddressColumn(bool)));
}

void BreakWindow::showAddressColumn(bool on)
{
    setColumnHidden(7, !on);
}

void BreakWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Delete) {
        QItemSelectionModel *sm = selectionModel();
        QTC_ASSERT(sm, return);
        QModelIndexList si = sm->selectedIndexes();
        if (si.isEmpty())
            si.append(currentIndex());
        const BreakpointIds ids = breakHandler()->findBreakpointsByIndex(si);
        int row = qMin(model()->rowCount() - ids.size() - 1, currentIndex().row());
        deleteBreakpoints(ids);
        setCurrentIndex(si.at(0).sibling(row, 0));
    }
    QTreeView::keyPressEvent(ev);
}

void BreakWindow::resizeEvent(QResizeEvent *ev)
{
    QTreeView::resizeEvent(ev);
}

void BreakWindow::mouseDoubleClickEvent(QMouseEvent *ev)
{
    QModelIndex indexUnderMouse = indexAt(ev->pos());
    if (indexUnderMouse.isValid() && indexUnderMouse.column() >= 4) {
        BreakpointId id = breakHandler()->findBreakpointByIndex(indexUnderMouse);
        editBreakpoints(BreakpointIds() << id);
    }
    QTreeView::mouseDoubleClickEvent(ev);
}

void BreakWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    resizeColumnToContents(0); // Number
    resizeColumnToContents(3); // Line
    resizeColumnToContents(6); // Ignore count
}

void BreakWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QItemSelectionModel *sm = selectionModel();
    QTC_ASSERT(sm, return);
    QModelIndexList selectedIndices = sm->selectedIndexes();
    QModelIndex indexUnderMouse = indexAt(ev->pos());
    if (selectedIndices.isEmpty() && indexUnderMouse.isValid())
        selectedIndices.append(indexUnderMouse);

    BreakHandler *handler = breakHandler();
    BreakpointIds selectedIds = handler->findBreakpointsByIndex(selectedIndices);

    const int rowCount = model()->rowCount();
    unsigned engineCapabilities = AllDebuggerCapabilities;
    if (!selectedIndices.isEmpty())
        if (const unsigned ec = model()->data(selectedIndices.front(), EngineCapabilitiesRole).toUInt())
            engineCapabilities = ec;

    QAction *deleteAction = new QAction(tr("Delete Breakpoint"), &menu);
    deleteAction->setEnabled(!selectedIds.isEmpty());

    QAction *deleteAllAction = new QAction(tr("Delete All Breakpoints"), &menu);
    deleteAllAction->setEnabled(model()->rowCount() > 0);

    // Delete by file: Find indices of breakpoints of the same file.
    QAction *deleteByFileAction = 0;
    BreakpointIds breakpointsInFile;
    if (indexUnderMouse.isValid()) {
        const QModelIndex index = indexUnderMouse.sibling(indexUnderMouse.row(), 2);
        const QString file = index.data().toString();
        if (!file.isEmpty()) {
            for (int i = 0; i < rowCount; i++)
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

    QAction *alwaysAdjustAction =
        new QAction(tr("Always Adjust Column Widths to Contents"), &menu);

    alwaysAdjustAction->setCheckable(true);
    alwaysAdjustAction->setChecked(m_alwaysResizeColumnsToContents);

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
    QAction *breakAtThrowAction =
        new QAction(tr("Set Breakpoint at \"throw\""), this);
    QAction *breakAtCatchAction =
        new QAction(tr("Set Breakpoint at \"catch\""), this);

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
    if (engineCapabilities & BreakOnThrowAndCatchCapability) {
        menu.addSeparator();
        menu.addAction(breakAtThrowAction);
        menu.addAction(breakAtCatchAction);
    }
    menu.addSeparator();
    menu.addAction(debuggerCore()->action(UseToolTipsInBreakpointsView));
    menu.addAction(debuggerCore()->action(UseAddressInBreakpointsView));
    menu.addAction(adjustColumnAction);
    menu.addAction(alwaysAdjustAction);
    menu.addSeparator();
    menu.addAction(debuggerCore()->action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == deleteAction)
        deleteBreakpoints(selectedIds);
    else if (act == deleteAllAction)
        deleteBreakpoints(handler->allBreakpointIds());
    else if (act == deleteByFileAction)
        deleteBreakpoints(breakpointsInFile);
    else if (act == adjustColumnAction)
        resizeColumnsToContents();
    else if (act == alwaysAdjustAction)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
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
    else if (act == breakAtThrowAction)
        handler->appendBreakpoint(BreakpointParameters(BreakpointAtThrow));
    else if (act == breakAtCatchAction)
        handler->appendBreakpoint(BreakpointParameters(BreakpointAtCatch));
}

void BreakWindow::setBreakpointsEnabled(const BreakpointIds &ids, bool enabled)
{
    BreakHandler *handler = breakHandler();
    foreach (const BreakpointId id, ids)
        handler->setEnabled(id, enabled);
}

void BreakWindow::deleteBreakpoints(const BreakpointIds &ids)
{
    BreakHandler *handler = breakHandler();
    foreach (const BreakpointId id, ids)
       handler->removeBreakpoint(id);
}

void BreakWindow::editBreakpoint(BreakpointId id, QWidget *parent)
{
    BreakpointParameters data = breakHandler()->breakpointData(id);
    unsigned engineCapabilities = AllDebuggerCapabilities;
    if (const DebuggerEngine *engine = breakHandler()->engine(id))
        engineCapabilities = engine->debuggerCapabilities();
    BreakpointDialog dialog(engineCapabilities, parent);
    if (dialog.showDialog(&data))
        breakHandler()->setBreakpointData(id, data);
}

void BreakWindow::addBreakpoint()
{
    BreakpointParameters data(BreakpointByFileAndLine);
    BreakpointDialog dialog(AllDebuggerCapabilities, this);
    dialog.setWindowTitle(tr("Add Breakpoint"));
    if (dialog.showDialog(&data))
        breakHandler()->appendBreakpoint(data);
}

void BreakWindow::editBreakpoints(const BreakpointIds &ids)
{
    QTC_ASSERT(!ids.isEmpty(), return);

    const BreakpointId id = ids.at(0);

    if (ids.size() == 1) {
        editBreakpoint(id, this);
        return;
    }

    // This allows to change properties of multiple breakpoints at a time.
    BreakHandler *handler = breakHandler();
    unsigned engineCapabilities = AllDebuggerCapabilities;
    if (const DebuggerEngine *engine = breakHandler()->engine(id))
        engineCapabilities = engine->debuggerCapabilities();
    MultiBreakPointsDialog dialog(engineCapabilities);
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

    foreach (const BreakpointId id, ids) {
        handler->setCondition(id, newCondition.toLatin1());
        handler->setIgnoreCount(id, newIgnoreCount);
        handler->setThreadSpec(id, newThreadSpec);
    }
}

void BreakWindow::associateBreakpoint(const BreakpointIds &ids, int threadId)
{
    BreakHandler *handler = breakHandler();
    foreach (const BreakpointId id, ids)
        handler->setThreadSpec(id, threadId);
}

void BreakWindow::resizeColumnsToContents()
{
    for (int i = model()->columnCount(); --i >= 0; )
        resizeColumnToContents(i);
}

void BreakWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    for (int i = model()->columnCount(); --i >= 0; )
        header()->setResizeMode(i, mode);
}

void BreakWindow::rowActivated(const QModelIndex &index)
{
    breakHandler()->gotoLocation(breakHandler()->findBreakpointByIndex(index));
}

} // namespace Internal
} // namespace Debugger

#include "breakwindow.moc"
