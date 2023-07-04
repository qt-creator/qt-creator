// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace TextEditor {

class AssistInterface;

class QuickFixOperation
{
public:
    QuickFixOperation(int priority = -1)
        : _priority(priority)
    {}

    virtual ~QuickFixOperation() {}

    virtual int priority() const { return _priority; }
    void setPriority(int priority) { _priority = priority; }

    virtual QString description() const { return _description; }
    void setDescription(const QString &description) { _description = description; }

    virtual void perform() = 0;

private:
    int _priority = -1;
    QString _description;
};

}
