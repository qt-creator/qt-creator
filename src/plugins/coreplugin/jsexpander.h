/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "core_global.h"

#include <functional>

QT_BEGIN_NAMESPACE
class QJSEngine;
class QObject;
class QString;
QT_END_NAMESPACE

namespace Utils {
class MacroExpander;
}

namespace Core {

namespace Internal {
class MainWindow;
class JsExpanderPrivate;
} // namespace Internal

class CORE_EXPORT JsExpander
{
public:
    using ObjectFactory = std::function<QObject *()>;

    JsExpander();
    ~JsExpander();

    template <class T>
    static void registerGlobalObject(const QString &name)
    {
        registerGlobalObject(name, []{ return new T; });
    }

    static void registerGlobalObject(const QString &name, const ObjectFactory &factory);

    void registerObject(const QString &name, QObject *obj);
    QString evaluate(const QString &expression, QString *errorMessage = nullptr);

    QJSEngine &engine();
    void registerForExpander(Utils::MacroExpander *macroExpander);

private:
    static JsExpander *createGlobalJsExpander();

    Internal::JsExpanderPrivate *d;
    friend class Core::Internal::MainWindow;
};

} // namespace Core
