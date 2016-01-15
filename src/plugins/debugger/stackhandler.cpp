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
#include "debuggericons.h"
#include "debuggerengine.h"
#include "debuggerprotocol.h"
#include "simplifytype.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QDebug>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// StackHandler
//
////////////////////////////////////////////////////////////////////////

/*!
    \class Debugger::Internal::StackHandler
    \brief The StackHandler class provides a model to represent the stack in a
    QTreeView.
 */

StackHandler::StackHandler(DebuggerEngine *engine)
  : m_engine(engine),
    m_positionIcon(Icons::LOCATION.icon()),
    m_emptyIcon(Icons::EMPTY.icon())
{
    setObjectName(QLatin1String("StackModel"));
    m_resetLocationScheduled = false;
    m_contentsValid = false;
    m_currentIndex = -1;
    m_canExpand = false;
    connect(action(OperateByInstruction), &QAction::triggered,
        this, &StackHandler::resetModel);
}

StackHandler::~StackHandler()
{
}

int StackHandler::rowCount(const QModelIndex &parent) const
{
    // Since the stack is not a tree, row count is 0 for any valid parent
    return parent.isValid() ? 0 : (m_stackFrames.size() + m_canExpand);
}

int StackHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : StackColumnCount;
}

QVariant StackHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_stackFrames.size() + m_canExpand)
        return QVariant();

    if (index.row() == m_stackFrames.size()) {
        if (role == Qt::DisplayRole && index.column() == StackLevelColumn)
            return tr("...");
        if (role == Qt::DisplayRole && index.column() == StackFunctionNameColumn)
            return tr("<More>");
        if (role == Qt::DecorationRole && index.column() == StackLevelColumn)
            return m_emptyIcon;
        return QVariant();
    }

    const StackFrame &frame = m_stackFrames.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case StackLevelColumn:
            return QString::number(index.row() + 1);
        case StackFunctionNameColumn:
            return simplifyType(frame.function);
        case StackFileNameColumn:
            return frame.file.isEmpty() ? frame.module : Utils::FileName::fromString(frame.file).fileName();
        case StackLineNumberColumn:
            return frame.line > 0 ? QVariant(frame.line) : QVariant();
        case StackAddressColumn:
            if (frame.address)
                return QString::fromLatin1("0x%1").arg(frame.address, 0, 16);
            return QString();
        }
        return QVariant();
    }

    if (role == Qt::DecorationRole && index.column() == StackLevelColumn) {
        // Return icon that indicates whether this is the active stack frame
        return (m_contentsValid && index.row() == m_currentIndex)
            ? m_positionIcon : m_emptyIcon;
    }

    if (role == Qt::ToolTipRole && boolSetting(UseToolTipsInStackView))
        return frame.toToolTip();

    return QVariant();
}


QVariant StackHandler::headerData(int section, Qt::Orientation orient, int role) const
{
    if (orient == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Level");
            case 1: return tr("Function");
            case 2: return tr("File");
            case 3: return tr("Line");
            case 4: return tr("Address");
        };
    }
    return QVariant();
}

Qt::ItemFlags StackHandler::flags(const QModelIndex &index) const
{
    if (index.row() >= m_stackFrames.size() + m_canExpand)
        return 0;
    if (index.row() == m_stackFrames.size())
        return QAbstractTableModel::flags(index);
    const StackFrame &frame = m_stackFrames.at(index.row());
    const bool isValid = frame.isUsable() || boolSetting(OperateByInstruction);
    return isValid && m_contentsValid
        ? QAbstractTableModel::flags(index) : Qt::ItemFlags();
}

StackFrame StackHandler::currentFrame() const
{
    if (m_currentIndex == -1)
        return StackFrame();
    QTC_ASSERT(m_currentIndex >= 0, return StackFrame());
    QTC_ASSERT(m_currentIndex < m_stackFrames.size(), return StackFrame());
    return m_stackFrames.at(m_currentIndex);
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
    beginResetModel();
    m_stackFrames.clear();
    setCurrentIndex(-1);
    endResetModel();
}

void StackHandler::setFrames(const StackFrames &frames, bool canExpand)
{
    beginResetModel();
    m_resetLocationScheduled = false;
    m_contentsValid = true;
    m_canExpand = canExpand;
    m_stackFrames = frames;
    if (m_stackFrames.size() >= 0)
        setCurrentIndex(0);
    else
        m_currentIndex = -1;
    endResetModel();
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
    if (boolSetting(OperateByInstruction))
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
    const int count = frames.size();
    beginInsertRows(QModelIndex(), 0, count - 1);
    for (int i = count - 1; i >= 0; --i)
        m_stackFrames.prepend(frames.at(i));
    endInsertRows();
    if (m_currentIndex >= 0)
        setCurrentIndex(m_currentIndex + count);
    emit stackChanged();
}

int StackHandler::firstUsableIndex() const
{
    if (!boolSetting(OperateByInstruction)) {
        for (int i = 0, n = m_stackFrames.size(); i != n; ++i)
            if (m_stackFrames.at(i).isUsable())
                return i;
    }
    return 0;
}

const StackFrames &StackHandler::frames() const
{
    return m_stackFrames;
}

void StackHandler::scheduleResetLocation()
{
    m_resetLocationScheduled = true;
    m_contentsValid = false;
}

void StackHandler::resetLocation()
{
    if (m_resetLocationScheduled) {
        beginResetModel();
        m_resetLocationScheduled = false;
        endResetModel();
    }
}

} // namespace Internal
} // namespace Debugger
