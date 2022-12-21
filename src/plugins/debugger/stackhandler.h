// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "stackframe.h"

#include <utils/basetreeview.h>
#include <utils/treemodel.h>

namespace Debugger::Internal {

class DebuggerEngine;
class StackHandler;

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
    StackFrameItem(StackHandler *handler, const StackFrame &frame, int row = -1)
        : handler(handler), frame(frame), row(row)
    {}

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;

public:
    StackHandler *handler = nullptr;
    StackFrame frame;
    int row = -1;
};


class SpecialStackItem : public StackFrameItem
{
public:
    SpecialStackItem(StackHandler *handler)
        : StackFrameItem(handler, StackFrame{})
    {}

    QVariant data(int column, int role) const override;
};

// FIXME: Move ThreadItem over here.
class ThreadDummyItem : public Utils::TypedTreeItem<StackFrameItem>
{
public:
};

using StackHandlerModel = Utils::TreeModel<
    Utils::TypedTreeItem<ThreadDummyItem>,
    Utils::TypedTreeItem<StackFrameItem>,
    StackFrameItem
>;

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
    bool operatesByInstruction() const;
    void scheduleResetLocation();
    void resetLocation();

    QIcon iconForRow(int row) const;

signals:
    void stackChanged();
    void currentIndexChanged();

private:
    int stackRowCount() const; // Including the <more...> "frame"

    bool setData(const QModelIndex &idx, const QVariant &data, int role) override;
    ThreadDummyItem *dummyThreadItem() const;

    bool contextMenuEvent(const Utils::ItemViewEvent &event);
    void reloadFullStack();
    void saveTaskFile();

    DebuggerEngine *m_engine;
    int m_currentIndex = -1;
    bool m_canExpand = false;
    bool m_contentsValid = false;
};

} // Debugger::Internal
