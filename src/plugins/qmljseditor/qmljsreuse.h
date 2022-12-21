// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QChar;
QT_END_NAMESPACE

namespace QmlJSEditor {
namespace Internal {

bool isIdentifierChar(const QChar &c, bool atStart = false, bool acceptDollar = true);
bool isValidFirstIdentifierChar(const QChar &c);
bool isValidIdentifierChar(const QChar &c);
bool isDelimiterChar(const QChar &c);
bool isActivationChar(const QChar &c);

QIcon iconForColor(const QColor &color);

} // Internal
} // QmlJSEditor
