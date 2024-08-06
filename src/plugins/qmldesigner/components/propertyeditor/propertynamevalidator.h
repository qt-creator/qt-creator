// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QQmlEngine>
#include <QValidator>

class PropertyNameValidator : public QValidator
{
    Q_OBJECT

public:
    explicit PropertyNameValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &pos) const override;

    static void registerDeclarativeType();
};

QML_DECLARE_TYPE(PropertyNameValidator)
