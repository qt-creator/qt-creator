// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QTextEdit>

QT_BEGIN_NAMESPACE
class QCompleter;
class QEvent;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT CompletingTextEdit : public QTextEdit
{
    Q_OBJECT
    Q_PROPERTY(int completionLengthThreshold
               READ completionLengthThreshold WRITE setCompletionLengthThreshold)

public:
    CompletingTextEdit(QWidget *parent = nullptr);
    ~CompletingTextEdit() override;

    void setCompleter(QCompleter *c);
    QCompleter *completer() const;

    int completionLengthThreshold() const;
    void setCompletionLengthThreshold(int len);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    bool event(QEvent *e) override;

private:
    class CompletingTextEditPrivate *d;
};

} // namespace Utils
