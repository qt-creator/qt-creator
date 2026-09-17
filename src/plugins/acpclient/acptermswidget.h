// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace AcpClient::Internal {

class AcpTermsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AcpTermsWidget(QWidget *parent = nullptr);

signals:
    void accepted();
    void declined();
};

} // namespace AcpClient::Internal
