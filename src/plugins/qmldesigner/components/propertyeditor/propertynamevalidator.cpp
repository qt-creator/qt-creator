// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertynamevalidator.h"

#include "propertyeditortracing.h"

#include <modelutils.h>

#include <QRegularExpression>

using QmlDesigner::PropertyEditorTracing::category;

PropertyNameValidator::PropertyNameValidator(QObject *parent)
    : QValidator(parent)
{
    NanotraceHR::Tracer tracer{"property name validator constructor", category()};
}

QValidator::State PropertyNameValidator::validate(QString &input, int &) const
{
    NanotraceHR::Tracer tracer{"property name validator validate", category()};

    if (input.isEmpty())
        return QValidator::Intermediate;

    // Property names must begin with a lower case letter or underscore and can only contain
    // letters, numbers and underscores. JavaScript reserved words are not valid property names.

    if (QmlDesigner::ModelUtils::isQmlKeyword(input))
        return QValidator::Intermediate;

    static QRegularExpression regExp(R"(^[_a-z]\w*$)");

    if (input.contains(regExp))
        return QValidator::Acceptable;

    return QValidator::Invalid;
}

void PropertyNameValidator::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"property name validator register declarative type", category()};

    qmlRegisterType<PropertyNameValidator>("HelperWidgets", 2, 0, "PropertyNameValidator");
}
