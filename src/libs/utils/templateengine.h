// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "result.h"

#include <memory>

namespace Utils { class MacroExpander; }

namespace Utils::TemplateEngine {

// Opaque, movable holder for the embedded JavaScript engine's state. Reuse one
// instance across several evaluateBooleanJavaScriptExpression() calls.
class QTCREATOR_UTILS_EXPORT JsEngine
{
public:
    JsEngine();
    ~JsEngine();
    JsEngine(JsEngine &&) noexcept;
    JsEngine &operator=(JsEngine &&) noexcept;

    class Private;
    Private *d() const { return m_d.get(); }

private:
    std::unique_ptr<Private> m_d;
};

QTCREATOR_UTILS_EXPORT Result<QString> preprocessText(const QString &input);

QTCREATOR_UTILS_EXPORT Result<QString> processText(MacroExpander *expander, const QString &input);

QTCREATOR_UTILS_EXPORT Result<bool> evaluateBooleanJavaScriptExpression(JsEngine &engine,
                                                                        const QString &expression);

} // namespace Utils::TemplateEngine
