// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QStack>

namespace Valgrind::Callgrind {

class Function;

class StackBrowser : public QObject
{
    Q_OBJECT

public:
    explicit StackBrowser(QObject *parent = nullptr);

    void select(const Function *item);
    const Function *current() const;
    void clear();
    bool hasPrevious() const { return !m_stack.isEmpty(); }
    bool hasNext() const { return !m_redoStack.isEmpty(); }

    void goBack();
    void goNext();

signals:
    void currentChanged();

private:
    QStack<const Function *> m_stack;
    QStack<const Function *> m_redoStack;
};

} // namespace Valgrind::Callgrind
