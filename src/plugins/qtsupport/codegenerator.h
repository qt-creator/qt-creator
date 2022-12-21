// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <QObject>
#include <QStringList>

namespace QtSupport {

class QTSUPPORT_EXPORT CodeGenerator : public QObject
{
    Q_OBJECT

public:
    CodeGenerator(QObject *parent = nullptr) : QObject(parent) { }

    // Ui file related:
    // Change the class name in a UI XML form
    Q_INVOKABLE static QString changeUiClassName(const QString &uiXml, const QString &newUiClassName);

    // Low level method to get everything at the same time:
    static bool uiData(const QString &uiXml, QString *formBaseClass, QString *uiClassName);

    Q_INVOKABLE static QString uiClassName(const QString &uiXml);

    // Generic Qt:
    Q_INVOKABLE static QString qtIncludes(const QStringList &qt4, const QStringList &qt5);

    // UI file integration
    Q_INVOKABLE static bool uiAsPointer();
    Q_INVOKABLE static bool uiAsMember();
    Q_INVOKABLE static bool uiAsInheritance();
};

} // namespace QtSupport
