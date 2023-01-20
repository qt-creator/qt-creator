// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace Copilot {
class CopilotOptionsPageWidget : public QWidget
{
    Q_OBJECT
public:
    CopilotOptionsPageWidget(QWidget *parent = nullptr);
    ~CopilotOptionsPageWidget() override;
};
} // namespace Copilot
