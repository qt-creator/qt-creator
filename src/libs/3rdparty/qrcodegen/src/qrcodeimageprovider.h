// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qrcodegenerator_exports.h"

#include <QPixmap>
#include <QQuickImageProvider>

class QRCODEGENERATOR_EXPORT QrCodeImageProvider : public QQuickImageProvider
{
    Q_OBJECT

public:
    QrCodeImageProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
