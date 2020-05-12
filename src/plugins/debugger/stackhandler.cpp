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

#include "stackhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggericons.h"
#include "debuggerprotocol.h"
#include "memoryagent.h"
#include "simplifytype.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <utils/basetreeview.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QTextStream>

using namespace Utils;

namespace Debugger {
namespace Internal {

/*!
    \class Debugger::Internal::StackHandler
    \brief The StackHandler class provides a model to represent the stack in a
    QTreeView.
 */

StackHandler::StackHandler(DebuggerEngine *engine)
  : m_engine(engine)
{
    setObjectName("StackModel");
    setHeader({tr("Level"), tr("Function"), tr("File"), tr("Line"), tr("Address") });

    connect(action(ExpandStack), &QAction::triggered,
        this, &StackHandler::reloadFullStack);
    connect(action(MaximalStackDepth), &QAction::triggered,
        this, &StackHandler::reloadFullStack);

    // For now there's always only "the" current thread.
    rootItem()->appendChild(new ThreadDummyItem);
}

StackHandler::~StackHandler() = default;

QVariant SpecialStackItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole && column == StackLevelColumn)
        return StackHandler::tr("...");
    if (role == Qt::DisplayRole && column == StackFunctionNameColumn)
        return StackHandler::tr("<More>");
    if (role == Qt::DecorationRole && column == StackLevelColumn)
        return Icons::EMPTY.icon();
    return QVariant();
}

QVariant StackFrameItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (column) {
        case StackLevelColumn:
            return row >= 0 ? QString::number(row + 1) : QString();
        case StackFunctionNameColumn:
            return simplifyType(frame.function);
        case StackFileNameColumn:
            return frame.file.isEmpty() ? frame.module : FilePath::fromString(frame.file).fileName();
        case StackLineNumberColumn:
            return frame.line > 0 ? QVariant(frame.line) : QVariant();
        case StackAddressColumn:
            if (frame.address)
                return QString("0x%1").arg(frame.address, 0, 16);
            return QString();
        }
        return QVariant();
    }

    if (role == Qt::DecorationRole && column == StackLevelColumn)
        return handler->iconForRow(row);

    if (role == Qt::ToolTipRole && boolSetting(UseToolTipsInStackView))
        return frame.toToolTip();

    return QVariant();
}

Qt::ItemFlags StackFrameItem::flags(int column) const
{
    const bool isValid = frame.isUsable() || handler->operatesByInstruction();
    return isValid && handler->isContentsValid()
            ? TreeItem::flags(column) : Qt::ItemFlags();
}

QIcon StackHandler::iconForRow(int row) const
{
    // Return icon that indicates whether this is the active stack frame
    return (m_contentsValid && row == m_currentIndex)
            ? Icons::LOCATION.icon() : Icons::EMPTY.icon();
}

bool StackHandler::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    if (role == BaseTreeView::ItemActivatedRole || role == BaseTreeView::ItemClickedRole) {
        m_engine->activateFrame(idx.row());
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = data.value<ItemViewEvent>();
        if (ev.type() == QEvent::ContextMenu)
            return contextMenuEvent(ev);
    }

    return false;
}

ThreadDummyItem *StackHandler::dummyThreadItem() const
{
    QTC_ASSERT(rootItem()->childCount() == 1, return nullptr);
    return rootItem()->childAt(0);
}

StackFrame StackHandler::currentFrame() const
{
    if (m_currentIndex == -1)
        return {};
    QTC_ASSERT(m_currentIndex >= 0, return {});
    return frameAt(m_currentIndex);
}

StackFrame StackHandler::frameAt(int index) const
{
    auto threadItem = dummyThreadItem();
    QTC_ASSERT(threadItem, return {});
    StackFrameItem *frameItem = threadItem->childAt(index);
    QTC_ASSERT(frameItem, return {});
    return frameItem->frame;
}

int StackHandler::stackSize() const
{
    return stackRowCount() - m_canExpand;
}

quint64 StackHandler::topAddress() const
{
    QTC_ASSERT(stackRowCount() > 0, return 0);
    return frameAt(0).address;
}

void StackHandler::setCurrentIndex(int level)
{
    if (level == -1 || level == m_currentIndex)
        return;

    // Emit changed for previous frame
    QModelIndex i = index(m_currentIndex, 0);
    emit dataChanged(i, i);

    m_currentIndex = level;
    emit currentIndexChanged();

    // Emit changed for new frame
    i = index(m_currentIndex, 0);
    emit dataChanged(i, i);
}

void StackHandler::removeAll()
{
    auto threadItem = dummyThreadItem();
    QTC_ASSERT(threadItem, return);
    threadItem->removeChildren();

    setCurrentIndex(-1);
}

