/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "breakwindow.h"
#include "breakhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "ui_breakpoint.h"
#include "ui_breakcondition.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QHeaderView>
#include <QtGui/QIntValidator>
#include <QtGui/QItemSelectionModel>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>
#include <QtGui/QToolButton>
#include <QtGui/QTreeView>


namespace Debugger {
namespace Internal {


///////////////////////////////////////////////////////////////////////
//
// BreakpointDialog
//
///////////////////////////////////////////////////////////////////////

class BreakpointDialog : public QDialog, public Ui::BreakpointDialog
{
    Q_OBJECT
public:
    explicit BreakpointDialog(QWidget *parent);
    bool showDialog(BreakpointData *data);

    void setParameters(const BreakpointParameters &p);
    BreakpointParameters parameters() const;

public slots:
    void typeChanged(int index);

private:
    void setType(BreakpointType type);
    BreakpointType type() const;
};

BreakpointDialog::BreakpointDialog(QWidget *parent) : QDialog(parent)
{
    // match BreakpointType (except unknown type) with additional item
    setupUi(this);
    QStringList types;
    types << tr("File and Line Number") << tr("Function Name") << tr("Address")
          << tr("throw") << tr("catch") << tr("Function \"main()\"")
          << tr("Address (Watchpoint)");
    QTC_ASSERT(types.size() == Watchpoint, return; )
    comboBoxType->addItems(types);
    pathChooserFileName->setExpectedKind(Utils::PathChooser::File);
    connect(comboBoxType, SIGNAL(activated(int)), SLOT(typeChanged(int)));
    lineEditIgnoreCount->setValidator(
        new QIntValidator(0, 2147483647, lineEditIgnoreCount));
}

void BreakpointDialog::setType(BreakpointType type)
{
    const int comboIndex = type - 1; // Skip UnknownType
    if (comboIndex != comboBoxType->currentIndex()) {
        comboBoxType->setCurrentIndex(comboIndex);
        typeChanged(comboIndex);
    }
}

BreakpointType BreakpointDialog::type() const
{
    const int type = comboBoxType->currentIndex() + 1; // Skip unknown type
    return static_cast<BreakpointType>(type);
}

void BreakpointDialog::setParameters(const BreakpointParameters &p)
{
    pathChooserFileName->setPath(p.fileName);
    lineEditLineNumber->setText(QString::number(p.lineNumber));
    lineEditFunction->setText(p.functionName);
    lineEditCondition->setText(QString::fromUtf8(p.condition));
    lineEditIgnoreCount->setText(QString::number(p.ignoreCount));
    checkBoxUseFullPath->setChecked(p.useFullPath);
    lineEditThreadSpec->setText(p.threadSpec);
    const quint64 address = p.address;
    if (address)
        lineEditAddress->setText(QString::fromAscii("0x%1").arg(address, 0, 16));
    setType(p.type);
}

BreakpointParameters BreakpointDialog::parameters() const
{
    BreakpointParameters rc(type());
    rc.lineNumber = lineEditLineNumber->text().toInt();
    rc.useFullPath = checkBoxUseFullPath->isChecked();
    rc.address = lineEditAddress->text().toULongLong(0, 0);
    rc.functionName = lineEditFunction->text();
    rc.fileName = pathChooserFileName->path();
    rc.condition = lineEditCondition->text().toUtf8();
    rc.ignoreCount = lineEditIgnoreCount->text().toInt();
    rc.threadSpec = lineEditThreadSpec->text().toUtf8();
    return rc;
}

void BreakpointDialog::typeChanged(int)
{
    const BreakpointType t = type();
    const bool isLineVisible = t == BreakpointByFileAndLine;
    const bool isFunctionVisible = t == BreakpointByFunction || t == BreakpointAtMain;
    const bool isAddressVisible = t == BreakpointByAddress || t == Watchpoint;
    labelFileName->setEnabled(isLineVisible);
    pathChooserFileName->setEnabled(isLineVisible);
    labelLineNumber->setEnabled(isLineVisible);
    lineEditLineNumber->setEnabled(isLineVisible);
    labelUseFullPath->setEnabled(isLineVisible);
    checkBoxUseFullPath->setEnabled(isLineVisible);
    labelFunction->setEnabled(isFunctionVisible);
    lineEditFunction->setEnabled(isFunctionVisible);
    labelAddress->setEnabled(isAddressVisible);
    lineEditAddress->setEnabled(isAddressVisible);
    if (t == BreakpointAtMain)
        lineEditFunction->setText(QLatin1String("main"));
}

bool BreakpointDialog::showDialog(BreakpointData *data)
{
    setParameters(data->parameters());
    if (exec() != QDialog::Accepted)
        return false;

    // Check if changed.
    const BreakpointParameters newParameters = parameters();
    if (newParameters == data->parameters())
        return false;

    bool result = false;
    result |= data->setType(newParameters.type);
    result |= data->setAddress(newParameters.address);
    result |= data->setFunctionName(newParameters.functionName);
    result |= data->setUseFullPath(newParameters.useFullPath);
    result |= data->setFileName(newParameters.fileName);
    result |= data->setLineNumber(newParameters.lineNumber);
    result |= data->setCondition(newParameters.condition);
    result |= data->setIgnoreCount(newParameters.ignoreCount);
    result |= data->setThreadSpec(newParameters.threadSpec);
    return result;
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
    connect(debuggerCore()->action(UseAddressInBreakpointsView), SIGNAL(toggled(bool)),
        SLOT(showAddressColumn(bool)));
}

BreakWindow::~BreakWindow()
{
}

void BreakWindow::showAddressColumn(bool on)
{
    setColumnHidden(7, !on);
}

static QModelIndexList normalizeIndexes(const QModelIndexList &list)
{
    QModelIndexList res;
    foreach (const QModelIndex &index, list)
        if (index.column() == 0)
            res.append(index);
    return res;
}

void BreakWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Delete) {
        QItemSelectionModel *sm = selectionModel();
        QTC_ASSERT(sm, return);
        QModelIndexList si = sm->selectedIndexes();
        if (si.isEmpty())
            si.append(currentIndex().sibling(currentIndex().row(), 0));
        deleteBreakpoints(normalizeIndexes(si));
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
    if (indexUnderMouse.isValid() && indexUnderMouse.column() >= 4)
        editBreakpoints(QModelIndexList() << indexUnderMouse);
    QTreeView::mouseDoubleClickEvent(ev);
}

void BreakWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;
    QItemSelectionModel *sm = selectionModel();
    QTC_ASSERT(sm, return);
    QModelIndexList si = sm->selectedIndexes();
    QModelIndex indexUnderMouse = indexAt(ev->pos());
    if (si.isEmpty() && indexUnderMouse.isValid())
        si.append(indexUnderMouse.sibling(indexUnderMouse.row(), 0));
    si = normalizeIndexes(si);

