// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils.h"

#include <QDebug>
#include <QFile>

QByteArray fileContents(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Warning: Could not open '%s'.", qPrintable(filePath));
        return QByteArray();
    }
    return file.readAll();
}
