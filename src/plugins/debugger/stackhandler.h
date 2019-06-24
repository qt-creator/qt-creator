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

#pragma once

#include "stackframe.h"

#include <utils/basetreeview.h>
#include <utils/treemodel.h>

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

class StackFrameItem : public Utils::TreeItem
{
public:
    StackFrameItem();
    explicit StackFrameItem(const StackFrame &frame) : frame(frame) {}

public:
    StackFrame frame;
};

using StackHandlerModel = Utils::TreeModel<Utils::TypedTreeItem<StackFrameItem>, StackFrameItem>;

class StackHandler : public StackHandlerModel
{
    Q_OBJECT

public:
    explicit StackHandler(DebuggerEngine *engine);
    ~StackHandler() override;

    void setFrames(const StackFrames &frames, bool canExpand = false);
    void setFramesAndCurrentIndex(const GdbMi &frames, bool isFull);
    int updateTargetFrame(bool isFull);
    void prependFrames(const StackFrames &frames);
    bool isSpecialFrame(int index) const;
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }
    int firstUsableIndex() const;
    StackFrame currentFrame() const;
    StackFrame frameAt(int index) const;
    int stackSize() const;
    quint64 topAddress() const;

    // Called from StackHandler after a new stack list has been received
    void removeAll();
    QAbstractItemModel *model() { return this; }
    bool isContentsValid() const { return m_contentsValid; }
    void scheduleResetLocation();
    void resetLocation();
    void resetModel() { beginResetModel(); endResetModel(); }

signals:
    void stackChanged();
    void currentIndexChanged();

private:
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &idx, const QVariant &data, int role) override;

    bool contextMenuEvent(const Utils::ItemViewEvent &event);
    void reloadFullStack();
    void copyContentsToClipboard();
    void saveTaskFile();

    DebuggerEngine *m_engine;
    int m_currentIndex = -1;
    bool m_canExpand = false;
    bool m_resetLocationScheduled = false;
    bool m_contentsValid = false;
};

} // namespace Internal
} // namespace Debugger
