// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QJSEngine;
class QString;
QT_END_NAMESPACE

namespace Utils {

class MacroExpander;

class QTCREATOR_UTILS_EXPORT TemplateEngine {
public:
    static bool preprocessText(const QString &input, QString *output, QString *errorMessage);

    static QString processText(MacroExpander *expander, const QString &input,
                               QString *errorMessage);

    static bool evaluateBooleanJavaScriptExpression(QJSEngine &engine, const QString &expression,
                                                    bool *result, QString *errorMessage);
};

} // namespace Utils
