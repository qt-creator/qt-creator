// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef COMMON_H
#define COMMON_H

#include <QString>

QString maybeQuote(const QString &str);

void logSilently(const QString &msg);
void logFlashing(const QString &msg);

#endif // COMMON_H
