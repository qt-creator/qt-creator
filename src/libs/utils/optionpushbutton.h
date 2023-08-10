// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QPushButton>

QT_BEGIN_NAMESPACE
class QMenu;
class QMouseEvent;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT OptionPushButton : public QPushButton
{
public:
    OptionPushButton(QWidget *parent = nullptr);
    OptionPushButton(const QString &text, QWidget *parent = nullptr);

    void setOptionalMenu(QMenu *menu);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

} // namespace Utils
