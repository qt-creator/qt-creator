// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "listvalidator.h"

#include "propertyeditortracing.h"

using QmlDesigner::PropertyEditorTracing::category;

ListValidator::ListValidator(QObject *parent)
    : QValidator{parent}
{
    NanotraceHR::Tracer tracer{"list validator constructor", category()};
}

QValidator::State ListValidator::validate(QString &input, int &) const
{
    NanotraceHR::Tracer tracer{"list validator validate", category()};

    if (input.isEmpty())
        return QValidator::Intermediate;

    State state = QValidator::Invalid;

    for (const QString &item : m_filterList) {
        if (item.compare(input, Qt::CaseSensitive) == 0)
            return QValidator::Acceptable;

        if (item.compare(input, Qt::CaseInsensitive) == 0)
            return QValidator::Intermediate;

        if (item.contains(input, Qt::CaseInsensitive))
            state = QValidator::Intermediate;
    }

    return state;
}

void ListValidator::fixup(QString &input) const
{
    NanotraceHR::Tracer tracer{"list validator fixup", category()};

    for (const QString &item : m_filterList) {
        if (item.compare(input, Qt::CaseInsensitive) == 0) {
            input = item;
            return;
        }
    }
}

void ListValidator::setFilterList(const QStringList &filterList)
{
    NanotraceHR::Tracer tracer{"list validator set filter list", category()};

    if (m_filterList == filterList)
        return;

    m_filterList = filterList;
    emit filterListChanged();
}

QStringList ListValidator::filterList() const
{
    NanotraceHR::Tracer tracer{"list validator filter list", category()};

    return m_filterList;
}

void ListValidator::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"list validator register declarative type", category()};

    qmlRegisterType<ListValidator>("HelperWidgets", 2, 0, "ListValidator");
}
