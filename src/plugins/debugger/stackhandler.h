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

#ifndef DEBUGGER_STACKHANDLER_H
#define DEBUGGER_STACKHANDLER_H

#include "stackframe.h"

#include <QAbstractItemModel>

namespace Debugger {
namespace Internal {

class DebuggerEngine;

enum StackColumns
{
    StackLevelColumn,
    StackFunctionNameColumn,
    StackFileNameColumn,
    StackLineNumberColumn,
    StackAddressColumn,
    StackColumnCount
};

class StackHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit StackHandler(DebuggerEngine *engine);
    ~StackHandler();

    void setFrames(const StackFrames &frames, bool canExpand = false);
    void setFramesAndCurrentIndex(const GdbMi &frames, bool isFull);
    int updateTargetFrame(bool isFull);
    void prependFrames(const StackFrames &frames);
    const StackFrames &frames() const;
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }
    int firstUsableIndex() const;
    StackFrame currentFrame() const;
    const StackFrame &frameAt(int index) const { return m_stackFrames.at(index); }
    int stackSize() const { return m_stackFrames.size(); }
    quint64 topAddress() const { return m_stackFrames.at(0).address; }

    // Called from StackHandler after a new stack list has been received
    void removeAll();
    QAbstractItemModel *model() { return this; }
    bool isContentsValid() const { return m_contentsValid; }
    void scheduleResetLocation();
    void resetLocation();

signals:
    void stackChanged();
    void currentIndexChanged();

private:
    // QAbstractTableModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    Q_SLOT void resetModel() { beginResetModel(); endResetModel(); }

    DebuggerEngine *m_engine;
    StackFrames m_stackFrames;
    int m_currentIndex;
    const QVariant m_positionIcon;
    const QVariant m_emptyIcon;
    bool m_canExpand;
    bool m_resetLocationScheduled;
    bool m_contentsValid;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_STACKHANDLER_H
