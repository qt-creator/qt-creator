// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesignerbase_global.h"

#include <QtQml/qqml.h>

#include <QLocale>
#include <QValidator>

namespace QmlDesigner {

class QMLDESIGNERBASE_EXPORT StudioIntValidator : public QIntValidator
{
    Q_OBJECT
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)

public:
    StudioIntValidator(QObject *parent = nullptr);

    static void registerDeclarativeType();

    QLocale locale() const;
    void setLocale(const QLocale &locale);

signals:
    void localeChanged();
};

class QMLDESIGNERBASE_EXPORT StudioDoubleValidator : public QDoubleValidator
{
    Q_OBJECT
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)

public:
    StudioDoubleValidator(QObject *parent = nullptr);

    static void registerDeclarativeType();

    QLocale locale() const;
    void setLocale(const QLocale &locale);

signals:
    void localeChanged();
};

} // namespace QmlDesigner
