// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "studiovalidator.h"

#include <coreplugin/icore.h>

namespace QmlDesigner {

StudioIntValidator::StudioIntValidator(QObject *parent)
    : QIntValidator(parent)
{}

void StudioIntValidator::registerDeclarativeType()
{
    qmlRegisterType<StudioIntValidator>("StudioQuickUtils", 1, 0, "IntValidator");
}

QLocale StudioIntValidator::locale() const
{
    return QIntValidator::locale();
}

void StudioIntValidator::setLocale(const QLocale &locale)
{
    if (QIntValidator::locale() != locale) {
        QIntValidator::setLocale(locale);
        emit localeChanged();
    }
}

StudioDoubleValidator::StudioDoubleValidator(QObject *parent)
    : QDoubleValidator(parent)
{}

void StudioDoubleValidator::registerDeclarativeType()
{
    qmlRegisterType<StudioDoubleValidator>("StudioQuickUtils", 1, 0, "DoubleValidator");
}

QLocale StudioDoubleValidator::locale() const
{
    return QDoubleValidator::locale();
}

void StudioDoubleValidator::setLocale(const QLocale &locale)
{
    if (QDoubleValidator::locale() != locale) {
        QDoubleValidator::setLocale(locale);
        emit localeChanged();
    }
}

} // namespace QmlDesigner
