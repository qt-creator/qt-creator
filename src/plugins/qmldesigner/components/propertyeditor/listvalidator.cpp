// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "listvalidator.h"

ListValidator::ListValidator(QObject *parent)
    : QValidator{parent}
{}

QValidator::State ListValidator::validate(QString &input, int &) const
{
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
    for (const QString &item : m_filterList) {
        if (item.compare(input, Qt::CaseInsensitive) == 0) {
            input = item;
            return;
        }
    }
}

void ListValidator::setFilterList(const QStringList &filterList)
{
    if (m_filterList == filterList)
        return;

    m_filterList = filterList;
    emit filterListChanged();
}

QStringList ListValidator::filterList() const
{
    return m_filterList;
}

void ListValidator::registerDeclarativeType()
{
    qmlRegisterType<ListValidator>("HelperWidgets", 2, 0, "ListValidator");
}
