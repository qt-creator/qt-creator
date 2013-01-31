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

#include "stackhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"

#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QDebug>
#include <QFileInfo>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// StackHandler
//
////////////////////////////////////////////////////////////////////////

/*!
    \class Debugger::Internal::StackHandler
    \brief A model to represent the stack in a QTreeView.
 */

StackHandler::StackHandler()
  : m_positionIcon(QIcon(QLatin1String(":/debugger/images/location_16.png"))),
    m_emptyIcon(QIcon(QLatin1String(":/debugger/images/debugger_empty_14.png")))
{
    m_resetLocationScheduled = false;
    m_contentsValid = false;
    m_currentIndex = -1;
    m_canExpand = false;
    connect(debuggerCore()->action(OperateByInstruction), SIGNAL(triggered()),
        this, SLOT(resetModel()));
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
    return parent.isValid() ? 0 : 5;
}

QVariant StackHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_stackFrames.size() + m_canExpand)
        return QVariant();

    if (index.row() == m_stackFrames.size()) {
        if (role == Qt::DisplayRole && index.column() == 0)
            return tr("...");
        if (role == Qt::DisplayRole && index.column() == 1)
            return tr("<More>");
        if (role == Qt::DecorationRole && index.column() == 0)
            return m_emptyIcon;
        return QVariant();
    }

    const StackFrame &frame = m_stackFrames.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: // Stack frame level
            return QString::number(frame.level);
        case 1: // Function name
            return frame.function;
        case 2: // File name
            return frame.file.isEmpty() ? frame.from : QFileInfo(frame.file).fileName();
        case 3: // Line number
            return frame.line > 0 ? QVariant(frame.line) : QVariant();
        case 4: // Address
            if (frame.address)
                return QString::fromLatin1("0x%1").arg(frame.address, 0, 16);
            return QString();
        }
        return QVariant();
    }

    if (role == Qt::DecorationRole && index.column() == 0) {
        // Return icon that indicates whether this is the active stack frame
        return (m_contentsValid && index.row() == m_currentIndex)
            ? m_positionIcon : m_emptyIcon;
    }

    if (role == Qt::ToolTipRole)
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
    const bool isValid = frame.isUsable()
        || debuggerCore()->boolSetting(OperateByInstruction);
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
