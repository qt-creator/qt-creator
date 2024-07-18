// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertynamevalidator.h"

#include "modelutils.h"

#include <QRegularExpression>

PropertyNameValidator::PropertyNameValidator(QObject *parent)
    : QValidator(parent)
{}

QValidator::State PropertyNameValidator::validate(QString &input, int &) const
{
    if (input.isEmpty())
        return QValidator::Intermediate;

    // Property names must begin with a lower case letter and can only contain letters, numbers
    // and underscores. JavaScript reserved words are not valid property names.

    if (QmlDesigner::ModelUtils::isQmlKeyword(input))
        return QValidator::Intermediate;

    static QRegularExpression regExp(R"(^[a-z]\w*$)");

    if (input.contains(regExp))
        return QValidator::Acceptable;

    return QValidator::Invalid;
}

void PropertyNameValidator::registerDeclarativeType()
{
    qmlRegisterType<PropertyNameValidator>("HelperWidgets", 2, 0, "PropertyNameValidator");
}
