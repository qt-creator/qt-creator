// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QPlainTextEdit>

namespace CppEditor {

class WrappableLineEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit WrappableLineEdit(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void insertFromMimeData(const QMimeData *source) override;
};

} // namespace CppEditor
