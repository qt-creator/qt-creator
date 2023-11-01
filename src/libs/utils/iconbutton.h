// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QAbstractButton>

namespace Utils {

class QTCREATOR_UTILS_EXPORT IconButton : public QAbstractButton
{
public:
    IconButton(QWidget *parent = nullptr);

    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    void leaveEvent(QEvent *e) override;

    QSize sizeHint() const override;

private:
    bool m_containsMouse{false};
};

} // namespace Utils
