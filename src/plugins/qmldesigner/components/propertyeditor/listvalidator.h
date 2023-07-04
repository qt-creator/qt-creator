// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QQmlEngine>
#include <QValidator>

class ListValidator : public QValidator
{
    Q_OBJECT

    Q_PROPERTY(QStringList filterList READ filterList WRITE setFilterList NOTIFY filterListChanged)

public:
    explicit ListValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &data, int &pos) const override;
    void fixup(QString &input) const override;

    void setFilterList(const QStringList &filterList);
    QStringList filterList() const;

    static void registerDeclarativeType();

signals:
    void filterListChanged();

private:
    QStringList m_filterList;
};

QML_DECLARE_TYPE(ListValidator)