bool StackHandler::operatesByInstruction() const
{
    return m_engine->operatesByInstruction();
}

void StackHandler::setFrames(const StackFrames &frames, bool canExpand)
{
    auto threadItem = dummyThreadItem();
    QTC_ASSERT(threadItem, return);

    threadItem->removeChildren();

    m_contentsValid = true;
    m_canExpand = canExpand;

    int row = 0;
    for (const StackFrame &frame : frames)
        threadItem->appendChild(new StackFrameItem(this, frame, row++));

    if (canExpand)
        threadItem->appendChild(new SpecialStackItem(this));

    if (frames.isEmpty())
        m_currentIndex = -1;
    else
        setCurrentIndex(0);

    emit stackChanged();
}

void StackHandler::setFramesAndCurrentIndex(const GdbMi &frames, bool isFull)
{
    int targetFrame = -1;

    StackFrames stackFrames;
    const int n = frames.childCount();
    for (int i = 0; i != n; ++i) {
        stackFrames.append(StackFrame::parseFrame(frames.childAt(i), m_engine->runParameters()));
        const StackFrame &frame = stackFrames.back();

        // Initialize top frame to the first valid frame.
        const bool isValid = frame.isUsable() && !frame.function.isEmpty();
        if (isValid && targetFrame == -1)
            targetFrame = i;
    }

    bool canExpand = !isFull && (n >= action(MaximalStackDepth)->value().toInt());
    action(ExpandStack)->setEnabled(canExpand);
    setFrames(stackFrames, canExpand);

    // We can't jump to any file if we don't have any frames.
    if (stackFrames.isEmpty())
        return;

    // targetFrame contains the top most frame for which we have source
    // information. That's typically the frame we'd like to jump to, with
    // a few exceptions:

    // Always jump to frame #0 when stepping by instruction.
    if (m_engine->operatesByInstruction())
        targetFrame = 0;

    // If there is no frame with source, jump to frame #0.
    if (targetFrame == -1)
        targetFrame = 0;

    setCurrentIndex(targetFrame);
}

void StackHandler::prependFrames(const StackFrames &frames)
{
    if (frames.isEmpty())
        return;
    auto threadItem = dummyThreadItem();
    QTC_ASSERT(threadItem, return);
    const int count = frames.size();
    for (int i = count - 1; i >= 0; --i)
        threadItem->prependChild(new StackFrameItem(this, frames.at(i)));
    if (m_currentIndex >= 0)
        setCurrentIndex(m_currentIndex + count);
    emit stackChanged();
}

bool StackHandler::isSpecialFrame(int index) const
{
    return m_canExpand && index + 1 == stackRowCount();
}

int StackHandler::firstUsableIndex() const
{
    if (!m_engine->operatesByInstruction()) {
        for (int i = 0, n = stackRowCount(); i != n; ++i)
            if (frameAt(i).isUsable())
                return i;
    }
    return 0;
}

void StackHandler::scheduleResetLocation()
{
    m_contentsValid = false;
}

void StackHandler::resetLocation()
{
    emit layoutChanged();
}

int StackHandler::stackRowCount() const
{
    // Only one "thread" for now.
    auto threadItem = dummyThreadItem();
    QTC_ASSERT(threadItem, return 0);
    return threadItem->childCount();
}

// Input a function to be disassembled. Accept CDB syntax
// 'Module!function' for module specification

static StackFrame inputFunctionForDisassembly()
{
    StackFrame frame;
    QInputDialog dialog;
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setLabelText(StackHandler::tr("Function:"));
    dialog.setWindowTitle(StackHandler::tr("Disassemble Function"));
    if (dialog.exec() != QDialog::Accepted)
        return frame;
    const QString function = dialog.textValue();
    if (function.isEmpty())
        return frame;
    const int bangPos = function.indexOf('!');
    if (bangPos != -1) {
        frame.module = function.left(bangPos);
        frame.function = function.mid(bangPos + 1);
    } else {
        frame.function = function;
    }
    frame.line = 42; // trick gdb into mixed mode.
    return frame;
}

// Write stack frames as task file for displaying it in the build issues pane.
void StackHandler::saveTaskFile()
{
    QFile file;
    QFileDialog fileDialog(Core::ICore::dialogParent());
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.selectFile(QDir::currentPath() + "/stack.tasks");
    while (!file.isOpen()) {
        if (fileDialog.exec() != QDialog::Accepted)
            return;
        const QString fileName = fileDialog.selectedFiles().constFirst();
        file.setFileName(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString msg = tr("Cannot open \"%1\": %2")
                    .arg(QDir::toNativeSeparators(fileName), file.errorString());
            Core::AsynchronousMessageBox::warning(tr("Cannot Open Task File"), msg);
        }
    }

    QTextStream str(&file);
    forItemsAtLevel<2>([&str](StackFrameItem *item) {
        const StackFrame &frame = item->frame;
        if (frame.isUsable())
            str << frame.file << '\t' << frame.line << "\tstack\tFrame #" << frame.level << '\n';
    });
}

