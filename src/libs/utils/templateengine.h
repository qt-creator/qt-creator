// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "result.h"

QT_BEGIN_NAMESPACE
class QJSEngine;
QT_END_NAMESPACE

namespace Utils { class MacroExpander; }

namespace Utils::TemplateEngine {

QTCREATOR_UTILS_EXPORT Result<QString> preprocessText(const QString &input);

QTCREATOR_UTILS_EXPORT Result<QString> processText(MacroExpander *expander, const QString &input);

QTCREATOR_UTILS_EXPORT Result<bool> evaluateBooleanJavaScriptExpression(QJSEngine &engine,
                                                                        const QString &expression);

} // namespace Utils::TemplateEngine
