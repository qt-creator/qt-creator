// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include <QString>

namespace Debugger {
namespace Internal {

bool isSkippableFunction(const QStringView funcName, const QStringView fileName);
bool isLeavableFunction(const QStringView funcName, const QStringView fileName);

bool hasLetterOrNumber(const QString &exp);
bool hasSideEffects(const QString &exp);
bool isKeyWord(const QString &exp);
bool isPointerType(const QString &type);
bool isFloatType(const QString &type);
bool isIntOrFloatType(const QString &type);
bool isIntType(const QString &type);

QString formatToolTipAddress(quint64 a);
QString removeObviousSideEffects(const QString &exp);

QString escapeUnprintable(const QString &str, int unprintableBase = -1);

} // namespace Internal
} // namespace Debugger
