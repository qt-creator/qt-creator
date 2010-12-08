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

#include "stackwindow.h"
#include "stackhandler.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QtCore/QDebug>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>
#include <QtGui/QResizeEvent>

namespace Debugger {
namespace Internal {

static DebuggerEngine *currentEngine()
{
    return debuggerCore()->currentEngine();
}

StackWindow::StackWindow(QWidget *parent)
    : QTreeView(parent), m_alwaysResizeColumnsToContents(false)
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);

    QAction *act = debuggerCore()->action(UseAlternatingRowColors);
    setWindowTitle(tr("Stack"));

    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setIconSize(QSize(10, 10));

    header()->setDefaultAlignment(Qt::AlignLeft);
    header()->resizeSection(0, 60);
    header()->resizeSection(3, 60);

    connect(this, SIGNAL(activated(QModelIndex)),
        SLOT(rowActivated(QModelIndex)));
    connect(act, SIGNAL(toggled(bool)),
        SLOT(setAlternatingRowColorsHelper(bool)));
    connect(debuggerCore()->action(UseAddressInStackView), SIGNAL(toggled(bool)),
        SLOT(showAddressColumn(bool)));
    connect(debuggerCore()->action(ExpandStack), SIGNAL(triggered()),
        SLOT(reloadFullStack()));
    connect(debuggerCore()->action(MaximalStackDepth), SIGNAL(triggered()),
        SLOT(reloadFullStack()));
}

void StackWindow::showAddressColumn(bool on)
{
    setColumnHidden(4, !on);
}

void StackWindow::rowActivated(const QModelIndex &index)
{
    currentEngine()->activateFrame(index.row());
}

void StackWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    DebuggerEngine *engine = currentEngine();
    StackHandler *handler = engine->stackHandler();
    const QModelIndex index = indexAt(ev->pos());
    const int row = index.row();
    const unsigned engineCapabilities = engine->debuggerCapabilities();
    StackFrame frame;
    if (row >= 0 && row < handler->stackSize())
        frame = handler->frameAt(row);
    const quint64 address = frame.address;

    QMenu menu;
    menu.addAction(debuggerCore()->action(ExpandStack));

    QAction *actCopyContents = menu.addAction(tr("Copy Contents to Clipboard"));
    actCopyContents->setEnabled(model() != 0);

    if (engineCapabilities & CreateFullBacktraceCapability)
        menu.addAction(debuggerCore()->action(CreateFullBacktrace));

    QAction *actShowMemory = menu.addAction(QString());
    if (address == 0) {
        actShowMemory->setText(tr("Open Memory Editor"));
        actShowMemory->setEnabled(false);
    } else {
        actShowMemory->setText(tr("Open Memory Editor at 0x%1").arg(address, 0, 16));
        actShowMemory->setEnabled(engineCapabilities & ShowMemoryCapability);
    }

    QAction *actShowDisassembler = menu.addAction(QString());
    if (address == 0) {
        actShowDisassembler->setText(tr("Open Disassembler"));
        actShowDisassembler->setEnabled(false);
    } else {
        actShowDisassembler->setText(tr("Open Disassembler at 0x%1").arg(address, 0, 16));
        actShowDisassembler->setEnabled(engineCapabilities & DisassemblerCapability);
    }

    menu.addSeparator();
#if 0 // @TODO: not implemented
    menu.addAction(debuggerCore()->action(UseToolTipsInStackView));
#endif
    menu.addAction(debuggerCore()->action(UseAddressInStackView));

    QAction *actAdjust = menu.addAction(tr("Adjust Column Widths to Contents"));

    QAction *actAlwaysAdjust =
        menu.addAction(tr("Always Adjust Column Widths to Contents"));
    actAlwaysAdjust->setCheckable(true);
    actAlwaysAdjust->setChecked(m_alwaysResizeColumnsToContents);

    menu.addSeparator();

    menu.addAction(debuggerCore()->action(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());

    if (act == actCopyContents)
        copyContentsToClipboard();
    else if (act == actAdjust)
        resizeColumnsToContents();
    else if (act == actAlwaysAdjust)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    else if (act == actShowMemory)
        engine->openMemoryView(address);
    else if (act == actShowDisassembler)
        engine->openDisassemblerView(frame);
}

void StackWindow::copyContentsToClipboard()
{
    QString str;
    int n = model()->rowCount();
    int m = model()->columnCount();
    for (int i = 0; i != n; ++i) {
        for (int j = 0; j != m; ++j) {
            QModelIndex index = model()->index(i, j);
            str += model()->data(index).toString();
            str += '\t';
        }
        str += '\n';
    }
    QClipboard *clipboard = QApplication::clipboard();
#    ifdef Q_WS_X11
    clipboard->setText(str, QClipboard::Selection);
#    endif
    clipboard->setText(str, QClipboard::Clipboard);
}

void StackWindow::reloadFullStack()
{
    currentEngine()->reloadFullStack();
}

void StackWindow::resizeColumnsToContents()
{
    for (int i = model()->columnCount(); --i >= 0; )
        resizeColumnToContents(i);
}

void StackWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode =
        on ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    for (int i = model()->columnCount(); --i >= 0; )
        header()->setResizeMode(i, mode);
}

} // namespace Internal
} // namespace Debugger
