// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/Token.h>

#include <QList>

QT_BEGIN_NAMESPACE
class QString;
class QTextCursor;
QT_END_NAMESPACE

namespace CPlusPlus {

class BackwardsScanner;

class CPLUSPLUS_EXPORT ExpressionUnderCursor
{
public:
    ExpressionUnderCursor(const LanguageFeatures &languageFeatures);

    QString operator()(const QTextCursor &cursor);
    int startOfFunctionCall(const QTextCursor &cursor) const;

private:
    int startOfExpression(BackwardsScanner &tk, int index);
    int startOfExpression_helper(BackwardsScanner &tk, int index);

private:
    bool _jumpedComma;
    LanguageFeatures _languageFeatures;
};

} // namespace CPlusPlus
