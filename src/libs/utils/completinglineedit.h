// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QLineEdit>

namespace Utils {

class QTCREATOR_UTILS_EXPORT CompletingLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit CompletingLineEdit(QWidget *parent = nullptr);

protected:
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
};

} // namespace Utils
