// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include <QString>

namespace Debugger::Internal {

bool isSkippableFunction(const QStringView funcName, const QStringView fileName);
bool isLeavableFunction(const QStringView funcName, const QStringView fileName);

bool hasLetterOrNumber(const QStringView exp);
bool hasSideEffects(const QStringView exp);
bool isKeyWord(const QStringView exp);
bool isPointerType(const QStringView type);
bool isFloatType(const QStringView type);
bool isIntOrFloatType(const QStringView type);
bool isIntType(const QStringView type);

QString formatToolTipAddress(quint64 a);
QString removeObviousSideEffects(const QString &exp);

QString escapeUnprintable(const QString &str, int unprintableBase = -1);

} // Debugger::Internal
