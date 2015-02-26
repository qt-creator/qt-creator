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

#include "stackwindow.h"
#include "stackhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerdialogs.h"
#include "memoryagent.h"

#include <coreplugin/messagebox.h>

#include <utils/savedaction.h>

#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QDir>

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QFileDialog>
#include <QMenu>

namespace Debugger {
namespace Internal {

StackTreeView::StackTreeView()
{
    setWindowTitle(tr("Stack"));

    connect(action(UseAddressInStackView), &QAction::toggled,
        this, &StackTreeView::showAddressColumn);
    connect(action(ExpandStack), &QAction::triggered,
        this, &StackTreeView::reloadFullStack);
    connect(action(MaximalStackDepth), &QAction::triggered,
        this, &StackTreeView::reloadFullStack);
    showAddressColumn(false);
}

void StackTreeView::showAddressColumn(bool on)
{
    setColumnHidden(StackAddressColumn, !on);
    resizeColumnToContents(StackLevelColumn);
    resizeColumnToContents(StackLineNumberColumn);
    resizeColumnToContents(StackAddressColumn);
}

void StackTreeView::rowActivated(const QModelIndex &index)
{
    currentEngine()->activateFrame(index.row());
}

void StackTreeView::setModel(QAbstractItemModel *model)
{
    BaseTreeView::setModel(model);
    resizeColumnToContents(StackLevelColumn);
    resizeColumnToContents(StackLineNumberColumn);
    showAddressColumn(action(UseAddressInStackView)->isChecked());
}

// Input a function to be disassembled. Accept CDB syntax
// 'Module!function' for module specification

static inline StackFrame inputFunctionForDisassembly()
{
    StackFrame frame;
    QInputDialog dialog;
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setLabelText(StackTreeView::tr("Function:"));
    dialog.setWindowTitle(StackTreeView::tr("Disassemble Function"));
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    if (dialog.exec() != QDialog::Accepted)
        return frame;
    const QString function = dialog.textValue();
    if (function.isEmpty())
        return frame;
    const int bangPos = function.indexOf(QLatin1Char('!'));
    if (bangPos != -1) {
        frame.from = function.left(bangPos);
        frame.function = function.mid(bangPos + 1);
    } else {
        frame.function = function;
    }
    frame.line = 42; // trick gdb into mixed mode.
    return frame;
}

// Write stack frames as task file for displaying it in the build issues pane.
void saveTaskFile(QWidget *parent, const StackHandler *sh)
{
    QFile file;
    QFileDialog fileDialog(parent);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.selectFile(QDir::currentPath() + QLatin1String("/stack.tasks"));
    while (!file.isOpen()) {
        if (fileDialog.exec() != QDialog::Accepted)
            return;
        const QString fileName = fileDialog.selectedFiles().front();
        file.setFileName(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Core::AsynchronousMessageBox::warning(StackTreeView::tr("Cannot Open Task File"),
                                 StackTreeView::tr("Cannot open \"%1\": %2").arg(QDir::toNativeSeparators(fileName), file.errorString()));
        }
    }

    QTextStream str(&file);
    foreach (const StackFrame &frame, sh->frames()) {
        if (frame.isUsable())
            str << frame.file << '\t' << frame.line << "\tstack\tFrame #" << frame.level << '\n';
    }
}

void StackTreeView::contextMenuEvent(QContextMenuEvent *ev)
{
    DebuggerEngine *engine = currentEngine();
    StackHandler *handler = engine->stackHandler();
    const QModelIndex index = indexAt(ev->pos());
    const int row = index.row();
    StackFrame frame;
    if (row >= 0 && row < handler->stackSize())
        frame = handler->frameAt(row);
    const quint64 address = frame.address;

    QMenu menu;
    menu.addAction(action(ExpandStack));

    QAction *actCopyContents = menu.addAction(tr("Copy Contents to Clipboard"));
    actCopyContents->setEnabled(model() != 0);

    QAction *actSaveTaskFile = menu.addAction(tr("Save as Task File..."));
    actSaveTaskFile->setEnabled(model() != 0);

    if (engine->hasCapability(CreateFullBacktraceCapability))
        menu.addAction(action(CreateFullBacktrace));

    QAction *additionalQmlStackAction = 0;
    if (engine->hasCapability(AdditionalQmlStackCapability))
        additionalQmlStackAction = menu.addAction(tr("Load QML Stack"));

    QAction *actShowMemory = 0;
    if (engine->hasCapability(ShowMemoryCapability)) {
        actShowMemory = menu.addAction(QString());
        if (address == 0) {
            actShowMemory->setText(tr("Open Memory Editor"));
            actShowMemory->setEnabled(false);
        } else {
            actShowMemory->setText(tr("Open Memory Editor at 0x%1").arg(address, 0, 16));
            actShowMemory->setEnabled(engine->hasCapability(ShowMemoryCapability));
        }
    }

    QAction *actShowDisassemblerAt = 0;
    QAction *actShowDisassemblerAtAddress = 0;
    QAction *actShowDisassemblerAtFunction = 0;

    if (engine->hasCapability(DisassemblerCapability)) {
        actShowDisassemblerAt = menu.addAction(QString());
        actShowDisassemblerAtAddress = menu.addAction(tr("Open Disassembler at Address..."));
        actShowDisassemblerAtFunction = menu.addAction(tr("Disassemble Function..."));
        if (address == 0) {
            actShowDisassemblerAt->setText(tr("Open Disassembler"));
            actShowDisassemblerAt->setEnabled(false);
        } else {
            actShowDisassemblerAt->setText(tr("Open Disassembler at 0x%1").arg(address, 0, 16));
        }
    }


    QAction *actLoadSymbols = 0;
    if (engine->hasCapability(ShowModuleSymbolsCapability))
        actLoadSymbols = menu.addAction(tr("Try to Load Unknown Symbols"));

    if (engine->hasCapability(MemoryAddressCapability))
        menu.addAction(action(UseAddressInStackView));

    menu.addSeparator();
    menu.addAction(action(UseToolTipsInStackView));
    menu.addSeparator();
    menu.addAction(action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());
    if (!act)
        return;

    if (act == actCopyContents) {
        copyContentsToClipboard();
    } else if (act == actShowMemory) {
        MemoryViewSetupData data;
        data.startAddress = address;
        data.title = tr("Memory at Frame #%1 (%2) 0x%3").
            arg(row).arg(frame.function).arg(address, 0, 16);
        data.markup.push_back(MemoryMarkup(address, 1, QColor(Qt::blue).lighter(),
                                  tr("Frame #%1 (%2)").arg(row).arg(frame.function)));
        engine->openMemoryView(data);
    } else if (act == actShowDisassemblerAtAddress) {
        AddressDialog dialog;
        if (address)
            dialog.setAddress(address);
        if (dialog.exec() == QDialog::Accepted)
            currentEngine()->openDisassemblerView(Location(dialog.address()));
    } else if (act == actShowDisassemblerAtFunction) {
        const StackFrame frame = inputFunctionForDisassembly();
        if (!frame.function.isEmpty())
            currentEngine()->openDisassemblerView(Location(frame));
    } else if (act == actShowDisassemblerAt)
        engine->openDisassemblerView(frame);
    else if (act == actLoadSymbols)
        engine->loadSymbolsForStack();
    else if (act == actSaveTaskFile)
        saveTaskFile(this, handler);
    else if (act == additionalQmlStackAction)
        engine->loadAdditionalQmlStack();
}

void StackTreeView::copyContentsToClipboard()
{
    QString str;
    int n = model()->rowCount();
    int m = model()->columnCount();
    for (int i = 0; i != n; ++i) {
        for (int j = 0; j != m; ++j) {
            QModelIndex index = model()->index(i, j);
            str += model()->data(index).toString();
            str += QLatin1Char('\t');
        }
        str += QLatin1Char('\n');
    }
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

void StackTreeView::reloadFullStack()
{
    currentEngine()->reloadFullStack();
}

} // namespace Internal
} // namespace Debugger
