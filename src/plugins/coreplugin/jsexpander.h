// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <functional>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Utils { class MacroExpander; }

namespace Core {

namespace Internal {
class ICorePrivate;
class JsExpanderPrivate;
} // namespace Internal

namespace JsBind { class Registrar; }

class CORE_EXPORT JsExpander
{
public:
    // Populates a named JavaScript object with bound C++ callables (see jsbind.h).
    using FunctionFactory = std::function<void(JsBind::Registrar &)>;

    JsExpander();
    ~JsExpander();

    static void registerGlobalFunctions(const QString &name, const FunctionFactory &factory);

    void registerFunctions(const QString &name, const FunctionFactory &factory);
    QString evaluate(const QString &expression, QString *errorMessage = nullptr);

    void registerForExpander(Utils::MacroExpander *macroExpander);

private:
    static JsExpander *createGlobalJsExpander();

    Internal::JsExpanderPrivate *d;
    friend class Internal::ICorePrivate;
};

#ifdef WITH_TESTS
namespace Internal { QObject *createJsExpanderTest(); }
#endif

} // namespace Core