    const int rowCount = model()->rowCount();
    const unsigned engineCapabilities = BreakOnThrowAndCatchCapability;
    BreakHandler *handler = breakHandler();
    // FIXME BP:    model()->data(QModelIndex(), EngineCapabilitiesRole).toUInt();

    QAction *deleteAction = new QAction(tr("Delete Breakpoint"), &menu);
    deleteAction->setEnabled(si.size() > 0);

    QAction *deleteAllAction = new QAction(tr("Delete All Breakpoints"), &menu);
    deleteAllAction->setEnabled(model()->rowCount() > 0);

    // Delete by file: Find indices of breakpoints of the same file.
    QAction *deleteByFileAction = 0;
    QList<QModelIndex> breakPointsOfFile;
    if (indexUnderMouse.isValid()) {
        const QModelIndex index = indexUnderMouse.sibling(indexUnderMouse.row(), 2);
        const QString file = model()->data(index).toString();
        if (!file.isEmpty()) {
            for (int i = 0; i < rowCount; i++)
                if (model()->data(model()->index(i, 2)).toString() == file)
                    breakPointsOfFile.push_back(model()->index(i, 2));
            if (breakPointsOfFile.size() > 1) {
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
    editBreakpointAction->setEnabled(si.size() > 0);

    int threadId = 0;
    // FIXME BP: m_engine->threadsHandler()->currentThreadId();
    QString associateTitle = threadId == -1
        ?  tr("Associate Breakpoint With All Threads")
        :  tr("Associate Breakpoint With Thread %1").arg(threadId);
    QAction *associateBreakpointAction = new QAction(associateTitle, &menu);
    associateBreakpointAction->setEnabled(si.size() > 0);

    QAction *synchronizeAction =
        new QAction(tr("Synchronize Breakpoints"), &menu);
    synchronizeAction->setEnabled(debuggerCore()->hasSnapshots());

    QModelIndex idx0 = (si.size() ? si.front() : QModelIndex());
    const BreakpointId id = handler->findBreakpointByIndex(idx0);

    bool enabled = si.isEmpty() || handler->isEnabled(id);

    const QString str5 = si.size() > 1
        ? enabled
            ? tr("Disable Selected Breakpoints")
            : tr("Enable Selected Breakpoints")
        : enabled
            ? tr("Disable Breakpoint")
            : tr("Enable Breakpoint");
    QAction *toggleEnabledAction = new QAction(str5, &menu);
    toggleEnabledAction->setEnabled(si.size() > 0);

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

    if (act == deleteAction) {
        deleteBreakpoints(si);
    } else if (act == deleteAllAction) {
        QList<QModelIndex> allRows;
        for (int i = 0; i < rowCount; i++)
            allRows.push_back(model()->index(i, 0));
        deleteBreakpoints(allRows);
    }  else if (act == deleteByFileAction)
        deleteBreakpoints(breakPointsOfFile);
    else if (act == adjustColumnAction)
        resizeColumnsToContents();
    else if (act == alwaysAdjustAction)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    else if (act == editBreakpointAction)
        editBreakpoints(si);
    else if (act == associateBreakpointAction)
        associateBreakpoint(si, threadId);
    else if (act == synchronizeAction)
        ; //synchronizeBreakpoints();
    else if (act == toggleEnabledAction)
        setBreakpointsEnabled(si, !enabled);
    else if (act == addBreakpointAction)
        addBreakpoint();
    else if (act == breakAtThrowAction) {
        handler->appendBreakpoint(BreakpointData(BreakpointAtThrow));
    } else if (act == breakAtCatchAction) {
        handler->appendBreakpoint(BreakpointData(BreakpointAtCatch));
    }
}

void BreakWindow::setBreakpointsEnabled(const QModelIndexList &list, bool enabled)
{
    BreakHandler *handler = breakHandler();
    foreach (const BreakpointId id, handler->findBreakpointsByIndex(list))
        handler->setEnabled(id, enabled);
}

void BreakWindow::setBreakpointsFullPath(const QModelIndexList &list, bool fullpath)
{
    BreakHandler *handler = breakHandler();
    foreach (const BreakpointId id, handler->findBreakpointsByIndex(list))
       handler->setUseFullPath(id, fullpath);
}

void BreakWindow::deleteBreakpoints(const QModelIndexList &list)
{
    BreakHandler *handler = breakHandler();
    foreach (const BreakpointId id, handler->findBreakpointsByIndex(list))
       handler->removeBreakpoint(id);
}

void BreakWindow::editBreakpoint(BreakpointId id, QWidget *parent)
{
    BreakpointDialog dialog(parent);
    dialog.showDialog(breakHandler()->breakpointById(id));
}

void BreakWindow::addBreakpoint()
{
    BreakpointData data(BreakpointByFileAndLine);
    BreakpointDialog dialog(this);
    if (dialog.showDialog(&data))
        breakHandler()->appendBreakpoint(data);
}

void BreakWindow::editBreakpoints(const QModelIndexList &list)
{
    QTC_ASSERT(!list.isEmpty(), return);

    BreakHandler *handler = breakHandler();
    const BreakpointIds ids = handler->findBreakpointsByIndex(list);
    const BreakpointId id = ids.at(0);

    if (list.size() == 1) {
        editBreakpoint(id, this);
        return;
    }

    // This allows to change properties of multiple breakpoints at a time.
    QDialog dlg(this);
    Ui::BreakCondition ui;
    ui.setupUi(&dlg);
    dlg.setWindowTitle(tr("Edit Breakpoint Properties"));
    ui.lineEditIgnoreCount->setValidator(
        new QIntValidator(0, 2147483647, ui.lineEditIgnoreCount));

    const QString oldCondition = QString::fromLatin1(handler->condition(id));
    const QString oldIgnoreCount = QString::number(handler->ignoreCount(id));
    const QString oldThreadSpec = QString::fromLatin1(handler->threadSpec(id));

    ui.lineEditCondition->setText(oldCondition);
    ui.lineEditIgnoreCount->setText(oldIgnoreCount);
    ui.lineEditThreadSpec->setText(oldThreadSpec);

    if (dlg.exec() == QDialog::Rejected)
        return;

    const QString newCondition = ui.lineEditCondition->text();
    const QString newIgnoreCount = ui.lineEditIgnoreCount->text();
    const QString newThreadSpec = ui.lineEditThreadSpec->text();

    // Unchanged -> cancel
    if (newCondition == oldCondition && newIgnoreCount == oldIgnoreCount
            && newThreadSpec == oldThreadSpec)
        return;

    foreach (const BreakpointId id, handler->findBreakpointsByIndex(list)) {
        handler->setCondition(id, newCondition.toLatin1());
        handler->setIgnoreCount(id, newIgnoreCount.toInt());
        handler->setThreadSpec(id, newThreadSpec.toLatin1());
    }
}

void BreakWindow::associateBreakpoint(const QModelIndexList &list, int threadId)
{
    BreakHandler *handler = breakHandler();
    QByteArray spec = QByteArray::number(threadId);
    foreach (const BreakpointId id, handler->findBreakpointsByIndex(list))
        handler->setThreadSpec(id, spec);
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