bool StackHandler::contextMenuEvent(const ItemViewEvent &ev)
{
    auto menu = new QMenu;

    const int row = ev.sourceModelIndex().row();
    StackFrame frame;
    if (row >= 0 && row < stackSize())
        frame = frameAt(row);
    const quint64 address = frame.address;

    menu->addAction(action(ExpandStack));

    addAction(menu, tr("Copy Contents to Clipboard"), true, [this] { copyContentsToClipboard(); });
    addAction(menu, tr("Save as Task File..."), true, [this] { saveTaskFile(); });

    if (m_engine->hasCapability(CreateFullBacktraceCapability))
        menu->addAction(action(CreateFullBacktrace));

    if (m_engine->hasCapability(AdditionalQmlStackCapability))
        addAction(menu, tr("Load QML Stack"), true, [this] { m_engine->loadAdditionalQmlStack(); });

    if (m_engine->hasCapability(ShowMemoryCapability))
        addAction(menu, tr("Open Memory Editor at 0x%1").arg(address, 0, 16),
                  tr("Open Memory Editor"),
                  address,
                  [this, row, frame, address] {
                        MemoryViewSetupData data;
                        data.startAddress = address;
                        data.title = tr("Memory at Frame #%1 (%2) 0x%3")
                                        .arg(row).arg(frame.function).arg(address, 0, 16);
                        data.markup.push_back(MemoryMarkup(address, 1, QColor(Qt::blue).lighter(),
                                                  tr("Frame #%1 (%2)").arg(row).arg(frame.function)));
                        m_engine->openMemoryView(data);
                   });

    if (m_engine->hasCapability(DisassemblerCapability)) {
        addAction(menu, tr("Open Disassembler at 0x%1").arg(address, 0, 16),
                  tr("Open Disassembler"),
                  address,
                  [this, frame] { m_engine->openDisassemblerView(frame); });

        addAction(menu, tr("Open Disassembler at Address..."), true,
                  [this, address] {
                        AddressDialog dialog;
                        if (address)
                            dialog.setAddress(address);
                        if (dialog.exec() == QDialog::Accepted)
                            m_engine->openDisassemblerView(Location(dialog.address()));
                   });

        addAction(menu, tr("Disassemble Function..."), true,
                  [this] {
                        const StackFrame frame = inputFunctionForDisassembly();
                        if (!frame.function.isEmpty())
                            m_engine->openDisassemblerView(Location(frame));
                  });
    }

    if (m_engine->hasCapability(ShowModuleSymbolsCapability)) {
        addAction(menu, tr("Try to Load Unknown Symbols"), true,
                  [this] { m_engine->loadSymbolsForStack(); });
    }

    menu->addSeparator();
    menu->addAction(action(UseToolTipsInStackView));
    Internal::addHideColumnActions(menu, ev.view());
    menu->addAction(action(SettingsDialog));
    menu->popup(ev.globalPos());
    return true;
}

void StackHandler::copyContentsToClipboard()
{
    const int m = columnCount(QModelIndex());
    QVector<int> largestColumnWidths(m, 0);

    // First, find the widths of the largest columns,
    // so that we can print them out nicely aligned.
    forItemsAtLevel<2>([m, &largestColumnWidths](StackFrameItem *item) {
        for (int j = 0; j < m; ++j) {
            const int columnWidth = item->data(j, Qt::DisplayRole).toString().size();
            if (columnWidth > largestColumnWidths.at(j))
                largestColumnWidths[j] = columnWidth;
        }
    });

    QString str;
    forItemsAtLevel<2>([m, largestColumnWidths, &str](StackFrameItem *item) {
        for (int j = 0; j != m; ++j) {
            const QString columnEntry = item->data(j, Qt::DisplayRole).toString();
            str += columnEntry;
            const int difference = largestColumnWidths.at(j) - columnEntry.size();
            // Add one extra space between columns.
            str += QString(qMax(difference, 0) + 1, QChar(' '));
        }
        str += '\n';
    });

    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard->supportsSelection())
        clipboard->setText(str, QClipboard::Selection);
    clipboard->setText(str, QClipboard::Clipboard);
}

void StackHandler::reloadFullStack()
{
    m_engine->reloadFullStack();
}

} // namespace Internal
} // namespace Debugger
