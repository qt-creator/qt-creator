// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <QString>
#include <QStringList>

namespace Core::JsBind { class Registrar; }

namespace QtSupport {

class QTSUPPORT_EXPORT CodeGenerator
{
public:
    // Ui file related:
    // Change the class name in a UI XML form
    static QString changeUiClassName(const QString &uiXml, const QString &newUiClassName);

    // Low level method to get everything at the same time:
    static bool uiData(const QString &uiXml, QString *formBaseClass, QString *uiClassName);

    static QString uiClassName(const QString &uiXml);

    // Generic Qt:
    static QString qtIncludes(const QStringList &qt4, const QStringList &qt5);

    // UI file integration
    static bool uiAsPointer();
    static bool uiAsMember();
    static bool uiAsInheritance();
};

// Registers the functions of the "QtSupport" JavaScript object.
void registerQtSupportFunctions(Core::JsBind::Registrar &registrar);

} // namespace QtSupport
