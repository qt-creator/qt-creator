// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/Token.h>

#include <QList>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace CPlusPlus {
class Snapshot;

QList<Token> CPLUSPLUS_EXPORT commentsForDeclaration(const Symbol *symbol,
                                                     const Snapshot &snapshot,
                                                     const QTextDocument &textDoc);

} // namespace CPlusPlus
